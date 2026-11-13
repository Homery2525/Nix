// Fase 2 - Pruebas de almacenamiento (Repositorio) y primer entendimiento del lenguaje (Nlu básico).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#include "Nlu.hpp"
#include "Repositorio.hpp"

static int fallos = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) { std::printf("  FALLO línea %d: %s\n", __LINE__, #cond); ++fallos; } \
    } while (0)

// Domingo 11-oct-2026, 15:00 (hora local, con TZ=UTC)
static std::time_t AHORA() {
    std::tm t{}; t.tm_year = 126; t.tm_mon = 9; t.tm_mday = 11; t.tm_hour = 15; t.tm_isdst = -1;
    return std::mktime(&t);
}

static std::string carpetaTemporal(const char* nombre) {
    auto dir = std::filesystem::temp_directory_path() /
               (std::string("nix_") + nombre + "_" + std::to_string(std::time(nullptr)) + "_" +
                std::to_string(reinterpret_cast<std::uintptr_t>(&fallos) & 0xFFFFF));
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir.string();
}

int main() {
    setenv("TZ", "UTC", 1);
    tzset();
    {  // ── Repositorio en memoria ──
        Repositorio r;
        r.agregarTarea(Tarea("baja", 0, false, Prioridad::Baja));
        r.agregarTarea(Tarea("alta", 0, false, Prioridad::Alta));
        r.agregarTarea(Tarea("media", 0, false, Prioridad::Media));
        auto v = r.tareas();
        CHECK(v.size() == 3 && v[0].titulo() == "alta" && v[1].titulo() == "media" && v[2].titulo() == "baja");
        CHECK(r.contarPendientes() == 3);
        CHECK(r.completarTarea(v[0].id()) && r.contarPendientes() == 2);
        CHECK(r.tareas().back().completada());  // las completadas pasan al final
        CHECK(r.reabrirTarea(v[0].id()) && r.tareas()[0].titulo() == "alta");
        CHECK(!r.completarTarea(999));

        auto por2 = r.buscarTarea("2", true);  // "tarea 2" = la 2.ª pendiente
        CHECK(por2 && r.obtenerTarea(*por2)->titulo() == "media");
        CHECK(!r.buscarTarea("9", true) && !r.buscarTarea("0", true));
        auto porPalabra = r.buscarTarea("la tarea baja", true);
        CHECK(porPalabra && r.obtenerTarea(*porPalabra)->titulo() == "baja");

        Tarea quitada = *r.obtenerTarea(v[1].id());
        CHECK(r.eliminarTarea(quitada.id()) && !r.eliminarTarea(quitada.id()));
        r.restaurarTarea(quitada);  // conserva el id original
        CHECK(r.obtenerTarea(quitada.id()).has_value());

        r.agregarNota(Nota("a", "primera", "General"));
        Nota b = r.agregarNota(Nota("b", "segunda", "General"));
        CHECK(r.notas().size() == 2);
        CHECK(r.eliminarNota(b.id()) && r.notas().size() == 1 && !r.eliminarNota(b.id()));
    }
    {  // ── Persistencia atómica ──
        const std::string d = carpetaTemporal("fase2");
        {
            Repositorio r(d);
            r.agregarTarea(Tarea("baja", 0, false, Prioridad::Baja));
            r.agregarTarea(Tarea("alta", 0, false, Prioridad::Alta));
            r.agregarNota(Nota("nota", "contenido", "Universidad"));
            CHECK(!std::filesystem::exists(d + "/tareas.txt.tmp"));
        }
        {
            Repositorio r(d);  // se recarga desde archivos
            auto v = r.tareas();
            CHECK(v.size() == 2 && v[0].titulo() == "alta" && v[1].titulo() == "baja");
            CHECK(r.notas().size() == 1 && r.notas()[0].categoria() == "Universidad");
            Tarea nueva = r.agregarTarea(Tarea("otra", 0, false, Prioridad::Media));
            CHECK(nueva.id() > v[0].id() && nueva.id() > v[1].id());  // los id no se repiten tras recargar
        }
        std::filesystem::remove_all(d);
    }
    Nlu nlu;
    {  // ── Nlu: intenciones y entidades básicas ──
        auto r = nlu.analizar("Agendar reunión de Programación 3 a las 4", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea);
        CHECK(r.entidades.asunto == "Reunión de Programación 3");
        CHECK(r.entidades.tieneHora);
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 16 && t.tm_mday == 11);  // «a las 4» a las 15:00 -> 4 de la tarde
    }
    {
        auto r = nlu.analizar("Recordar clase de la ULA a las 10", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea && r.entidades.asunto == "Clase de la ULA");
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 10 && t.tm_mday == 12);  // las 10 de hoy ya pasaron -> mañana
    }
    {
        auto r = nlu.analizar("Recuérdame entregar el informe mañana a las 8 de la mañana", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea && r.entidades.asunto == "Entregar el informe");
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 8 && t.tm_mday == 12 && !r.entidades.horaAmbigua);
    }
    {
        auto r = nlu.analizar("Tengo que estudiar urgente el viernes a las tres", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea && r.entidades.asunto == "Estudiar");
        CHECK(r.entidades.prioridad == Prioridad::Alta);
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 15 && t.tm_mday == 16);
    }
    {
        auto r = nlu.analizar("Anota que el parcial es el viernes", AHORA());
        CHECK(r.intencion == Intencion::CrearNota && r.entidades.asunto == "El parcial es el viernes");
    }
    {
        auto r = nlu.analizar("¿Qué tengo mañana?", AHORA());
        CHECK(r.intencion == Intencion::ListarTareas && mismoDia(r.entidades.cuando, AHORA() + 86400));
    }
    {
        auto r = nlu.analizar("Ya terminé la tarea de matemática", AHORA());
        CHECK(r.intencion == Intencion::CompletarTarea && r.entidades.asunto == "Matemática");
    }
    {
        auto r = nlu.analizar("borra la tarea 2", AHORA());
        CHECK(r.intencion == Intencion::EliminarTarea && r.entidades.asunto == "2");
    }
    CHECK(nlu.analizar("Hola Nix", AHORA()).intencion == Intencion::Saludo);
    CHECK(nlu.analizar("¿Qué hora es?", AHORA()).intencion == Intencion::FechaHora);
    CHECK(nlu.analizar("muéstrame mis notas", AHORA()).intencion == Intencion::ListarNotas);
    CHECK(nlu.analizar("blablabla", AHORA()).intencion == Intencion::Desconocida);
    {  // robustez: números enormes y órdenes gigantes no lanzan excepciones
        bool lanzo = false;
        try {
            nlu.analizar("agendar llamar en 99999999999 dias", AHORA());
            nlu.analizar("agendar " + std::string(200000, 'a') + " mañana", AHORA());
        } catch (...) { lanzo = true; }
        CHECK(!lanzo);
    }
    std::printf(fallos ? "\n%d prueba(s) fallaron\n" : "\nFase 2: todas las pruebas pasaron\n", fallos);
    return fallos ? 1 : 0;
}
