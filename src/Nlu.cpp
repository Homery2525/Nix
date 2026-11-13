#include "Nlu.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <map>
#include <regex>
#include <set>
#include <vector>

#include "Closet.hpp"
#include "Texto.hpp"

namespace {

using I = Intencion;
struct Regla { I intencion; const char* clave; int peso; };

// Intent matching por palabras clave con puntaje: gana la intención con más peso acumulado.
const Regla REGLAS[] = {
    {I::CrearTarea, "agendar", 3}, {I::CrearTarea, "agendame", 3}, {I::CrearTarea, "agenda", 2},
    {I::CrearTarea, "recordar", 3}, {I::CrearTarea, "recuerdame", 3}, {I::CrearTarea, "recuerda", 2},
    {I::CrearTarea, "recordatorio", 3}, {I::CrearTarea, "tarea", 2}, {I::CrearTarea, "nueva tarea", 2},
    {I::CrearTarea, "pendiente", 2}, {I::CrearTarea, "programar", 2}, {I::CrearTarea, "programame", 2},
    {I::CrearTarea, "tengo que", 2}, {I::CrearTarea, "debo", 2}, {I::CrearTarea, "no olvides", 3},

    {I::CrearNota, "nota", 3}, {I::CrearNota, "anota", 3}, {I::CrearNota, "anotar", 3},
    {I::CrearNota, "apunta", 3}, {I::CrearNota, "apuntar", 3}, {I::CrearNota, "toma nota", 3},
    {I::CrearNota, "guarda", 1},

    {I::ListarTareas, "mis tareas", 4}, {I::ListarTareas, "tareas", 2}, {I::ListarTareas, "mis pendientes", 4},
    {I::ListarTareas, "pendientes", 2}, {I::ListarTareas, "mi agenda", 4}, {I::ListarTareas, "que tengo", 3},
    {I::ListarTareas, "que debo hacer", 4}, {I::ListarTareas, "que me falta", 3},
    {I::ListarTareas, "muestrame", 1}, {I::ListarTareas, "dime", 1}, {I::ListarTareas, "lista", 1},

    {I::ListarNotas, "mis notas", 4}, {I::ListarNotas, "notas", 2},

    {I::CompletarTarea, "termine", 4}, {I::CompletarTarea, "terminar", 3}, {I::CompletarTarea, "complete", 4},
    {I::CompletarTarea, "completa", 4}, {I::CompletarTarea, "completar", 4}, {I::CompletarTarea, "hice", 4},
    {I::CompletarTarea, "finalice", 4}, {I::CompletarTarea, "hecha", 2}, {I::CompletarTarea, "marca", 2},

    {I::EliminarTarea, "elimina", 4}, {I::EliminarTarea, "eliminar", 4}, {I::EliminarTarea, "borra", 4},
    {I::EliminarTarea, "borrar", 4}, {I::EliminarTarea, "quita", 3}, {I::EliminarTarea, "cancela", 3},
    {I::EliminarTarea, "cancelar", 3},

    {I::Saludo, "hola", 3}, {I::Saludo, "buenos dias", 3}, {I::Saludo, "buenas tardes", 3},
    {I::Saludo, "buenas noches", 3}, {I::Saludo, "buenas", 2},

    {I::FechaHora, "que hora es", 4}, {I::FechaHora, "que dia es hoy", 4}, {I::FechaHora, "que fecha es", 4},
    {I::FechaHora, "hora", 1},

    {I::Ayuda, "ayuda", 3}, {I::Ayuda, "que puedes hacer", 4}, {I::Ayuda, "como funcionas", 3},

    {I::Gracias, "gracias", 3},

    {I::CompletarTarea, "completala", 4}, {I::CompletarTarea, "completalo", 4}, {I::CompletarTarea, "marcala", 3},
    {I::CompletarTarea, "marcalo", 3}, {I::CompletarTarea, "terminala", 4}, {I::CompletarTarea, "terminalo", 4},
    {I::EliminarTarea, "eliminala", 4}, {I::EliminarTarea, "eliminalo", 4}, {I::EliminarTarea, "borrala", 4},
    {I::EliminarTarea, "borralo", 4}, {I::EliminarTarea, "quitala", 3}, {I::EliminarTarea, "cancelala", 3},

    {I::Reprogramar, "cambia", 4}, {I::Reprogramar, "cambiar", 4}, {I::Reprogramar, "cambialo", 4},
    {I::Reprogramar, "cambiala", 4}, {I::Reprogramar, "mueve", 4}, {I::Reprogramar, "mover", 4},
    {I::Reprogramar, "muevelo", 4}, {I::Reprogramar, "muevela", 4}, {I::Reprogramar, "pasalo", 4},
    {I::Reprogramar, "pasala", 4}, {I::Reprogramar, "posponer", 4}, {I::Reprogramar, "pospon", 4},
    {I::Reprogramar, "posponlo", 4}, {I::Reprogramar, "posponla", 4}, {I::Reprogramar, "aplaza", 4},
    {I::Reprogramar, "aplazar", 4}, {I::Reprogramar, "reprograma", 5}, {I::Reprogramar, "reprogramar", 5},
    {I::Reprogramar, "adelanta", 4}, {I::Reprogramar, "adelantala", 4}, {I::Reprogramar, "adelantalo", 4},
    {I::Reprogramar, "atrasa", 4}, {I::Reprogramar, "atrasala", 4}, {I::Reprogramar, "atrasalo", 4},

    {I::Deshacer, "deshaz", 5}, {I::Deshacer, "deshacer", 5}, {I::Deshacer, "deshazlo", 5},
    {I::Deshacer, "cancela eso", 5}, {I::Deshacer, "olvida eso", 5}, {I::Deshacer, "anula eso", 5},
    {I::Deshacer, "no era eso", 5}, {I::Deshacer, "me equivoque", 5}, {I::Deshacer, "revierte", 5},

    {I::Resumen, "resumen", 4}, {I::Resumen, "como va mi dia", 5}, {I::Resumen, "ponme al dia", 5},
    {I::Resumen, "briefing", 4}, {I::Resumen, "informe del dia", 5}, {I::Resumen, "que hay para hoy", 4},

    {I::LimpiarAgendaHoy, "limpia mi agenda", 12}, {I::LimpiarAgendaHoy, "limpiar mi agenda", 12},
    {I::LimpiarAgendaHoy, "quiero mi dia libre", 12},
    {I::LimpiarAgendaHoy, "limpia la agenda", 12}, {I::LimpiarAgendaHoy, "limpiar agenda de hoy", 12},
    {I::LimpiarAgendaHoy, "limpia agenda de hoy", 12}, {I::LimpiarAgendaHoy, "elimina todas mis actividades", 12},
    {I::LimpiarAgendaHoy, "borra todas mis actividades", 12}, {I::LimpiarAgendaHoy, "elimina mis actividades de hoy", 12},
    {I::LimpiarAgendaHoy, "borra mis actividades de hoy", 12}, {I::LimpiarAgendaHoy, "borra todo lo de hoy", 12},
    {I::LimpiarAgendaHoy, "elimina todo lo de hoy", 12}, {I::LimpiarAgendaHoy, "cancela todo lo de hoy", 12},
    {I::LimpiarAgendaHoy, "vaciar mi agenda", 12}, {I::LimpiarAgendaHoy, "vacía mi agenda", 12},
    {I::LimpiarAgendaHoy, "elimina todas las tareas de hoy", 12}, {I::LimpiarAgendaHoy, "borra todas las tareas de hoy", 12},
    {I::LimpiarAgendaHoy, "elimina todas las actividades de hoy", 12}, {I::LimpiarAgendaHoy, "borra las actividades de hoy", 12},
    {I::LimpiarAgendaHoy, "cancela mis actividades de hoy", 12}, {I::LimpiarAgendaHoy, "elimina todo lo que tengo hoy", 12},
};

// Palabras de "mando" que se quitan al inicio para quedarnos con el asunto.
const std::set<std::string> LIDER = {
    "jarvis", "nix", "por", "favor", "puedes", "podrias", "quiero", "quisiera", "que", "me", "mi", "mis", "una", "un",
    "nueva", "nuevo", "nota", "tarea", "recordatorio", "pendiente", "recordar", "recuerdame", "recuerda",
    "agendar", "agendame", "agenda", "agrega", "agregar", "anade", "anotar", "anota", "apunta", "apuntar",
    "crear", "crea", "toma", "programar", "programa", "programame", "tengo", "tienes", "necesito", "debo",
    "para", "de", "sobre", "completa", "completar", "termine", "complete", "marca", "como", "hecha", "hecho",
    "elimina", "eliminar", "borra", "borrar", "quita", "quitar", "cancela", "cancelar", "ya", "hice",
    "finalice", "no", "olvides", "guarda",
    "cambia", "cambiar", "cambialo", "cambiala", "mueve", "mover", "muevelo", "muevela", "pasalo", "pasala",
    "posponer", "pospon", "posponlo", "posponla", "aplaza", "aplazar", "reprograma", "reprogramar",
    "adelanta", "adelantala", "adelantalo", "atrasa", "atrasala", "atrasalo",
    "completala", "completalo", "marcala", "marcalo", "terminala", "terminalo", "eliminala", "eliminalo",
    "borrala", "borralo", "quitala", "quitalo", "cancelala", "cancelalo",
    "esa", "ese", "eso", "esta", "este", "ultima", "ultimo", "anterior", "misma", "lo"};
const std::set<std::string> ARTICULOS = {"el", "la", "los", "las"};
const std::set<std::string> COLA = {"de", "a", "al", "para", "el", "la", "en", "y", "que", "por", "del", "los", "las"};

int numero(const std::string& t) {
    static const std::map<std::string, int> m = {
        {"un", 1}, {"una", 1}, {"dos", 2}, {"tres", 3}, {"cuatro", 4}, {"cinco", 5}, {"seis", 6},
        {"siete", 7}, {"ocho", 8}, {"nueve", 9}, {"diez", 10}, {"once", 11}, {"doce", 12}};
    if (t.empty()) return -1;
    if (std::isdigit(static_cast<unsigned char>(t[0]))) {
        if (t.size() > 6) return -1;  // stoi lanzaría out_of_range con números enormes («en 99999999999 dias»)
        return std::stoi(t);
    }
    auto it = m.find(t);
    return it == m.end() ? -1 : it->second;
}

// Busca `re` en w; si hay match guarda los grupos en g y BLANQUEA lo encontrado (w conserva su longitud,
// así el mapa hacia el texto original sigue siendo válido).
bool extraer(std::string& w, const std::regex& re, std::vector<std::string>& g) {
    std::smatch m;
    if (!std::regex_search(w, m, re)) return false;
    g.clear();
    for (size_t i = 0; i < m.size(); ++i) g.push_back(m[i].matched ? m[i].str() : "");
    size_t pos = static_cast<size_t>(m.position(0)), len = static_cast<size_t>(m.length(0));
    w.replace(pos, len, std::string(len, ' '));
    return true;
}

enum class Mer { Ninguno, AM, PM };

void extraerFechaHora(std::string& w, std::time_t ahora, Entidades& e) {
    static const std::regex
        reBaja(R"(\b(poco importante|prioridad baja|baja prioridad|sin prisa|no urgente)\b)"),
        reAlta(R"(\b(muy importante|importante|urgente|prioridad alta|alta prioridad)\b)"),
        reMerManana(R"(\b(de la|por la|en la|esta) manana\b)"),
        reMerTN(R"(\b(de la|por la|en la|esta) (tarde|noche)\b)"),
        reRelativo(R"(\ben (?:(\d{1,4}|una|un|dos|tres|cuatro|cinco) (horas?|minutos?)|media hora)\b)"),
        reHora(R"(\b(?:a|para) la(?:s)? (\d{1,2}|una|dos|tres|cuatro|cinco|seis|siete|ocho|nueve|diez|once|doce)(?::(\d{2}))?(?: (y media|y cuarto|menos cuarto|en punto))?\b)"),
        reNumAmPm(R"(\b(\d{1,2}) (am|pm)\b)"),
        reHoraSuelta(R"(\b(\d{1,2}):(\d{2})\b)"),
        reAmPm(R"(\b(am|pm)\b)"),
        rePasado(R"(\b(?:para )?pasado manana\b)"),
        reManana(R"(\b(?:para )?manana\b)"),
        reHoy(R"(\b(?:para )?hoy\b)"),
        reDiaSemana(R"(\b(?:para )?(?:el |este )?(?:proximo |siguiente )?(lunes|martes|miercoles|jueves|viernes|sabado|domingo)\b)"),
        reEnDias(R"(\ben (\d{1,3}|un|dos|tres|cuatro|cinco) dias?\b)"),
        reSemana(R"(\b(?:la )?(?:proxima semana|semana que viene|en una semana)\b)"),
        reFechaMes(R"(\b(?:el )?(\d{1,2}) de (enero|febrero|marzo|abril|mayo|junio|julio|agosto|septiembre|octubre|noviembre|diciembre)\b)");
    static const char* DIAS[] = {"domingo", "lunes", "martes", "miercoles", "jueves", "viernes", "sabado"};
    static const char* MESES[] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio",
                                  "agosto", "septiembre", "octubre", "noviembre", "diciembre"};

    std::vector<std::string> g;

    // Prioridad
    if (extraer(w, reBaja, g)) e.prioridad = Prioridad::Baja;
    else if (extraer(w, reAlta, g)) e.prioridad = Prioridad::Alta;

    // Parte del día ("de la tarde", "esta noche")
    Mer mer = Mer::Ninguno;
    bool fechaExplicita = false;
    int dias = 0;
    if (extraer(w, reMerManana, g)) { mer = Mer::AM; if (g[1] == "esta") fechaExplicita = true; }
    if (extraer(w, reMerTN, g)) { mer = Mer::PM; if (g[1] == "esta") fechaExplicita = true; }

    // Hora
    int h = -1, min = 0;
    bool relativo = false;
    int relMin = 0;
    if (extraer(w, reRelativo, g)) {
        relativo = true;
        if (g[0].find("media hora") != std::string::npos) relMin = 30;
        else { int n = numero(g[1]); relMin = (g[2][0] == 'h') ? n * 60 : n; }
    } else if (extraer(w, reHora, g)) {
        h = numero(g[1]);
        if (!g[2].empty()) min = std::stoi(g[2]);
        if (g[3] == "y media") min = 30;
        else if (g[3] == "y cuarto") min = 15;
        else if (g[3] == "menos cuarto") { h = (h <= 1) ? 12 : h - 1; min = 45; }
    } else if (extraer(w, reNumAmPm, g)) {
        h = std::stoi(g[1]);
        mer = (g[2] == "am") ? Mer::AM : Mer::PM;
    } else if (extraer(w, reHoraSuelta, g)) {
        h = std::stoi(g[1]);
        min = std::stoi(g[2]);
    }
    if (extraer(w, reAmPm, g)) mer = (g[1] == "am") ? Mer::AM : Mer::PM;
    if (h > 23 || min > 59) h = -1;

    if (h >= 0) {
        if (mer == Mer::AM) { if (h == 12) h = 0; }
        else if (mer == Mer::PM) { if (h < 12) h += 12; }
        else if (h >= 1 && h <= 6) { h += 12; e.horaAmbigua = true; }  // "a las 4" -> 4 PM
        else if (h >= 7 && h <= 11) { e.horaAmbigua = true; }          // "a las 10" -> 10 AM
    }

    // Fecha
    std::tm t = tmLocal(ahora);
    int mesFecha = -1, diaFecha = 0;
    if (extraer(w, rePasado, g)) { dias = 2; fechaExplicita = true; }
    else if (extraer(w, reManana, g)) { dias = 1; fechaExplicita = true; }
    else if (extraer(w, reHoy, g)) { dias = 0; fechaExplicita = true; }
    else if (extraer(w, reFechaMes, g)) {
        diaFecha = std::stoi(g[1]);
        for (int i = 0; i < 12; ++i) if (g[2] == MESES[i]) mesFecha = i;
        fechaExplicita = true;
    } else if (extraer(w, reDiaSemana, g)) {
        int objetivo = 0;
        for (int i = 0; i < 7; ++i) if (g[1] == DIAS[i]) objetivo = i;
        dias = (objetivo - t.tm_wday + 7) % 7;
        if (dias == 0) dias = 7;  // "el lunes" dicho en lunes = el siguiente
        fechaExplicita = true;
    } else if (extraer(w, reEnDias, g)) { dias = numero(g[1]); fechaExplicita = true; }
    else if (extraer(w, reSemana, g)) { dias = 7; fechaExplicita = true; }

    if (relativo) {
        e.cuando = ahora + static_cast<std::time_t>(relMin) * 60;
        e.tieneHora = true;
        e.fechaExplicita = true;
        return;
    }

    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0; t.tm_isdst = -1;
    if (mesFecha >= 0) {
        t.tm_mon = mesFecha;
        t.tm_mday = diaFecha;
        std::tm hoy0 = tmLocal(ahora);
        hoy0.tm_hour = hoy0.tm_min = hoy0.tm_sec = 0; hoy0.tm_isdst = -1;
        if (std::mktime(&t) < std::mktime(&hoy0)) { t.tm_year += 1; t.tm_isdst = -1; }
    } else {
        t.tm_mday += dias;
    }
    if (h >= 0) { t.tm_hour = h; t.tm_min = min; e.tieneHora = true; }
    t.tm_isdst = -1;
    std::time_t cuando = std::mktime(&t);
    if (!fechaExplicita && h >= 0 && cuando <= ahora) { t.tm_mday += 1; t.tm_isdst = -1; cuando = std::mktime(&t); }
    e.cuando = (fechaExplicita || h >= 0) ? cuando : 0;
    e.fechaExplicita = fechaExplicita;
}

// Reconstruye el asunto con el texto ORIGINAL (tildes y mayúsculas) de lo que sobró en `w`.
std::string construirAsunto(const texto::Normalizado& n, const std::string& w, const std::string& original,
                            bool quitarArticulos, const std::string& verboAprox = "", bool conservarMe = false) {
    struct Tok { std::string norm, orig; };
    std::vector<Tok> toks;
    for (size_t i = 0; i < w.size();) {
        if (w[i] == ' ') { ++i; continue; }
        Tok t;
        size_t j = i;
        while (j < w.size() && w[j] != ' ') {
            t.norm.push_back(w[j]);
            t.orig += original.substr(n.origen[j].first, n.origen[j].second);
            ++j;
        }
        toks.push_back(t);
        i = j;
    }
    size_t ini = 0, fin = toks.size();
    while (ini < fin && ((LIDER.count(toks[ini].norm) &&
                          !(conservarMe && (toks[ini].norm == "me" || toks[ini].norm == "mi"))) ||
                         (!verboAprox.empty() && toks[ini].norm == verboAprox) ||
                         (quitarArticulos && ARTICULOS.count(toks[ini].norm)))) ++ini;
    while (fin > ini && COLA.count(toks[fin - 1].norm)) --fin;
    std::string r;
    for (size_t i = ini; i < fin; ++i) { if (!r.empty()) r += ' '; r += toks[i].orig; }
    if (!r.empty() && r[0] >= 'a' && r[0] <= 'z') r[0] = static_cast<char>(r[0] - 'a' + 'A');
    return r;
}

// Distancia de edición con tope: devuelve > tope en cuanto se sabe que no se alcanza.
int distancia(const std::string& a, const std::string& b, int tope) {
    if (std::abs(static_cast<int>(a.size()) - static_cast<int>(b.size())) > tope) return tope + 1;
    std::vector<int> prev(b.size() + 1), cur(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) prev[j] = static_cast<int>(j);
    for (size_t i = 1; i <= a.size(); ++i) {
        cur[0] = static_cast<int>(i);
        int minimo = cur[0];
        for (size_t j = 1; j <= b.size(); ++j) {
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1)});
            minimo = std::min(minimo, cur[j]);
        }
        if (minimo > tope) return tope + 1;
        std::swap(prev, cur);
    }
    return prev[b.size()];
}

// Las expresiones regulares de std::regex son lentas en entradas largas; una orden hablada nunca necesita más.
std::string limitar(const std::string& s, size_t maxBytes = 600) {
    if (s.size() <= maxBytes) return s;
    size_t n = maxBytes;
    while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) --n;  // no partir un carácter UTF-8
    return s.substr(0, n);
}


// ───────── Closet ─────────
// Se evalúa antes que las reglas generales: «guarda mi camisa azul» no debe acabar como nota y «borra la camisa»
// no debe buscar una tarea. Cada orden exige vocabulario de prendas, así que no pisa las órdenes de agenda.
bool analizarCloset(const texto::Normalizado& n, const std::string& frase, Resultado& r) {
    const std::vector<std::string> tk = texto::dividir(n.s);
    if (tk.empty()) return false;
    const std::set<std::string> S(tk.begin(), tk.end());
    auto hay = [&](std::initializer_list<const char*> ps) {
        for (const char* p : ps) if (S.count(p)) return true;
        return false;
    };
    auto frasePide = [&](std::initializer_list<const char*> ps) {
        for (const char* p : ps) if (texto::contiene(n.s, p)) return true;
        return false;
    };

    Entidades& e = r.entidades;
    e.prenda.estilo = closet::estiloEnTexto(n.s);

    if (frasePide({"que me pongo", "que ropa uso", "que ropa me pongo", "que me visto", "que visto", "que me puedo poner",
                   "que puedo ponerme", "que debo ponerme", "que debo ponerme"}) || S.count("outfit")) {
        e.abrigo = hay({"frio", "lluvia", "llover", "lloviendo", "abrigo", "helado", "fresco"});
        r.intencion = Intencion::SugerirOutfit;
        r.puntaje = 10;
        return true;
    }
    if (hay({"tarea", "tareas", "nota", "notas", "recordatorio", "pendiente", "pendientes", "agenda"})) return false;

    const bool guardar = hay({"guarda", "guardar", "guardame", "agrega", "agregar", "agregame", "anade", "anadir",
                              "registra", "registrar", "incorpora", "incorporar"});
    const bool borrar = hay({"elimina", "eliminar", "borra", "borrar", "quita", "quitar", "saca", "sacar", "retira",
                             "retirar", "descarta", "descartar"});
    const bool palabraCloset = hay({"closet", "armario", "ropero", "guardarropa", "guardarropas", "prenda", "prendas"});

    size_t iCategoria = tk.size();
    for (size_t i = 0; i < tk.size(); ++i)
        if (!closet::categoriaDePalabra(tk[i]).empty()) { iCategoria = i; break; }
    const bool hayCategoria = iCategoria < tk.size();

    if ((hay({"vacia", "vaciar", "limpia", "limpiar"}) && palabraCloset) ||
        (borrar && hay({"todo", "todos", "todas"}) && (palabraCloset || hay({"ropa"})))) {
        r.intencion = Intencion::VaciarCloset;
        r.puntaje = 10;
        return true;
    }

    auto extraer = [&]() {
        static const std::set<std::string> estrictas = {
            "guarda", "guardar", "guardame", "agrega", "agregar", "agregame", "anade", "anadir", "registra", "registrar",
            "incorpora", "incorporar", "elimina", "eliminar", "borra", "borrar", "quita", "quitar", "saca", "sacar",
            "retira", "retirar", "descarta", "descartar", "mi", "mis", "un", "una", "unos", "unas", "el", "la", "los",
            "las", "al", "a", "en", "del", "closet", "armario", "ropero", "guardarropa", "guardarropas", "es", "son",
            "estilo", "color", "prenda", "prendas", "por", "favor", "nix", "jarvis", "oye", "hey", "quiero", "que", "y",
            "tambien", "ahora", "nueva", "nuevo", "para", "todo", "todos", "todas", "ropa"};
        // posición de cada palabra dentro del texto normalizado, para recuperar el original (con tildes y mayúsculas)
        std::vector<std::pair<size_t, size_t>> pos;
        for (size_t i = 0; i < n.s.size();) {
            size_t j = n.s.find(' ', i);
            if (j == std::string::npos) j = n.s.size();
            pos.emplace_back(i, j);
            i = j + 1;
        }
        std::vector<bool> conserva(tk.size(), false);
        for (size_t i = 0; i < tk.size(); ++i)
            conserva[i] = !estrictas.count(tk[i]) && closet::estiloDePalabra(tk[i]).empty() && tk[i] != "de" && tk[i] != "con";
        for (size_t i = 1; i + 1 < tk.size(); ++i) {  // «de» y «con» solo valen entre dos palabras que se conservan
            if (tk[i] != "de" && tk[i] != "con") continue;
            if (tk[i] == "de" && tk[i + 1] == "color") continue;
            conserva[i] = conserva[i - 1] && conserva[i + 1];
        }
        std::string nombre;
        for (size_t i = 0; i < tk.size(); ++i) {
            if (!conserva[i]) continue;
            const size_t a = pos[i].first, b = pos[i].second;
            const size_t ini = n.origen[a].first, fin = n.origen[b - 1].first + n.origen[b - 1].second;
            nombre += (nombre.empty() ? "" : " ") + frase.substr(ini, fin - ini);
        }
        e.prenda.nombre = nombre;
        for (const std::string& t : tk) {
            if (e.prenda.categoria.empty()) e.prenda.categoria = closet::categoriaDePalabra(t);
            if (e.prenda.color.empty()) e.prenda.color = closet::colorDePalabra(t);
        }
    };

    if (guardar && hayCategoria) {
        for (size_t i = 0; i + 1 < tk.size(); ++i)  // «guarda que me gusta la camisa azul» es una nota, no una prenda
            if (closet::categoriaDePalabra(tk[i]).empty() && (tk[i] == "guarda" || tk[i] == "guardar") && tk[i + 1] == "que")
                return false;
        extraer();
        r.intencion = Intencion::GuardarPrenda;
        r.puntaje = 10;
        return true;
    }
    if (borrar && (hayCategoria || palabraCloset)) {
        extraer();
        if (e.prenda.nombre.empty())
            for (const std::string& t : tk)
                if (t.size() <= 3 && std::all_of(t.begin(), t.end(), [](unsigned char c) { return std::isdigit(c); })) e.prenda.nombre = t;
        r.intencion = Intencion::EliminarPrenda;
        r.puntaje = 10;
        return true;
    }
    if (guardar) return false;

    bool misCategoria = false;
    for (size_t i = 0; i + 1 < tk.size(); ++i)
        if (tk[i] == "mis" && !closet::categoriaDePalabra(tk[i + 1]).empty()) misCategoria = true;
    if (palabraCloset || frasePide({"mi ropa", "que ropa tengo"}) || (hay({"ropa"}) && hay({"tengo"})) || misCategoria) {
        for (const std::string& t : tk) {
            if (e.prenda.categoria.empty()) e.prenda.categoria = closet::categoriaDePalabra(t);
            if (e.prenda.color.empty()) e.prenda.color = closet::colorDePalabra(t);
        }
        r.intencion = Intencion::ListarCloset;
        r.puntaje = 10;
        return true;
    }
    return false;
}

}  // namespace

Entidades Nlu::fechaDe(const std::string& entrada, std::time_t ahora) const {
    const std::string frase = limitar(entrada);
    Entidades e;
    std::string w = texto::normalizarConMapa(frase).s;
    extraerFechaHora(w, ahora, e);
    return e;
}

Resultado Nlu::analizar(const std::string& entrada, std::time_t ahora) const {
    const std::string frase = limitar(entrada);
    Resultado r;
    texto::Normalizado n = texto::normalizarConMapa(frase);
    r.normalizado = n.s;
    if (analizarCloset(n, frase, r)) return r;

    std::array<int, static_cast<size_t>(I::LimpiarAgendaHoy) + 1> puntajes{};
    for (const Regla& regla : REGLAS)
        if (texto::contiene(n.s, regla.clave)) puntajes[static_cast<size_t>(regla.intencion)] += regla.peso;
    size_t mejor = 0;
    for (size_t i = 1; i < puntajes.size(); ++i) if (puntajes[i] > puntajes[mejor]) mejor = i;

    // Rescate: si nada encajó, se tolera un error de escritura en el verbo con el que empieza la orden
    // («agendaar», «complestala»). Solo se mira la primera palabra con contenido, para no confundir
    // frases normales («mi informe completo»).
    std::string verboAprox;  // el verbo mal escrito, para no dejarlo dentro del asunto
    if (puntajes[mejor] < 2) {
        static const std::set<std::string> relleno = {"jarvis", "nix", "por", "favor", "puedes", "podrias", "oye", "hey", "quiero"};
        std::string primera;
        for (const std::string& t : texto::dividir(n.s)) if (!relleno.count(t)) { primera = t; break; }
        if (primera.size() >= 5) {
            for (const Regla& regla : REGLAS) {
                const std::string clave = regla.clave;
                if (clave.size() < 6 || clave.find(' ') != std::string::npos || clave == primera) continue;
                if (clave[0] != primera[0]) continue;
                if (distancia(primera, clave, clave.size() >= 9 ? 2 : 1) <= (clave.size() >= 9 ? 2 : 1)) {
                    puntajes[static_cast<size_t>(regla.intencion)] = std::max(puntajes[static_cast<size_t>(regla.intencion)], 2);
                    verboAprox = primera;
                }
            }
            mejor = 0;
            for (size_t i = 1; i < puntajes.size(); ++i) if (puntajes[i] > puntajes[mejor]) mejor = i;
        }
    }
    r.puntaje = puntajes[mejor];
    r.intencion = (r.puntaje >= 2) ? static_cast<Intencion>(mejor) : Intencion::Desconocida;

    switch (r.intencion) {
        case Intencion::CrearNota:  // el contenido de una nota se deja intacto (no se interpretan fechas)
            r.entidades.asunto = construirAsunto(n, n.s, frase, false, "", true);
            break;
        case Intencion::CrearTarea:
        case Intencion::Reprogramar:
        case Intencion::ListarTareas:
        case Intencion::CompletarTarea:
        case Intencion::EliminarTarea: {
            std::string w = n.s;
            extraerFechaHora(w, ahora, r.entidades);
            r.entidades.asunto = construirAsunto(n, w, frase, true, verboAprox);
            break;
        }
        default: break;
    }
    return r;
}
