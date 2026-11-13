// Nlu.hpp - Comprensión básica del lenguaje: intención + entidades (asunto, fecha, hora, prioridad).
#pragma once
#include <ctime>
#include <string>

#include "Modelos.hpp"

enum class Intencion {
    Desconocida, CrearTarea, CrearNota, ListarTareas, ListarNotas,
    CompletarTarea, EliminarTarea, Saludo, FechaHora, Ayuda, Gracias,
    Reprogramar, Deshacer, Resumen, LimpiarAgendaHoy,
    // Closet: se reconocen antes que las reglas generales y por eso van detrás de LimpiarAgendaHoy.
    GuardarPrenda, ListarCloset, EliminarPrenda, VaciarCloset, SugerirOutfit
};

struct PrendaDicha {  // lo que el usuario dijo de una prenda (campos vacíos = no lo dijo)
    std::string nombre, categoria, color, estilo;
};

struct Entidades {
    std::string asunto;                       // conserva mayúsculas y tildes originales
    std::time_t cuando = 0;                   // 0 = sin fecha
    bool tieneHora = false;
    bool horaAmbigua = false;                 // se dedujo AM/PM
    bool fechaExplicita = false;              // el usuario dijo el día («mañana», «el viernes»), no solo la hora
    Prioridad prioridad = Prioridad::Media;
    PrendaDicha prenda;                       // GuardarPrenda / EliminarPrenda / ListarCloset (filtros) / SugerirOutfit (estilo)
    bool abrigo = false;                      // SugerirOutfit: la frase habla de frío o lluvia
};

struct Resultado {
    Intencion intencion = Intencion::Desconocida;
    int puntaje = 0;
    Entidades entidades;
    std::string normalizado;
};

class Nlu {
public:
    // `ahora` se inyecta para poder probar fechas relativas ("mañana", "el viernes").
    Resultado analizar(const std::string& frase, std::time_t ahora) const;
    // Solo fecha/hora de una frase suelta («¿y mañana?»), sin intención.
    Entidades fechaDe(const std::string& frase, std::time_t ahora) const;
};
