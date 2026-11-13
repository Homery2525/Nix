// Closet - Pruebas de Prenda, repositorio, NLU, conversación (guardar, listar, quitar, vaciar, deshacer) y motor de outfit.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#include "Asistente.hpp"
#include "Closet.hpp"
#include "Nlu.hpp"
#include "Outfit.hpp"
#include "Repositorio.hpp"

static int fallos = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) { std::printf("  FALLO línea %d: %s\n", __LINE__, #cond); ++fallos; } \
    } while (0)

static bool tiene(const std::string& texto, const std::string& parte) { return texto.find(parte) != std::string::npos; }

// Domingo 11-oct-2026, 15:00 (hora local, con TZ=UTC)
static std::time_t AHORA() {
    std::tm t{}; t.tm_year = 126; t.tm_mon = 9; t.tm_mday = 11; t.tm_hour = 15; t.tm_isdst = -1;
    return std::mktime(&t);
}

static std::string carpetaTemporal(const char* nombre) {
    auto dir = std::filesystem::temp_directory_path() /
               (std::string("nix_closet_") + nombre + "_" + std::to_string(std::time(nullptr)) + "_" +
                std::to_string(reinterpret_cast<std::uintptr_t>(&fallos) & 0xFFFFF));
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir.string();
}

int main() {
    setenv("TZ", "UTC", 1);
    tzset();

    {  // ── Vocabulario ──
        CHECK(closet::categoriaDePalabra("camisa") == "superior");
        CHECK(closet::categoriaDePalabra("pantalones") == "inferior");
        CHECK(closet::categoriaDePalabra("zapatos") == "calzado");
        CHECK(closet::categoriaDePalabra("chaqueta") == "abrigo");
        CHECK(closet::categoriaDePalabra("mesa").empty());
        CHECK(closet::colorDePalabra("blanca") == "blanco");
        CHECK(closet::colorDePalabra("azules") == "azul");
        CHECK(closet::colorDePalabra("rosa") == "rosado");
        CHECK(closet::estiloDePalabra("formales") == "formal");
        CHECK(closet::estiloDePalabra("casual") == "informal");
        CHECK(closet::estiloEnTexto("Es informal, por favor") == "informal");
        CHECK(closet::estiloEnTexto("sin estilo").empty());
    }

    {  // ── Prenda: serialización y validación ──
        Prenda p("Camisa azul", "superior", "azul", "formal");
        p.setId(7);
        CHECK(p.serializar() == "7|Camisa azul|superior|azul|formal");
        auto q = Prenda::deserializar(p.serializar());
        CHECK(q && q->id() == 7 && q->nombre() == "Camisa azul" && q->categoria() == "superior" && q->estilo() == "formal");
        CHECK(!Prenda::deserializar("1|Algo|sombrero|rojo|formal"));  // categoría inválida
        CHECK(!Prenda::deserializar("1|Algo|superior|rojo|elegantisimo"));  // estilo inválido
        CHECK(!Prenda::deserializar("-3|Algo|superior|rojo|formal"));
        CHECK(!Prenda::deserializar("basura"));
        Prenda con_barra("Camisa|rara\nnueva", "superior", "azul", "formal");
        CHECK(con_barra.serializar().find('\n') == std::string::npos);
    }

    {  // ── Repositorio del closet: ids, persistencia, búsqueda, restaurar ──
        const std::string dir = carpetaTemporal("repo");
        {
            Repositorio r(dir);
            Prenda a = r.agregarPrenda(Prenda("Camisa azul", "superior", "azul", "formal"));
            Prenda b = r.agregarPrenda(Prenda("Pantalón gris", "inferior", "gris", "formal"));
            Prenda c = r.agregarPrenda(Prenda("Tenis blancos", "calzado", "blanco", "informal"));
            CHECK(a.id() == 1 && b.id() == 2 && c.id() == 3);
            CHECK(r.existePrenda(Prenda("camisa AZUL", "superior", "azul", "formal")));
            CHECK(!r.existePrenda(Prenda("Camisa azul", "superior", "azul", "informal")));
            CHECK(r.buscarPrenda("pantalon").value_or(-1) == 2);
            CHECK(r.buscarPrenda("3").value_or(-1) == 3);
            CHECK(!r.buscarPrenda("9"));
            CHECK(r.buscarPrenda("la camisa azul").value_or(-1) == 1);
            CHECK(r.eliminarPrenda(2));
            CHECK(!r.eliminarPrenda(2));
        }
        {
            Repositorio r(dir);  // se vuelve a cargar desde closet.txt
            CHECK(r.prendas().size() == 2);
            CHECK(r.obtenerPrenda(3) && r.obtenerPrenda(3)->nombre() == "Tenis blancos");
            Prenda d = r.agregarPrenda(Prenda("Botas negras", "calzado", "negro", "informal"));
            CHECK(d.id() == 4);  // el id no se reutiliza tras cargar
            r.restaurarPrenda(Prenda("Pantalón gris", "inferior", "gris", "formal"));  // id 0 -> se coloca primero
            CHECK(r.vaciarCloset() == 4);
            CHECK(r.prendas().empty());
        }
        {
            Repositorio r(dir);
            CHECK(r.prendas().empty());
        }
        std::filesystem::remove_all(dir);
    }

    {  // ── Archivo de closet dañado: las líneas inválidas se descartan ──
        const std::string dir = carpetaTemporal("danado");
        FILE* f = std::fopen((dir + "/closet.txt").c_str(), "w");
        std::fputs("1|Camisa|superior|azul|formal\nbasura\n2|Sombrero|sombrero|rojo|formal\n3|Zapatos|calzado|negro|formal\n", f);
        std::fclose(f);
        Repositorio r(dir);
        CHECK(r.prendas().size() == 2);
        std::filesystem::remove_all(dir);
    }

    {  // ── NLU del closet ──
        Nlu nlu;
        auto g = nlu.analizar("Guarda mi camisa azul, es formal", AHORA());
        CHECK(g.intencion == Intencion::GuardarPrenda);
        CHECK(g.entidades.prenda.nombre == "camisa azul" && g.entidades.prenda.categoria == "superior");
        CHECK(g.entidades.prenda.color == "azul" && g.entidades.prenda.estilo == "formal");

        auto g2 = nlu.analizar("agrega al closet unos zapatos negros informales", AHORA());
        CHECK(g2.intencion == Intencion::GuardarPrenda && g2.entidades.prenda.nombre == "zapatos negros");
        CHECK(g2.entidades.prenda.categoria == "calzado" && g2.entidades.prenda.estilo == "informal");

        auto g3 = nlu.analizar("guarda mi Pantalón de lino beige", AHORA());
        CHECK(g3.intencion == Intencion::GuardarPrenda && g3.entidades.prenda.nombre == "Pantalón de lino beige");
        CHECK(g3.entidades.prenda.estilo.empty() && g3.entidades.prenda.color == "beige");

        CHECK(nlu.analizar("guarda que me gusta la camisa azul", AHORA()).intencion != Intencion::GuardarPrenda);  // no es una prenda
        CHECK(nlu.analizar("anota que mi camisa favorita es la azul", AHORA()).intencion == Intencion::CrearNota);
        CHECK(nlu.analizar("agendar reunión mañana a las 4", AHORA()).intencion == Intencion::CrearTarea);
        CHECK(nlu.analizar("borra la tarea de la camisa", AHORA()).intencion == Intencion::EliminarTarea);

        CHECK(nlu.analizar("mi closet", AHORA()).intencion == Intencion::ListarCloset);
        auto l = nlu.analizar("qué tengo de ropa informal", AHORA());
        CHECK(l.intencion == Intencion::ListarCloset && l.entidades.prenda.estilo == "informal");
        auto l2 = nlu.analizar("muéstrame mis camisas", AHORA());
        CHECK(l2.intencion == Intencion::ListarCloset && l2.entidades.prenda.categoria == "superior");
        CHECK(nlu.analizar("qué tengo pendiente hoy", AHORA()).intencion == Intencion::ListarTareas);

        auto e = nlu.analizar("quita mi camisa azul del closet", AHORA());
        CHECK(e.intencion == Intencion::EliminarPrenda && e.entidades.prenda.nombre == "camisa azul");
        auto e2 = nlu.analizar("elimina la prenda 2", AHORA());
        CHECK(e2.intencion == Intencion::EliminarPrenda && e2.entidades.prenda.nombre == "2");

        CHECK(nlu.analizar("vacía mi closet", AHORA()).intencion == Intencion::VaciarCloset);
        CHECK(nlu.analizar("borra todas mis prendas", AHORA()).intencion == Intencion::VaciarCloset);
        CHECK(nlu.analizar("limpia mi agenda", AHORA()).intencion == Intencion::LimpiarAgendaHoy ||
              nlu.analizar("limpia mi agenda", AHORA()).intencion != Intencion::VaciarCloset);

        auto o = nlu.analizar("¿qué me pongo? hace frío", AHORA());
        CHECK(o.intencion == Intencion::SugerirOutfit && o.entidades.abrigo);
        auto o2 = nlu.analizar("sugiéreme un outfit formal", AHORA());
        CHECK(o2.intencion == Intencion::SugerirOutfit && o2.entidades.prenda.estilo == "formal" && !o2.entidades.abrigo);
    }

    {  // ── Motor de outfit (función pura) ──
        std::vector<Prenda> c;
        auto alta = [&](const char* n, const char* cat, const char* col, const char* est) {
            Prenda p(n, cat, col, est);
            p.setId(static_cast<int>(c.size()) + 1);
            c.push_back(p);
        };
        alta("Camisa azul", "superior", "azul", "formal");
        alta("Camisa blanca", "superior", "blanco", "formal");
        alta("Pantalón azul", "inferior", "azul", "formal");
        alta("Pantalón gris", "inferior", "gris", "formal");
        alta("Zapatos negros", "calzado", "negro", "formal");
        alta("Tenis rojos", "calzado", "rojo", "informal");
        alta("Camiseta roja", "superior", "rojo", "informal");
        alta("Chaqueta gris", "abrigo", "gris", "formal");

        Outfit f = armarOutfit(c, "formal");
        CHECK(f.completo() && f.faltantes.empty());
        CHECK(f.superior->nombre() == "Camisa azul");            // empate -> la más antigua
        CHECK(f.inferior->nombre() == "Pantalón gris");           // no repite el azul del superior
        CHECK(f.calzado->nombre() == "Zapatos negros");
        CHECK(!f.abrigo);
        CHECK(describirOutfit(f) == "Camisa azul, Pantalón gris y Zapatos negros");

        Outfit blanca = armarOutfit(c, "formal", "blanco");       // el color favorito manda en la parte de arriba
        CHECK(blanca.superior->nombre() == "Camisa blanca");

        Outfit frio = armarOutfit(c, "formal", "", true);
        CHECK(frio.abrigo && frio.abrigo->nombre() == "Chaqueta gris");
        CHECK(tiene(describirOutfit(frio), "por encima, Chaqueta gris"));

        Outfit i = armarOutfit(c, "informal");
        CHECK(i.superior && i.superior->nombre() == "Camiseta roja" && i.calzado && !i.inferior);
        CHECK(!i.completo() && i.faltantes.size() == 1 && i.faltantes[0] == "parte de abajo informal");

        Outfit iFrio = armarOutfit(c, "informal", "", true);
        CHECK(iFrio.faltantes.size() == 2 && iFrio.faltantes[1] == "abrigo informal");

        Outfit vacio = armarOutfit({}, "formal");
        CHECK(vacio.vacio() && vacio.faltantes.size() == 3);

        Outfit sinFiltro = armarOutfit(c, "");                    // sin estilo se usan todas las prendas
        CHECK(sinFiltro.completo());
    }

    {  // ── Conversación: guardar, preguntar el estilo, duplicados, listar, quitar ──
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        CHECK(tiene(a.procesar("mi closet"), "vacío"));

        auto r = a.procesar("guarda mi camisa azul, es formal");
        CHECK(tiene(r, "Guardada") && tiene(r, "camisa azul") && tiene(r, "1 prenda"));
        CHECK(a.prendas().size() == 1 && a.prendas()[0].estilo() == "formal");

        r = a.procesar("agrega un pantalón gris");  // falta el estilo: lo pregunta
        CHECK(tiene(r, "formal o informal"));
        CHECK(a.prendas().size() == 1);
        r = a.procesar("informal");
        CHECK(tiene(r, "Guardada") && a.prendas().size() == 2 && a.prendas()[1].estilo() == "informal");

        r = a.procesar("agrega un zapato negro");   // pregunta y el usuario cambia de tema: no se guarda
        CHECK(tiene(r, "formal o informal"));
        r = a.procesar("qué hora es");
        CHECK(a.prendas().size() == 2);

        CHECK(tiene(a.procesar("guarda mi camisa azul, es formal"), "Ya tiene"));
        CHECK(a.prendas().size() == 2);

        a.procesar("guarda unos zapatos negros formales");
        r = a.procesar("mi closet");
        CHECK(tiene(r, "3 prendas") && tiene(r, "camisa azul — formal, parte de arriba"));
        r = a.procesar("qué tengo de ropa formal");
        CHECK(tiene(r, "2 prendas") && !tiene(r, "pantalón gris"));
        CHECK(tiene(a.procesar("mis camisas"), "1 prenda"));
        CHECK(tiene(a.procesar("mis abrigos"), "No tiene prendas"));

        r = a.procesar("quita el pantalón del closet");
        CHECK(tiene(r, "He retirado") && a.prendas().size() == 2);
        r = a.procesar("deshaz");
        CHECK(tiene(r, "Restaurada") && a.prendas().size() == 3);
        CHECK(tiene(a.procesar("quita el sombrero del closet"), "No encuentro"));
    }

    {  // ── «bórrala» justo después de guardar y deshacer del alta ──
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        a.procesar("guarda mi chaqueta gris informal");
        CHECK(a.prendas().size() == 1);
        auto r = a.procesar("bórrala");
        CHECK(tiene(r, "He retirado «chaqueta gris»") && a.prendas().empty());
        a.procesar("deshaz");
        CHECK(a.prendas().size() == 1);
        a.procesar("deshaz");  // deshace el alta original
        CHECK(a.prendas().empty());
        CHECK(tiene(a.procesar("deshaz"), "nada que deshacer"));
    }

    {  // ── Vaciar el closet pide confirmación y se puede deshacer ──
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        CHECK(tiene(a.procesar("vacía mi closet"), "ya está vacío"));
        a.procesar("guarda mi camisa azul formal");
        a.procesar("guarda mis tenis blancos informales");
        auto r = a.procesar("vacía mi closet");
        CHECK(tiene(r, "2 prendas") && tiene(r, "Confirma") && a.esperandoConfirmacion());
        CHECK(a.prendas().size() == 2);
        CHECK(tiene(a.procesar("quizás"), "Por favor"));
        CHECK(tiene(a.procesar("no"), "no he tocado") && !a.esperandoConfirmacion() && a.prendas().size() == 2);
        a.procesar("vacía mi closet");
        r = a.procesar("sí, confirmo");
        CHECK(tiene(r, "He vaciado") && a.prendas().empty() && !a.esperandoConfirmacion());
        r = a.procesar("deshaz");
        CHECK(tiene(r, "2 prendas") && a.prendas().size() == 2);
    }

    {  // ── Recomendación de outfit en conversación ──
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        auto r = a.procesar("qué me pongo");
        CHECK(tiene(r, "closet está vacío") && !a.ultimoOutfit());

        for (const char* f : {"guarda mi camisa azul formal", "guarda mi pantalón gris formal", "guarda mis zapatos negros formales",
                              "guarda mi chaqueta gris formal", "guarda mi camiseta roja informal", "guarda mis tenis blancos informales"})
            a.procesar(f);

        r = a.procesar("qué me pongo para una entrevista");   // la palabra «entrevista» implica formal
        CHECK(tiene(r, "algo formal") && tiene(r, "camisa azul, pantalón gris y zapatos negros"));
        CHECK(a.ultimoOutfit() && a.ultimoOutfit()->completo() && a.ultimoOutfit()->estilo == "formal");
        a.procesar("hola");
        CHECK(!a.ultimoOutfit());                             // el outfit solo acompaña a su respuesta

        r = a.procesar("qué me pongo, va a llover");           // sin pista de estilo: pregunta
        CHECK(tiene(r, "formal o informal") && !a.ultimoOutfit());
        r = a.procesar("formal");
        CHECK(tiene(r, "por encima, chaqueta gris") && a.ultimoOutfit() && a.ultimoOutfit()->abrigo);

        r = a.procesar("qué me pongo con mis amigos");         // informal: falta la parte de abajo
        CHECK(tiene(r, "algo informal") && tiene(r, "Le falta: parte de abajo informal"));

        // Un compromiso de hoy con estilo decide cuando la frase no dice nada
        a.procesar("agendar reunión con el cliente hoy a las 6 de la tarde");
        r = a.procesar("qué me pongo");
        CHECK(tiene(r, "su compromiso") && tiene(r, "(formal)"));
    }

    {  // ── El color favorito de las notas orienta la parte de arriba ──
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        a.procesar("guarda mi camisa azul formal");
        a.procesar("guarda mi camisa verde formal");
        a.procesar("guarda mi pantalón gris formal");
        a.procesar("guarda mis zapatos negros formales");
        CHECK(a.colorPreferido().empty());
        CHECK(tiene(a.procesar("qué me pongo formal"), "camisa azul"));
        a.procesar("Anota que me gusta vestir de verde");
        CHECK(a.colorPreferido() == "verde");
        CHECK(tiene(a.procesar("qué me pongo formal"), "camisa verde"));
        a.procesar("Anota que mi bebida favorita es el té verde");  // no habla de ropa: no cambia la preferencia
        a.procesar("Anota que prefiero vestir de azul");
        CHECK(a.colorPreferido() == "azul");
    }

    {  // ── Alta directa (formulario) y persistencia a través del asistente ──
        const std::string dir = carpetaTemporal("asistente");
        std::string error;
        {
            Asistente a(dir, "Homery");
            CHECK(!a.agregarPrenda("", "superior", "azul", "formal", error) && !error.empty());
            CHECK(!a.agregarPrenda("Gorra", "sombrero", "azul", "formal", error));
            CHECK(!a.agregarPrenda("Gorra", "superior", "azul", "elegante", error));
            CHECK(!a.agregarPrenda(std::string(61, 'x'), "superior", "azul", "formal", error));
            auto p = a.agregarPrenda("  Camisa lila ", "superior", "Blanca", "informal", error);
            CHECK(p && p->nombre() == "Camisa lila" && p->color() == "blanco" && p->id() == 1);
            CHECK(!a.agregarPrenda("camisa lila", "superior", "blanco", "informal", error) && tiene(error, "Ya tiene"));
            a.procesar("guarda mis botas negras informales");
        }
        {
            Asistente a(dir, "Homery");
            CHECK(a.prendas().size() == 2 && a.prendas()[1].nombre() == "botas negras");
            CHECK(a.eliminarPrenda(1) && !a.eliminarPrenda(1) && a.prendas().size() == 1);
        }
        std::filesystem::remove_all(dir);
    }

    if (fallos) {
        std::printf("\nCloset: %d prueba(s) fallaron\n", fallos);
        return 1;
    }
    std::printf("\nCloset: todas las pruebas pasaron\n");
    return 0;
}
