#include "Asistente.hpp"

#include <algorithm>
#include <cmath>

#include "Closet.hpp"
#include "Texto.hpp"

namespace {
const char* DIAS[] = {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"};
const char* MESES[] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio",
                       "agosto", "septiembre", "octubre", "noviembre", "diciembre"};

std::string estiloDeEvento(const std::string& asunto, Prioridad prioridad) {
    const std::string n = texto::normalizar(asunto);
    auto tiene = [&](std::initializer_list<const char*> palabras) {
        for (const char* palabra : palabras) if (texto::contiene(n, palabra)) return true;
        return false;
    };
    if (tiene({"informal", "casual", "familia", "familiar", "amigos", "amigas", "paseo", "fiesta"}))
        return "informal";
    if (tiene({"formal", "trabajo", "oficina", "cliente", "entrevista", "conferencia", "presentacion",
               "reunion", "boda", "universidad", "proyecto", "examen"}) ||
        prioridad == Prioridad::Alta)
        return "formal";
    return "";
}

std::string nombreCategoria(const std::string& c) {
    return c == "superior" ? "parte de arriba" : c == "inferior" ? "parte de abajo" : c;
}

std::string horaHablada(int h24, int min) {
    int h12 = h24 % 12 == 0 ? 12 : h24 % 12;
    std::string s = std::to_string(h12);
    if (min) s += ":" + std::string(min < 10 ? "0" : "") + std::to_string(min);
    if (h24 == 12 && min == 0) return s + " del mediodía";
    if (h24 < 12) return s + " de la mañana";
    if (h24 < 19) return s + " de la tarde";
    return s + " de la noche";
}

std::optional<bool> respuestaConfirmacion(const std::string& frase) {
    const std::string respuesta = texto::normalizar(frase);
    if (respuesta.empty()) return std::nullopt;

    if (texto::contiene(respuesta, "no hay problema") ||
        texto::contiene(respuesta, "no me opongo") ||
        texto::contiene(respuesta, "sin problema"))
        return true;

    for (const char* negativa : {"no", "negativo", "rechazo", "cancela", "cancelar", "cancelo",
                                 "mejor no", "para nada", "de ninguna manera", "no quiero",
                                 "no lo hagas", "detente", "anula"})
        if (texto::contiene(respuesta, negativa)) return false;

    for (const char* afirmativa : {"si", "confirmo", "afirmativo", "claro", "por supuesto",
                                   "desde luego", "de acuerdo", "esta bien", "acepto", "adelante", "hazlo",
                                   "procede", "ok", "vale", "correcto", "listo"})
        if (texto::contiene(respuesta, afirmativa)) return true;
    return std::nullopt;
}
}  // namespace

Asistente::Asistente(std::string carpetaDatos, std::string usuario)
    : repo_(std::move(carpetaDatos)), usuario_(std::move(usuario)) {}

const Resultado& Asistente::analizar(const std::string& frase) const {
    const std::time_t t = ahora();
    if (!cache_.valido || cache_.ahora != t || cache_.frase != frase) {
        cache_.resultado = nlu_.analizar(frase, t);
        cache_.frase = frase;
        cache_.ahora = t;
        cache_.valido = true;
    }
    return cache_.resultado;
}

std::string Asistente::describirCuando(std::time_t cuando, bool tieneHora) const {
    if (cuando == 0) return "sin fecha";
    std::tm hoy = tmLocal(ahora()), t = tmLocal(cuando);
    hoy.tm_hour = hoy.tm_min = hoy.tm_sec = 0; hoy.tm_isdst = -1;
    std::tm dia = t;
    dia.tm_hour = dia.tm_min = dia.tm_sec = 0; dia.tm_isdst = -1;
    long diff = std::lround(difftime(std::mktime(&dia), std::mktime(&hoy)) / 86400.0);

    std::string s;
    if (diff == 0) s = "hoy";
    else if (diff == 1) s = "mañana";
    else if (diff == 2) s = "pasado mañana";
    else if (diff == -1) s = "ayer";
    else if (diff > 2 && diff < 7) s = std::string("el ") + DIAS[t.tm_wday];
    else s = std::string("el ") + std::to_string(t.tm_mday) + " de " + MESES[t.tm_mon];
    if (tieneHora) s += " a las " + horaHablada(t.tm_hour, t.tm_min);
    return s;
}

bool Asistente::vencida(const Tarea& t) const {
    if (t.completada() || !t.cuando()) return false;
    if (t.tieneHora()) return t.cuando() < ahora();
    std::tm a = tmLocal(t.cuando()), b = tmLocal(ahora());
    return a.tm_year < b.tm_year || (a.tm_year == b.tm_year && a.tm_yday < b.tm_yday);
}

std::string Asistente::resumen(bool corto) const {
    // Una sola pasada sobre la lista propia, guardando punteros (antes: dos copias completas de pendientes()).
    std::vector<const Tarea*> hoy, vencidas;
    const Tarea* siguiente = nullptr;  // lo más próximo que aún no ha ocurrido y no es de hoy
    const std::time_t ahoraTs = ahora();
    for (const Tarea& t : repo_.listaTareas()) {
        if (t.completada()) continue;
        if (vencida(t)) { vencidas.push_back(&t); continue; }
        if (!t.cuando()) continue;
        if (mismoDia(t.cuando(), ahoraTs)) hoy.push_back(&t);
        else if (!siguiente || t.cuando() < siguiente->cuando()) siguiente = &t;
    }
    auto cuantas = [](size_t n, const char* uno, const char* varias) {
        return std::to_string(n) + " " + (n == 1 ? uno : varias);
    };
    if (hoy.empty() && vencidas.empty()) return "Su agenda de hoy está despejada.";
    std::string r = hoy.empty() ? "No tiene nada programado para hoy" : "Hoy tiene " + cuantas(hoy.size(), "tarea", "tareas");
    if (!vencidas.empty())
        r += (hoy.empty() ? ", pero sí " : " y ") + cuantas(vencidas.size(), "vencida", "vencidas");
    r += ".";
    if (corto) return r;
    for (size_t i = 0; i < hoy.size() && i < 4; ++i) {
        r += "\n" + std::to_string(i + 1) + ". " + hoy[i]->titulo();
        if (hoy[i]->tieneHora()) r += " (" + describirCuando(hoy[i]->cuando(), true) + ")";
    }
    for (size_t i = 0; i < vencidas.size() && i < 3; ++i)
        r += "\nVencida: " + vencidas[i]->titulo() + " (" + describirCuando(vencidas[i]->cuando(), vencidas[i]->tieneHora()) + ")";
    if (siguiente) r += "\nLo siguiente en su agenda: " + siguiente->titulo() + ", " +
                        describirCuando(siguiente->cuando(), siguiente->tieneHora()) + ".";
    return r;
}

std::string Asistente::saludo() const {
    int h = tmLocal(ahora()).tm_hour;
    std::string base = h < 12 ? "Buenos días" : h < 19 ? "Buenas tardes" : "Buenas noches";
    return base + ", " + usuario_ + ". Todos los sistemas en línea. " + resumen(true) + " ¿En qué puedo asistirle?";
}

std::string Asistente::avisoDeConflicto(const Tarea& nueva) const {
    if (!nueva.cuando() || !nueva.tieneHora()) return "";
    for (const Tarea& t : repo_.listaTareas()) {
        if (t.completada() || t.id() == nueva.id() || !t.tieneHora() || !t.cuando()) continue;
        if (std::fabs(std::difftime(t.cuando(), nueva.cuando())) < 3600)
            return " Si me permite la observación, ya tiene «" + t.titulo() + "» " + describirCuando(t.cuando(), true) +
                   "; los horarios se solapan.";
    }
    return "";
}

std::optional<int> Asistente::resolverTarea(const Entidades& e, bool soloPendientes) {
    if (!e.asunto.empty()) return repo_.buscarTarea(e.asunto, soloPendientes);
    if (ultimaTareaId_) {  // sin asunto: «complétala», «bórrala», «cámbiala…» se refieren a la última tarea mencionada
        auto t = repo_.obtenerTarea(ultimaTareaId_);
        if (t && (!soloPendientes || !t->completada())) return ultimaTareaId_;
    }
    return std::nullopt;
}

std::string Asistente::fechaHora() const {
    std::tm t = tmLocal(ahora());
    int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
    return std::string(h12 == 1 ? "Es la " : "Son las ") + horaHablada(t.tm_hour, t.tm_min) + ", " + usuario_ +
           ". Hoy es " + DIAS[t.tm_wday] + " " + std::to_string(t.tm_mday) + " de " + MESES[t.tm_mon] + ".";
}

std::string Asistente::crearTarea(const Entidades& e) {
    if (e.asunto.empty()) return "¿Qué desea que agende, " + usuario_ + "?";
    const std::string estilo = estiloDeEvento(e.asunto, e.prioridad);
    Tarea creada = repo_.agregarTarea(Tarea(e.asunto, e.cuando, e.tieneHora, e.prioridad, estilo));
    ultimaTareaId_ = creada.id();
    historial_.push_back({Accion::Crear, creada.id(), creada});
    std::string que = "«" + e.asunto + "»";
    if (e.cuando) que += " para " + describirCuando(e.cuando, e.tieneHora);
    std::string r;
    switch (turno_++ % 3) {
        case 0: r = "Como usted ordene, " + usuario_ + ". He agendado " + que + "."; break;
        case 1: r = "Con gusto, " + usuario_ + ". Queda agendado " + que + "."; break;
        default: r = "Me ocupo de ello, " + usuario_ + ". He registrado " + que + "."; break;
    }
    if (e.prioridad == Prioridad::Alta) r += " Lo he marcado como prioritario.";
    if (!estilo.empty()) r += estilo == "formal" ? " Evento formal." : " Evento informal.";
    if (e.horaAmbigua && e.cuando) {
        std::tm t = tmLocal(e.cuando);
        r += " Supuse que era a las " + horaHablada(t.tm_hour, t.tm_min) + "; corríjame si me equivoco.";
    }
    return r + avisoDeConflicto(creada);
}

std::string Asistente::crearNota(const Entidades& e) {
    if (e.asunto.empty()) return "¿Qué desea que anote, " + usuario_ + "?";
    auto palabras = texto::dividir(e.asunto);
    std::string titulo;
    for (size_t i = 0; i < palabras.size() && i < 4; ++i) titulo += (i ? " " : "") + palabras[i];
    if (palabras.size() > 4) titulo += "…";

    std::string norm = texto::normalizar(e.asunto), categoria = "General";
    for (const char* k : {"mi mama", "mi madre", "mi papa", "mi padre", "mi hermana", "mi hermano",
                          "mi hija", "mi hijo", "mi pareja", "mi esposa", "mi esposo", "mi novia",
                          "mi novio", "mi amigo", "mi amiga", "mi mejor amigo", "mi mejor amiga",
                          "mi abuelo", "mi abuela", "mi primo", "mi prima", "mi tio", "mi tia",
                          "mi familia", "mis amigos", "mis amigas",
                          "vivo en", "resido en", "mi ciudad", "mi lugar favorito", "mi casa queda",
                          "mi trabajo esta en"})
        if (texto::contiene(norm, k)) { categoria = "Contexto personal"; break; }
    for (const char* k : {"me gusta", "me gustan", "me encanta", "me encantan", "prefiero",
                          "preferencia", "favorito", "favorita", "me fascina", "me fascinan",
                          "me apasiona", "me apasionan", "adoro", "disfruto", "disfruta"})
        if (categoria == "General" && texto::contiene(norm, k)) { categoria = "Preferencias"; break; }
    for (const char* k : {"parcial", "examen", "clase", "universidad", "ula", "profesor", "tarea", "proyecto"})
        if (categoria == "General" && texto::contiene(norm, k)) { categoria = "Universidad"; break; }
    repo_.agregarNota(Nota(titulo, e.asunto, categoria));
    return (turno_++ % 2 ? "Anotado, " : "Muy bien, ") + usuario_ + ". Lo he guardado en " + categoria + ": " + e.asunto + ".";
}

std::optional<std::string> Asistente::recordarPersona(const std::string& frase) {
    const auto normalizado = texto::normalizarConMapa(frase);
    static const std::vector<std::pair<std::string, std::string>> relaciones = {
        {"mejor amigo", "mejor amigo"}, {"mejor amiga", "mejor amiga"},
        {"abuelo", "abuelo"}, {"abuela", "abuela"}, {"primo", "primo"}, {"prima", "prima"},
        {"tio", "tío"}, {"tia", "tía"}, {"mama", "mamá"}, {"madre", "madre"},
        {"papa", "papá"}, {"padre", "padre"}, {"hermana", "hermana"}, {"hermano", "hermano"},
        {"hija", "hija"}, {"hijo", "hijo"}, {"pareja", "pareja"}, {"esposa", "esposa"},
        {"esposo", "esposo"}, {"novia", "novia"}, {"novio", "novio"}, {"amigo", "amigo"},
        {"amiga", "amiga"}};
    static const std::vector<std::string> conectores = {" se llama ", " es "};
    std::vector<size_t> inicios = {0};
    if (normalizado.s.rfind("nix ", 0) == 0) inicios.push_back(4);
    if (normalizado.s.rfind("oye nix ", 0) == 0) inicios.push_back(8);
    for (size_t inicioRelacion : inicios) {
        for (const auto& [clave, relacion] : relaciones) {
            for (const std::string& conector : conectores) {
                const std::string prefijo = "mi " + clave + conector;
                const size_t indice = inicioRelacion + prefijo.size();
                if (normalizado.s.compare(inicioRelacion, prefijo.size(), prefijo) != 0 ||
                    indice >= normalizado.origen.size()) continue;
                size_t inicio = normalizado.origen[indice].first;
                std::string nombre = frase.substr(inicio);
                while (!nombre.empty() && std::string(" \t\r\n.,;:!?¡¿\"'").find(nombre.back()) != std::string::npos)
                    nombre.pop_back();
                while (!nombre.empty() && std::string(" \t\r\n\"'").find(nombre.front()) != std::string::npos)
                    nombre.erase(nombre.begin());
                if (nombre.empty() || nombre.size() > 80) continue;

                std::string titulo = "Persona: " + relacion;
                if (relacion == "amigo" || relacion == "amiga" || relacion == "mejor amigo" ||
                    relacion == "mejor amiga")
                    titulo += ": " + texto::normalizar(nombre);
                std::vector<int> anteriores;
                for (const Nota& n : repo_.notas())
                    if (n.categoria() == "Contexto personal" && n.titulo() == titulo) anteriores.push_back(n.id());
                for (int id : anteriores) repo_.eliminarNota(id);
                repo_.agregarNota(Nota(titulo, "Mi " + relacion + " se llama " + nombre, "Contexto personal"));
                return "Lo recordaré: su " + relacion + " se llama " + nombre + ".";
            }
        }
    }
    return std::nullopt;
}

std::string Asistente::listarTareas(const Entidades& e) const {
    std::vector<const Tarea*> lista;  // punteros a la lista propia: no se copia ninguna tarea
    for (const Tarea& t : repo_.listaTareas()) {
        if (t.completada()) continue;
        if (e.cuando && (!t.cuando() || !mismoDia(t.cuando(), e.cuando))) continue;
        lista.push_back(&t);
    }
    if (lista.empty())
        return e.cuando ? "No tiene nada programado para ese día, " + usuario_ + "."
                        : "No tiene tareas pendientes, " + usuario_ + ". Agenda despejada.";
    std::string r = "Tiene " + std::to_string(lista.size()) + (lista.size() == 1 ? " tarea pendiente, " : " tareas pendientes, ") +
                    usuario_ + ":";
    for (size_t i = 0; i < lista.size() && i < 6; ++i) {
        r += "\n" + std::to_string(i + 1) + ". " + lista[i]->titulo();
        if (lista[i]->cuando())
            r += std::string(" (") + (vencida(*lista[i]) ? "vencida, " : "") +
                 describirCuando(lista[i]->cuando(), lista[i]->tieneHora()) + ")";
    }
    if (lista.size() == 1) ultimaTareaId_ = lista[0]->id();
    if (lista.size() > 6) r += "\ny " + std::to_string(lista.size() - 6) + " más en pantalla.";
    return r;
}

std::string Asistente::listarNotas() const {
    const auto& n = repo_.notas();
    if (n.empty()) return "No tiene notas guardadas, " + usuario_ + ".";
    std::string r = "Tiene " + std::to_string(n.size()) + (n.size() == 1 ? " nota, " : " notas, ") + usuario_ + ":";
    for (size_t i = 0; i < n.size() && i < 6; ++i) r += "\n• " + n[i].contenido();
    if (n.size() > 6) r += "\ny " + std::to_string(n.size() - 6) + " más en pantalla.";
    return r;
}

std::string Asistente::completarOEliminar(const Entidades& e, bool completar) {
    auto id = resolverTarea(e, completar);
    if (!id) {
        if (e.asunto.empty())
            return (completar ? "¿Cuál tarea desea marcar como completada, " : "¿Cuál tarea desea eliminar, ") + usuario_ + "?";
        return "Me temo que no encuentro ninguna tarea que coincida con «" + e.asunto + "», " + usuario_ + ".";
    }
    Tarea antes = *repo_.obtenerTarea(*id);
    if (completar) {
        repo_.completarTarea(*id);
        historial_.push_back({Accion::Completar, *id, antes});
        ultimaTareaId_ = *id;
        return (turno_++ % 2 ? "Excelente, " : "Hecho, ") + usuario_ + ". He marcado como completada: " + antes.titulo() + ".";
    }
    repo_.eliminarTarea(*id);
    historial_.push_back({Accion::Eliminar, *id, antes});
    ultimaTareaId_ = 0;
    return "Muy bien, " + usuario_ + ". He eliminado la tarea: " + antes.titulo() + ".";
}

std::string Asistente::reprogramar(const Entidades& e) {
    if (!e.cuando) return "¿Para cuándo desea moverla, " + usuario_ + "?";
    auto id = resolverTarea(e, true);
    if (!id) {
        if (e.asunto.empty()) return "¿Qué tarea desea mover, " + usuario_ + "?";
        return "Me temo que no encuentro ninguna tarea que coincida con «" + e.asunto + "», " + usuario_ + ".";
    }
    Tarea antes = *repo_.obtenerTarea(*id);
    std::time_t nuevo = e.cuando;
    bool conHora = e.tieneHora;
    if (!e.fechaExplicita && e.tieneHora && antes.cuando()) {          // «a las 5»: conserva el día que ya tenía
        std::tm d = tmLocal(antes.cuando()), hh = tmLocal(e.cuando);
        d.tm_hour = hh.tm_hour; d.tm_min = hh.tm_min; d.tm_sec = 0; d.tm_isdst = -1;
        nuevo = std::mktime(&d);
    } else if (e.fechaExplicita && !e.tieneHora && antes.tieneHora()) {  // «para el viernes»: conserva la hora
        std::tm d = tmLocal(e.cuando), hh = tmLocal(antes.cuando());
        d.tm_hour = hh.tm_hour; d.tm_min = hh.tm_min; d.tm_sec = 0; d.tm_isdst = -1;
        nuevo = std::mktime(&d);
        conHora = true;
    }
    repo_.reprogramarTarea(*id, nuevo, conHora);
    historial_.push_back({Accion::Reprogramar, *id, antes});
    ultimaTareaId_ = *id;
    Tarea despues = *repo_.obtenerTarea(*id);
    std::string r = std::string(turno_++ % 2 ? "Como usted ordene, " : "Muy bien, ") + usuario_ + ". «" + despues.titulo() +
                    "» queda para " + describirCuando(despues.cuando(), despues.tieneHora()) + ".";
    return r + avisoDeConflicto(despues);
}

std::string Asistente::deshacer() {
    if (historial_.empty()) return "No hay nada que deshacer, " + usuario_ + ".";
    Accion a = historial_.back();
    historial_.pop_back();
    const std::string t = "«" + a.antes.titulo() + "»";
    switch (a.tipo) {
        case Accion::Crear:
            repo_.eliminarTarea(a.id);
            if (ultimaTareaId_ == a.id) ultimaTareaId_ = 0;
            return "Entendido, " + usuario_ + ". He retirado " + t + " de su agenda.";
        case Accion::Completar:
            repo_.reabrirTarea(a.id);
            ultimaTareaId_ = a.id;
            return "Muy bien, " + usuario_ + ". " + t + " vuelve a estar pendiente.";
        case Accion::Eliminar:
            repo_.restaurarTarea(a.antes);
            ultimaTareaId_ = a.id;
            return "Restaurada, " + usuario_ + ": " + t + ".";
        case Accion::Reprogramar:
            repo_.reprogramarTarea(a.id, a.antes.cuando(), a.antes.tieneHora());
            ultimaTareaId_ = a.id;
            return "Como estaba, " + usuario_ + ": " + t + (a.antes.cuando() ? " " + describirCuando(a.antes.cuando(), a.antes.tieneHora()) : " sin fecha") + ".";
        case Accion::EliminarDia:
            for (const Tarea& tarea : a.varias) repo_.restaurarTarea(tarea);
            ultimaTareaId_ = 0;
            return "He restaurado " + std::to_string(a.varias.size()) +
                   (a.varias.size() == 1 ? " tarea" : " tareas") +
                   " que eliminé de la agenda de hoy, " + usuario_ + ".";
        case Accion::AgregarPrenda:
            repo_.eliminarPrenda(a.id);
            if (ultimaPrendaId_ == a.id) ultimaPrendaId_ = 0;
            return "Entendido, " + usuario_ + ". He retirado «" + a.prenda.nombre() + "» de su closet.";
        case Accion::QuitarPrenda:
            repo_.restaurarPrenda(a.prenda);
            return "Restaurada, " + usuario_ + ": «" + a.prenda.nombre() + "» vuelve a su closet.";
        case Accion::VaciarClosetA:
            for (const Prenda& prenda : a.prendas) repo_.restaurarPrenda(prenda);
            return "He restaurado " + std::to_string(a.prendas.size()) +
                   (a.prendas.size() == 1 ? " prenda" : " prendas") + " en su closet, " + usuario_ + ".";
    }
    return "";
}

std::string Asistente::limpiarAgendaHoy() {
    size_t pendientesHoy = 0;
    for (const Tarea& tarea : repo_.listaTareas())
        if (!tarea.completada() && tarea.cuando() && mismoDia(tarea.cuando(), ahora())) ++pendientesHoy;
    if (!pendientesHoy) return "No tiene tareas pendientes programadas para hoy, " + usuario_ + ".";
    esperandoConfirmacionLimpiarHoy_ = true;
    return "Tiene " + std::to_string(pendientesHoy) +
           (pendientesHoy == 1 ? " tarea pendiente para hoy. " : " tareas pendientes para hoy. ") +
           "¿Confirma que las elimine? Responda «sí» o «no».";
}

std::string Asistente::confirmarLimpiarAgendaHoy() {
    esperandoConfirmacionLimpiarHoy_ = false;
    ListaEnlazada<Tarea> eliminadas;
    for (const Tarea& tarea : repo_.listaTareas())
        if (!tarea.completada() && tarea.cuando() && mismoDia(tarea.cuando(), ahora())) eliminadas.push_back(tarea);
    if (eliminadas.empty()) return "Ya no tiene tareas pendientes para hoy, " + usuario_ + ".";
    const std::size_t cantidadEliminada = eliminadas.size();
    for (const Tarea& tarea : eliminadas) repo_.eliminarTarea(tarea.id());
    historial_.push_back({Accion::EliminarDia, 0, Tarea{}, std::move(eliminadas)});
    ultimaTareaId_ = 0;
    return "He eliminado " + std::to_string(cantidadEliminada) +
           (cantidadEliminada == 1 ? " tarea pendiente de hoy, " : " tareas pendientes de hoy, ") +
           usuario_ + ". Puede decir «deshaz» para restaurarlas.";
}

std::string Asistente::procesar(const std::string& frase) {
    ultimoOutfit_.reset();
    if (esperandoConfirmacionVaciarCloset_) {
        const std::optional<bool> confirmacion = respuestaConfirmacion(frase);
        if (confirmacion && !*confirmacion) {
            esperandoConfirmacionVaciarCloset_ = false;
            return "De acuerdo, no he tocado su closet, " + usuario_ + ".";
        }
        if (confirmacion && *confirmacion) return confirmarVaciarCloset();
        return "Por favor, responda afirmativamente para vaciar el closet o negativamente para cancelarlo, " + usuario_ + ".";
    }
    if (esperandoConfirmacionLimpiarHoy_) {
        const std::optional<bool> confirmacion = respuestaConfirmacion(frase);
        if (confirmacion && !*confirmacion) {
            esperandoConfirmacionLimpiarHoy_ = false;
            return "De acuerdo, no he cambiado su agenda, " + usuario_ + ".";
        }
        if (confirmacion && *confirmacion) return confirmarLimpiarAgendaHoy();
        return "Por favor, responda afirmativamente para confirmar la limpieza o negativamente para cancelarla, " +
               usuario_ + ".";
    }
    if (prendaPendiente_) {  // se preguntó el estilo de una prenda: la respuesta corta es «formal» o «informal»
        PrendaDicha dicha = *prendaPendiente_;
        prendaPendiente_.reset();
        const std::string estilo = closet::estiloEnTexto(frase);
        if (!estilo.empty()) {
            dicha.estilo = estilo;
            ultimaIntencion_ = Intencion::GuardarPrenda;
            return completarPrenda(dicha);
        }
    }
    if (outfitPendiente_) {  // se preguntó para qué estilo es el outfit
        const bool abrigo = outfitAbrigoPendiente_;
        outfitPendiente_ = outfitAbrigoPendiente_ = false;
        const std::string estilo = closet::estiloEnTexto(frase);
        if (!estilo.empty()) {
            ultimaIntencion_ = Intencion::SugerirOutfit;
            return responderOutfit(estilo, abrigo, "");
        }
    }
    if (auto persona = recordarPersona(frase)) return *persona;
    const Resultado r = analizar(frase);
    const Intencion previa = ultimaIntencion_;

    // «bórrala» justo después de guardar una prenda se refiere a esa prenda, no a una tarea.
    if (r.intencion == Intencion::EliminarTarea && previa == Intencion::GuardarPrenda && ultimaPrendaId_ &&
        texto::dividir(r.normalizado).size() <= 2) {
        ultimaIntencion_ = Intencion::EliminarPrenda;
        return quitarPrendaPorId(ultimaPrendaId_);
    }

    // Seguimiento de la conversación: «¿y mañana?», «¿y el viernes?» tras consultar las tareas.
    if (r.intencion == Intencion::Desconocida && previa == Intencion::ListarTareas) {
        Entidades e = nlu_.fechaDe(frase, ahora());
        if (e.cuando && texto::dividir(r.normalizado).size() <= 5) return listarTareas(e);
    }
    ultimaIntencion_ = r.intencion;
    while (historial_.size() > 20) historial_.pop_front();

    switch (r.intencion) {
        case Intencion::Saludo: return saludo();
        case Intencion::FechaHora: return fechaHora();
        case Intencion::CrearTarea: return crearTarea(r.entidades);
        case Intencion::CrearNota: return crearNota(r.entidades);
        case Intencion::ListarTareas: return listarTareas(r.entidades);
        case Intencion::ListarNotas: return listarNotas();
        case Intencion::CompletarTarea: return completarOEliminar(r.entidades, true);
        case Intencion::EliminarTarea: return completarOEliminar(r.entidades, false);
        case Intencion::Reprogramar: return reprogramar(r.entidades);
        case Intencion::Deshacer: return deshacer();
        case Intencion::Resumen: return resumen(false);
        case Intencion::LimpiarAgendaHoy: return limpiarAgendaHoy();
        case Intencion::GuardarPrenda: return guardarPrendaDicha(r.entidades);
        case Intencion::ListarCloset: return listarCloset(r.entidades);
        case Intencion::EliminarPrenda: return quitarPrendaDicha(r.entidades);
        case Intencion::VaciarCloset: return vaciarCloset();
        case Intencion::SugerirOutfit: return sugerirOutfit(r.entidades, r.normalizado);
        case Intencion::Gracias:
            switch (turno_++ % 3) {
                case 0: return "Es un placer, " + usuario_ + ".";
                case 1: return "Para eso estoy, " + usuario_ + ".";
                default: return "A sus órdenes, " + usuario_ + ".";
            }
        case Intencion::Ayuda:
            return "Puedo gestionar tareas y notas, consultar sus pendientes, reprogramar o eliminar tareas, deshacer "
                   "acciones y ofrecer recomendaciones basadas en su agenda y preferencias. Guardo su closet («guarda mi camisa azul, es formal») y le sugiero qué ponerse con sus propias prendas. También puedo responder "
                   "consultas abiertas si conecta un cerebro en Ajustes, " + usuario_ + ".";
        case Intencion::Desconocida: break;
    }
    return "Me temo que no he comprendido la orden, " + usuario_ +
           ". Pruebe con «agendar reunión mañana a las 4» o diga «ayuda».";
}

// ───────────────────────── Closet ─────────────────────────

std::string Asistente::colorPreferido() const {
    const auto& notas = repo_.notas();
    for (auto it = notas.rbegin(); it != notas.rend(); ++it) {  // la preferencia más reciente manda
        if (it->categoria() != "Preferencias") continue;
        const std::string texto = texto::normalizar(it->contenido());
        const bool habla_de_ropa = texto::contiene(texto, "vestir") || texto::contiene(texto, "ropa") ||
                                   texto::contiene(texto, "color") || texto::contiene(texto, "usar") ||
                                   texto::contiene(texto, "ponerme");
        for (const std::string& palabra : texto::dividir(texto)) {
            if (!habla_de_ropa && closet::categoriaDePalabra(palabra).empty()) continue;
            if (auto c = closet::colorDePalabra(palabra); !c.empty()) return c;
        }
    }
    return "";
}

std::optional<Prenda> Asistente::agregarPrenda(const std::string& nombre, const std::string& categoria,
                                               const std::string& color, const std::string& estilo, std::string& error) {
    std::string n = nombre;
    while (!n.empty() && n.front() == ' ') n.erase(n.begin());
    while (!n.empty() && n.back() == ' ') n.pop_back();
    if (n.empty() || n.size() > 60) { error = "El nombre de la prenda debe tener entre 1 y 60 caracteres."; return std::nullopt; }
    if (!Prenda::categoriaValida(categoria)) { error = "La categoría debe ser superior, inferior, calzado o abrigo."; return std::nullopt; }
    if (!Prenda::estiloValido(estilo)) { error = "El estilo debe ser formal o informal."; return std::nullopt; }
    std::string c = texto::normalizar(color);
    if (auto canon = closet::colorDePalabra(c); !canon.empty()) c = canon;
    if (c.size() > 20) { error = "El color es demasiado largo."; return std::nullopt; }
    Prenda p(n, categoria, c, estilo);
    if (repo_.existePrenda(p)) { error = "Ya tiene esa prenda en su closet."; return std::nullopt; }
    return repo_.agregarPrenda(p);
}

std::string Asistente::guardarPrendaDicha(const Entidades& e) {
    PrendaDicha d = e.prenda;
    if (d.nombre.empty()) d.nombre = d.categoria;
    if (d.estilo.empty()) {
        prendaPendiente_ = d;
        return "¿La prenda «" + d.nombre + "» es formal o informal, " + usuario_ + "?";
    }
    return completarPrenda(d);
}

std::string Asistente::completarPrenda(const PrendaDicha& d) {
    std::string error;
    Prenda p(d.nombre, d.categoria, d.color, d.estilo);
    if (repo_.existePrenda(p)) return "Ya tiene «" + d.nombre + "» como prenda " + d.estilo + " en su closet, " + usuario_ + ".";
    p = repo_.agregarPrenda(p);
    ultimaPrendaId_ = p.id();
    Accion a{Accion::AgregarPrenda, p.id(), Tarea{}};
    a.prenda = p;
    historial_.push_back(std::move(a));
    const size_t total = repo_.listaPrendas().size();
    return "Guardada, " + usuario_ + ": «" + p.nombre() + "» (" + p.estilo() + ", " + nombreCategoria(p.categoria()) +
           "). Su closet tiene " + std::to_string(total) + (total == 1 ? " prenda." : " prendas.");
}

std::string Asistente::listarCloset(const Entidades& e) const {
    const auto& todas = repo_.listaPrendas();
    if (todas.empty())
        return "Su closet está vacío, " + usuario_ + ". Puede guardar prendas diciendo «guarda mi camisa azul, es formal».";
    std::vector<Prenda> coinciden;
    for (const Prenda& p : todas) {
        if (!e.prenda.categoria.empty() && p.categoria() != e.prenda.categoria) continue;
        if (!e.prenda.estilo.empty() && p.estilo() != e.prenda.estilo) continue;
        if (!e.prenda.color.empty() && p.color() != e.prenda.color) continue;
        coinciden.push_back(p);
    }
    if (coinciden.empty()) return "No tiene prendas con ese criterio en su closet, " + usuario_ + ".";
    std::string r = "Su closet tiene " + std::to_string(coinciden.size()) + (coinciden.size() == 1 ? " prenda, " : " prendas, ") + usuario_ + ":";
    for (size_t i = 0; i < coinciden.size() && i < 8; ++i)
        r += "\n• " + coinciden[i].nombre() + " — " + coinciden[i].estilo() + ", " + nombreCategoria(coinciden[i].categoria());
    if (coinciden.size() > 8) r += "\ny " + std::to_string(coinciden.size() - 8) + " más en pantalla.";
    return r;
}

std::string Asistente::quitarPrendaPorId(int id) {
    auto p = repo_.obtenerPrenda(id);
    if (!p || !repo_.eliminarPrenda(id)) return "No encuentro esa prenda en su closet, " + usuario_ + ".";
    if (ultimaPrendaId_ == id) ultimaPrendaId_ = 0;
    Accion a{Accion::QuitarPrenda, id, Tarea{}};
    a.prenda = *p;
    historial_.push_back(std::move(a));
    return "He retirado «" + p->nombre() + "» de su closet, " + usuario_ + ". Puede decir «deshaz» para recuperarla.";
}

std::string Asistente::quitarPrendaDicha(const Entidades& e) {
    auto id = repo_.buscarPrenda(e.prenda.nombre);
    if (!id) return "No encuentro esa prenda en su closet, " + usuario_ + ". Diga «mi closet» para ver la lista.";
    return quitarPrendaPorId(*id);
}

std::string Asistente::vaciarCloset() {
    const size_t n = repo_.listaPrendas().size();
    if (!n) return "Su closet ya está vacío, " + usuario_ + ".";
    esperandoConfirmacionVaciarCloset_ = true;
    return "Tiene " + std::to_string(n) + (n == 1 ? " prenda" : " prendas") +
           " en su closet. ¿Confirma que las elimine todas? Responda «sí» o «no».";
}

std::string Asistente::confirmarVaciarCloset() {
    esperandoConfirmacionVaciarCloset_ = false;
    ListaEnlazada<Prenda> quitadas = repo_.listaPrendas();
    if (quitadas.empty()) return "Su closet ya estaba vacío, " + usuario_ + ".";
    const size_t n = quitadas.size();
    repo_.vaciarCloset();
    ultimaPrendaId_ = 0;
    Accion a{Accion::VaciarClosetA, 0, Tarea{}};
    a.prendas = std::move(quitadas);
    historial_.push_back(std::move(a));
    return "He vaciado su closet (" + std::to_string(n) + (n == 1 ? " prenda" : " prendas") + "), " + usuario_ +
           ". Puede decir «deshaz» para restaurarlas.";
}

std::string Asistente::sugerirOutfit(const Entidades& e, const std::string& frase) {
    if (repo_.listaPrendas().empty()) return responderOutfit("", false, "");  // closet vacío: el mensaje es el mismo
    std::string estilo = e.prenda.estilo;
    if (estilo.empty()) estilo = estiloDeEvento(frase, Prioridad::Media);
    std::string compromiso;
    if (estilo.empty()) {  // sin pista en la frase: se mira el primer compromiso de hoy que tenga estilo
        const Tarea* elegida = nullptr;
        for (const Tarea& t : repo_.listaTareas()) {
            if (t.completada() || !t.cuando() || !mismoDia(t.cuando(), ahora()) || t.estiloEvento().empty()) continue;
            if (!elegida || t.cuando() < elegida->cuando()) elegida = &t;
        }
        if (elegida) { estilo = elegida->estiloEvento(); compromiso = elegida->titulo(); }
    }
    if (estilo.empty()) {
        outfitPendiente_ = true;
        outfitAbrigoPendiente_ = e.abrigo;
        return "¿Es para algo formal o informal, " + usuario_ + "?";
    }
    return responderOutfit(estilo, e.abrigo, compromiso);
}

std::string Asistente::responderOutfit(const std::string& estilo, bool abrigo, const std::string& compromiso) {
    const std::vector<Prenda> prendas = repo_.prendas();
    if (prendas.empty())
        return "Su closet está vacío, " + usuario_ + ". Guarde algunas prendas —por ejemplo «guarda mi camisa azul, es formal»— "
               "y podré sugerirle qué ponerse.";
    const Outfit o = armarOutfit(prendas, estilo, colorPreferido(), abrigo);
    ultimoOutfit_ = o;
    const std::string cuando = !compromiso.empty() ? "su compromiso «" + compromiso + "» (" + estilo + ")"
                                                   : "algo " + estilo;
    std::string faltan;
    for (size_t i = 0; i < o.faltantes.size(); ++i)
        faltan += (i == 0 ? "" : i + 1 == o.faltantes.size() ? " y " : ", ") + o.faltantes[i];
    if (o.vacio())
        return "No tiene prendas " + estilo + "es" + " guardadas, " + usuario_ +
               ". Le faltan: " + faltan + ".";
    if (o.completo() && (!abrigo || o.abrigo))
        return "Para " + cuando + ", " + usuario_ + ", le sugiero " + describirOutfit(o) + ".";
    return "Para " + cuando + ", con lo que tiene guardado le sugiero " + describirOutfit(o) + ". Le falta: " + faltan + ".";
}
