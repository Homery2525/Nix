                                                                                               
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "Asistente.hpp"
#include "Estructuras.hpp"
#include "Nlu.hpp"
#include "Repositorio.hpp"
#include "Texto.hpp"

static int fallos = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) { std::printf("  FALLO línea %d: %s\n", __LINE__, #cond); ++fallos; } \
    } while (0)

                                          
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
    {
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
        asignada = alias;
        CHECK(asignada.comoVector() == std::vector<int>({1, 5}));

        ListaEnlazada<int> movida = std::move(copia);
        CHECK(copia.vacia() && movida.comoVector() == std::vector<int>({1, 5}));
        ListaEnlazada<int> destino;
        destino.push_back(99);
        destino = std::move(movida);
        CHECK(movida.vacia() && destino.comoVector() == std::vector<int>({1, 5}));

        ListaEnlazada<int> ordenada;
        for (int valor : {4, 1, 3, 2})
            ordenada.insertarOrdenado(valor, [](int a, int b) { return a < b; });
        CHECK(ordenada.comoVector() == std::vector<int>({1, 2, 3, 4}));
        const ListaEnlazada<int>& soloLectura = ordenada;
        int suma = 0;
        for (int valor : soloLectura) suma += valor;
        CHECK(suma == 10);
    }
    {
        auto antigua = Tarea::deserializar("7|Reunión|0|0|2|0");
        CHECK(antigua && antigua->estiloEvento().empty());
        Tarea evento("Entrevista", 0, false, Prioridad::Alta, "formal");
        auto recuperada = Tarea::deserializar(evento.serializar());
        CHECK(recuperada && recuperada->estiloEvento() == "formal" && recuperada->titulo() == "Entrevista");

        Nlu nlu;
        auto nota = nlu.analizar("Anota que mi color favorito es morado", AHORA());
        CHECK(nota.intencion == Intencion::CrearNota && nota.entidades.asunto == "Mi color favorito es morado");
    }
    {
        Asistente a;
        a.procesar("Agendar reunión familiar informal mañana");
        CHECK(a.tareas().size() == 1 && a.tareas()[0].estiloEvento() == "informal");
        const std::string recuerdo = a.procesar("Mi mamá se llama Ana");
        CHECK(recuerdo.find("Ana") != std::string::npos);
        CHECK(a.notas().size() >= 1);
    }

    Nlu nlu;
    {
        auto r = nlu.analizar("Agendar reunión de Programación 3 a las 4", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea);
        CHECK(r.entidades.asunto == "Reunión de Programación 3");
        CHECK(r.entidades.tieneHora);
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 16 && t.tm_mday == 11);
    }
    {
        auto r = nlu.analizar("Recordar clase de la ULA a las 10", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea);
        CHECK(r.entidades.asunto == "Clase de la ULA");
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 10 && t.tm_mday == 12);                                       
    }
    {
        auto r = nlu.analizar("Recuérdame entregar el informe mañana a las 8 de la mañana", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea);
        CHECK(r.entidades.asunto == "Entregar el informe");
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 8 && t.tm_mday == 12 && !r.entidades.horaAmbigua);
    }
    {
        auto r = nlu.analizar("Tengo que estudiar urgente el viernes a las tres", AHORA());
        CHECK(r.intencion == Intencion::CrearTarea);
        CHECK(r.entidades.asunto == "Estudiar");
        CHECK(r.entidades.prioridad == Prioridad::Alta);
        std::tm t = tmLocal(r.entidades.cuando);
        CHECK(t.tm_hour == 15 && t.tm_mday == 16);
    }
    {
        auto r = nlu.analizar("Anota que el parcial es el viernes", AHORA());
        CHECK(r.intencion == Intencion::CrearNota);
        CHECK(r.entidades.asunto == "El parcial es el viernes");
    }
    {
        auto r = nlu.analizar("¿Qué tengo mañana?", AHORA());
        CHECK(r.intencion == Intencion::ListarTareas);
        CHECK(mismoDia(r.entidades.cuando, AHORA() + 86400));
    }
    {
        auto r = nlu.analizar("Ya terminé la tarea de matemática", AHORA());
        CHECK(r.intencion == Intencion::CompletarTarea);
        CHECK(r.entidades.asunto == "Matemática");
    }
    {
        auto r = nlu.analizar("borra la tarea 2", AHORA());
        CHECK(r.intencion == Intencion::EliminarTarea && r.entidades.asunto == "2");
    }
    CHECK(nlu.analizar("Hola Jarvis", AHORA()).intencion == Intencion::Saludo);
    CHECK(nlu.analizar("Hola Nix", AHORA()).intencion == Intencion::Saludo);
    CHECK(nlu.analizar("¿Qué hora es?", AHORA()).intencion == Intencion::FechaHora);
    CHECK(nlu.analizar("muéstrame mis notas", AHORA()).intencion == Intencion::ListarNotas);
    CHECK(nlu.analizar("limpia mi agenda de hoy", AHORA()).intencion == Intencion::LimpiarAgendaHoy);
    CHECK(nlu.analizar("quiero mi día libre", AHORA()).intencion == Intencion::LimpiarAgendaHoy);
    CHECK(nlu.analizar("elimina todas mis actividades del día", AHORA()).intencion == Intencion::LimpiarAgendaHoy);
    CHECK(nlu.analizar("borra todas las tareas de hoy", AHORA()).intencion == Intencion::LimpiarAgendaHoy);
    CHECK(nlu.analizar("blablabla", AHORA()).intencion == Intencion::Desconocida);

                                    
    const std::string dir = carpetaTemporal("test");
    {
        Asistente a(dir, "Homery");
        a.usarReloj(AHORA);
        std::string r1 = a.procesar("Agendar reunión de Programación 3 a las 4");
        std::cout << r1 << "\n";
        CHECK(r1.find("Homery") != std::string::npos);                                 
        CHECK(r1.find("Supuse que era a las 4 de la tarde") != std::string::npos);                              
        CHECK(a.procesar("hola").find("Todos los sistemas en línea") != std::string::npos);
        CHECK(a.procesar("blablabla").find("Me temo que") != std::string::npos);
        std::cout << a.procesar("Recuérdame entregar el informe mañana a las 8 de la mañana urgente") << "\n";
        std::cout << a.procesar("Recordar comprar leche") << "\n";
        std::cout << a.procesar("Anota que el parcial de cálculo es el viernes") << "\n";
        CHECK(a.notas().size() == 1 && a.notas()[0].categoria() == "Universidad");
        std::string lista = a.procesar("¿Qué tengo pendiente?");
        std::cout << lista << "\n";
        CHECK(lista.find("Tiene 3 tareas") != std::string::npos);
        CHECK(lista.find("1. Entregar el informe") != std::string::npos);                       
        std::cout << a.procesar("ya terminé lo de comprar leche") << "\n";
        CHECK(a.tareas().size() == 3 && a.tareas().back().completada());
        std::cout << a.procesar("elimina la tarea 1") << "\n";
        CHECK(a.tareas().size() == 2);
        std::cout << a.procesar("qué hora es") << "\n";
        CHECK(a.entiende("agendar reunión mañana a las 4"));
        CHECK(!a.entiende("explícame cómo funciona la fotosíntesis"));
    }
    {
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        a.procesar("Agendar paseo hoy a las 18");
        a.procesar("Agendar llamar a Ana mañana");
        a.procesar("Agendar cita hoy a las 19");
        CHECK(a.tareas().size() == 3);
        a.completarTarea(a.tareas()[0].id());

        std::string confirmacion = a.procesar("limpia mi agenda de hoy");
        CHECK(confirmacion.find("Confirma") != std::string::npos);
        CHECK(a.tareas().size() == 3);
        CHECK(a.procesar("no").find("no he cambiado") != std::string::npos);
        CHECK(a.tareas().size() == 3);

        a.procesar("limpia mi agenda de hoy");
        CHECK(a.procesar("Tal vez").find("responda afirmativamente") != std::string::npos);
        CHECK(a.tareas().size() == 3);
        CHECK(a.procesar("claro, por supuesto").find("1 tarea pendiente de hoy") != std::string::npos);
        CHECK(a.tareas().size() == 2);
        CHECK(a.procesar("deshaz").find("restaurado 1 tarea") != std::string::npos);
        CHECK(a.tareas().size() == 3);

        a.procesar("limpia mi agenda de hoy");
        std::string eliminacion = a.procesar("confirmo");
        CHECK(eliminacion.find("1 tarea pendiente de hoy") != std::string::npos);
        CHECK(a.tareas().size() == 2);
        CHECK(a.tareas()[0].completada() || a.tareas()[1].completada());
        CHECK(a.procesar("deshaz").find("restaurado 1 tarea") != std::string::npos);
        CHECK(a.tareas().size() == 3);
    }
    {
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        a.procesar("Agendar estudiar hoy");
        a.procesar("quiero mi día libre");
        CHECK(a.tareas().size() == 1);
        CHECK(a.procesar("Prefiero que no, déjalo así").find("no he cambiado") != std::string::npos);
        CHECK(a.tareas().size() == 1);
    }
    {
        Asistente b(dir, "Homery");                              
        b.usarReloj(AHORA);
        CHECK(b.tareas().size() == 2);
        CHECK(b.notas().size() == 1 && b.notas()[0].categoria() == "Universidad");
        std::cout << b.procesar("mis notas") << "\n";
    }
                                                                                                                  
    {
        const std::string dir = carpetaTemporal("ctx");
        std::time_t reloj = AHORA();
        Asistente a(dir, "señor");
        a.usarReloj([&] { return reloj; });
        auto hora = [&](size_t i) { return tmLocal(a.tareas()[i].cuando()); };

        a.procesar("Anota que me gusta vestir de azul");
        CHECK(a.notas().size() == 1 && a.notas()[0].categoria() == "Preferencias" &&
              a.notas()[0].contenido() == "Me gusta vestir de azul");

                                                           
        a.procesar("Agendar reunión de física mañana a las 10");
        std::string c = a.procesar("complétala");
        CHECK(c.find("completada") != std::string::npos && c.find("eunión de física") != std::string::npos);
        CHECK(a.tareas()[0].completada());
        CHECK(a.procesar("deshaz eso").find("vuelve a estar pendiente") != std::string::npos);
        CHECK(!a.tareas()[0].completada());

                                                                                        
        std::string m = a.procesar("cámbiala para el viernes");
        CHECK(m.find("queda para") != std::string::npos);
        CHECK(hora(0).tm_mday == 16 && hora(0).tm_hour == 10);
        a.procesar("pásala a las 3");
        CHECK(hora(0).tm_mday == 16 && hora(0).tm_hour == 15);
                                                    
        a.procesar("deshaz");
        CHECK(hora(0).tm_mday == 16 && hora(0).tm_hour == 10);
        a.procesar("deshaz");
        CHECK(hora(0).tm_mday == 12 && hora(0).tm_hour == 10);
        CHECK(a.procesar("deshaz eso").find("retirado") != std::string::npos);
        CHECK(a.tareas().empty());
        CHECK(a.procesar("deshaz").find("nada que deshacer") != std::string::npos);

                                
        a.procesar("Agendar dentista mañana a las 4 de la tarde");
        std::string s2 = a.procesar("Agendar reunión con Pedro mañana a las 4:30 de la tarde");
        CHECK(s2.find("se solapan") != std::string::npos && s2.find("Dentista") != std::string::npos);

                                                            
        int idPedro = 0;
        for (auto& t : a.tareas()) if (t.titulo().find("Pedro") != std::string::npos) idPedro = t.id();
        CHECK(a.procesar("bórrala").find("He eliminado") != std::string::npos);
        a.procesar("deshaz eso");
        bool volvio = false;
        for (auto& t : a.tareas()) if (t.id() == idPedro) volvio = true;
        CHECK(volvio);

                                                            
        CHECK(a.procesar("¿qué tengo hoy?").find("nada programado") != std::string::npos);
        std::string sig = a.procesar("¿y mañana?");
        CHECK(sig.find("Tiene 2 tareas") != std::string::npos && sig.find("Dentista") != std::string::npos);

                                                     
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        Asistente b(dir, "señor");
        std::time_t relojB = AHORA() - 2 * 86400;
        b.usarReloj([&] { return relojB; });
        b.procesar("Recordar pagar el recibo mañana a las 9 de la mañana");
        relojB = AHORA();
        CHECK(b.procesar("qué tengo pendiente").find("vencida") != std::string::npos);
        CHECK(b.procesar("resumen del día").find("vencida") != std::string::npos);
        CHECK(b.procesar("buenos días").find("1 vencida") != std::string::npos);
        std::filesystem::remove_all(dir);
    }
    {                                                            
        Nlu nlu;
        CHECK(nlu.analizar("agendaar reunión mañana a las 9", AHORA()).intencion == Intencion::CrearTarea);
        CHECK(nlu.analizar("complestala", AHORA()).intencion == Intencion::CompletarTarea);
        CHECK(nlu.analizar("agendaar llamar a mamá mañana a las 9", AHORA()).entidades.asunto == "Llamar a mamá");
        CHECK(nlu.analizar("mi informe completo", AHORA()).intencion == Intencion::Desconocida);
        CHECK(nlu.analizar("posponla para el lunes", AHORA()).intencion == Intencion::Reprogramar);
        CHECK(nlu.analizar("cancela eso", AHORA()).intencion == Intencion::Deshacer);
        CHECK(nlu.analizar("qué pasa con el clima", AHORA()).intencion == Intencion::Desconocida);
    }

                                                  
    {
        CHECK(texto::contiene("quiero mi dia libre", "dia libre"));
        CHECK(texto::contiene("hola", "hola") && texto::contiene("buenos dias amigo", "buenos dias"));
        CHECK(!texto::contiene("agendar", "agenda") && !texto::contiene("desagenda", "agenda"));                     
        CHECK(!texto::contiene("", "hola") && !texto::contiene("hola", ""));
        CHECK(texto::contiene("x agenda agendar", "agenda"));                                                      

                                                                                                  
        auto rara = Tarea::deserializar("5|Tarea|0|0|99|0");
        CHECK(rara && rara->prioridad() == Prioridad::Media);
        CHECK(!Tarea::deserializar("-3|Tarea|0|0|2|0"));
        CHECK(!Tarea::deserializar("basura sin campos"));

                                                                                          
        const std::string d = carpetaTemporal("repo");
        {
            Repositorio r(d);
            r.agregarTarea(Tarea("baja", 0, false, Prioridad::Baja));
            r.agregarTarea(Tarea("alta", 0, false, Prioridad::Alta));
            r.agregarTarea(Tarea("media", 0, false, Prioridad::Media));
            CHECK(!std::filesystem::exists(d + "/tareas.txt.tmp"));
            CHECK(r.contarPendientes() == 3);
            CHECK(r.buscarTarea("2", true).has_value() && r.obtenerTarea(*r.buscarTarea("2", true))->titulo() == "media");
            CHECK(!r.buscarTarea("9", true) && !r.buscarTarea("0", true));
        }
        {
            Repositorio r(d);                                
            auto v = r.tareas();
            CHECK(v.size() == 3 && v[0].titulo() == "alta" && v[1].titulo() == "media" && v[2].titulo() == "baja");
            CHECK(r.eliminarTarea(v[0].id()) && !r.eliminarTarea(v[0].id()));
        }
        std::filesystem::remove_all(d);

                                                         
        Nlu n2;
        bool lanzo = false;
        try { n2.analizar("agendar llamar en 99999999999 dias", AHORA()); } catch (...) { lanzo = true; }
        CHECK(!lanzo);
                                                                      
        bool largaOk = true;
        try { n2.analizar("agendar " + std::string(200000, 'a') + " mañana", AHORA()); } catch (...) { largaOk = false; }
        CHECK(largaOk);

                                                                               
        Asistente a("", "Homery");
        a.usarReloj(AHORA);
        CHECK(a.entiende("agendar reunión mañana a las 4") && a.tareas().empty());
        a.procesar("agendar reunión mañana a las 4");
        CHECK(a.tareas().size() == 1);
    }
    std::filesystem::remove_all(dir);
    std::printf(fallos ? "\n%d prueba(s) fallaron\n" : "\nTodas las pruebas pasaron\n", fallos);
    return fallos ? 1 : 0;
}
