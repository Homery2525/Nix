// Fase 1 - Pruebas de cimientos: ListaEnlazada, Tarea/Nota (orden y serialización) y utilidades de Texto.
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

#include "Estructuras.hpp"
#include "Modelos.hpp"
#include "Texto.hpp"

static int fallos = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) { std::printf("  FALLO línea %d: %s\n", __LINE__, #cond); ++fallos; } \
    } while (0)

int main() {
    setenv("TZ", "UTC", 1);
    tzset();
    {  // ── ListaEnlazada ──
        ListaEnlazada<int> lista;
        lista.push_back(3);
        lista.push_front(1);
        lista.insert(lista.end(), 5);
        CHECK(lista.size() == 3 && lista.front() == 1 && lista.back() == 5);

        auto medio = lista.begin();
        ++medio;
        lista.erase(medio);
        CHECK(lista.comoVector() == std::vector<int>({1, 5}));

        ListaEnlazada<int> copia = lista;
        CHECK(copia.size() == 2 && copia.front() == 1 && copia.back() == 5);
        CHECK(lista.pop_front() && lista.pop_back() && lista.vacia());
        CHECK(!lista.pop_back());

        ListaEnlazada<int> asignada;
        asignada.push_back(99);
        asignada = copia;
        const ListaEnlazada<int>& alias = asignada;
        asignada = alias;  // autoasignación
        CHECK(asignada.comoVector() == std::vector<int>({1, 5}));

        ListaEnlazada<int> movida = std::move(copia);
        CHECK(copia.vacia() && movida.comoVector() == std::vector<int>({1, 5}));
        ListaEnlazada<int> destino;
        destino.push_back(99);
        destino = std::move(movida);
        CHECK(movida.vacia() && destino.comoVector() == std::vector<int>({1, 5}));

        ListaEnlazada<int> ordenada;
        for (int valor : {4, 1, 3, 2}) ordenada.insertarOrdenado(valor, [](int a, int b) { return a < b; });
        CHECK(ordenada.comoVector() == std::vector<int>({1, 2, 3, 4}));
        const ListaEnlazada<int>& soloLectura = ordenada;
        int suma = 0;
        for (int valor : soloLectura) suma += valor;
        CHECK(suma == 10);
    }
    {  // ── Tarea y Nota ──
        auto antigua = Tarea::deserializar("7|Reunión|0|0|2|0");  // formato anterior, sin estilo de evento
        CHECK(antigua && antigua->estiloEvento().empty() && antigua->id() == 7);
        Tarea evento("Entrevista", 0, false, Prioridad::Alta, "formal");
        auto recuperada = Tarea::deserializar(evento.serializar());
        CHECK(recuperada && recuperada->estiloEvento() == "formal" && recuperada->titulo() == "Entrevista");

        Tarea con_barra("a|b\nc", 0, false, Prioridad::Baja);  // los separadores no deben romper la línea
        auto r2 = Tarea::deserializar(con_barra.serializar());
        CHECK(r2 && r2->titulo() == "a b c");

        auto rara = Tarea::deserializar("5|Tarea|0|0|99|0");  // prioridad fuera de rango -> Media
        CHECK(rara && rara->prioridad() == Prioridad::Media);
        CHECK(!Tarea::deserializar("-3|Tarea|0|0|2|0"));
        CHECK(!Tarea::deserializar("basura sin campos"));

        Nota nota("Parcial", "El parcial es el viernes", "Universidad");
        auto n2 = Nota::deserializar(nota.serializar());
        CHECK(n2 && n2->contenido() == "El parcial es el viernes" && n2->categoria() == "Universidad");

        // Orden de urgencia: pendiente < completada; prioridad alta primero; fecha cercana primero; sin fecha al final
        Tarea alta("alta", 2000, false, Prioridad::Alta), baja("baja", 1000, false, Prioridad::Baja);
        Tarea sinFecha("sin fecha", 0, false, Prioridad::Alta), hecha("hecha", 10, false, Prioridad::Alta);
        hecha.completar();
        CHECK(alta < baja);
        CHECK(alta < sinFecha);
        CHECK(baja < hecha);
        CHECK(!(hecha < alta));
    }
    {  // ── Texto ──
        CHECK(texto::normalizar("¡Reunión MAÑANA, a las 4:30 p.m.!") == "reunion manana a las 4:30 pm");
        CHECK(texto::normalizar("Año nuevo 😀 ñandú") == "ano nuevo nandu");
        auto n = texto::normalizarConMapa("Café  grande");
        CHECK(n.s == "cafe grande" && n.origen.size() == n.s.size());
        CHECK(texto::dividir("uno  dos tres").size() == 3);
        CHECK(texto::contiene("quiero mi dia libre", "dia libre"));
        CHECK(!texto::contiene("agendar", "agenda") && !texto::contiene("desagenda", "agenda"));
        CHECK(!texto::contiene("", "hola") && !texto::contiene("hola", ""));
        CHECK(texto::contiene("x agenda agendar", "agenda"));
    }
    std::time_t t = 0;
    CHECK(mismoDia(t, t + 3600));
    std::printf(fallos ? "\n%d prueba(s) fallaron\n" : "\nFase 1: todas las pruebas pasaron\n", fallos);
    return fallos ? 1 : 0;
}
