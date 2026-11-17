                                                                                                     
#pragma once
#include <ctime>
#include <string>

#include "Modelos.hpp"

enum class Intencion {
    Desconocida, CrearTarea, CrearNota, ListarTareas, ListarNotas,
    CompletarTarea, EliminarTarea, Saludo, FechaHora, Ayuda, Gracias,
    Reprogramar, Deshacer, Resumen, LimpiarAgendaHoy,
                                                                                                    
    GuardarPrenda, ListarCloset, EliminarPrenda, VaciarCloset, SugerirOutfit
};

struct PrendaDicha {                                                                      
    std::string nombre, categoria, color, estilo;
};

struct Entidades {
    std::string asunto;                                                                 
    std::time_t cuando = 0;                                   
    bool tieneHora = false;
    bool horaAmbigua = false;                                   
    bool fechaExplicita = false;                                                                                 
    Prioridad prioridad = Prioridad::Media;
    PrendaDicha prenda;                                                                                                          
    bool abrigo = false;                                                                       
};

struct Resultado {
    Intencion intencion = Intencion::Desconocida;
    int puntaje = 0;
    Entidades entidades;
    std::string normalizado;
};

class Nlu {
public:
                                                                                      
    Resultado analizar(const std::string& frase, std::time_t ahora) const;
                                                                         
    Entidades fechaDe(const std::string& frase, std::time_t ahora) const;
};
