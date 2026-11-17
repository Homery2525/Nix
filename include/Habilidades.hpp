                                                                                    
                                                                                                                 
                                                                                                             
#pragma once
#include <ctime>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "Http.hpp"
#include "Json.hpp"

namespace habilidades {

struct Memoria {                                                          
    int iChiste = -1, iDato = -1;
    bool esperandoCiudad = false;
    int diaPendiente = 0;
    struct Wiki {
        std::string titulo;
        std::vector<std::string> oraciones;
        size_t pos = 0;
        bool activa = false;
    } wiki;
};

struct EstadoSistema {
    int tareas = 0, notas = 0, minutos = 0;
    bool nube = false;
    std::string proveedor;
};

struct Contexto {
    std::string trato = "señor", tz = "UTC", ciudad;
    std::function<std::time_t()> reloj;                                        
    std::mt19937 rng{std::random_device{}()};
    FuncionHttp http = httpCurl;
    std::function<EstadoSistema()> estado;
    Memoria* memoria = nullptr;

    std::time_t ahora() const { return reloj ? reloj() : std::time(nullptr); }
};

struct Resultado {
    std::string respuesta, habilidad;
    Json accion;                                                                                        
    std::string ajusteCiudad;                                                                     
    int probabilidadLluvia = -1;
    double temperaturaMaxima = 0;
    std::string ciudadPronostico;
};

struct Pronostico {
    int probabilidadLluvia = -1;
    double temperaturaMaxima = 0;
    std::string ciudad;
};

                                                                                                                         
struct ConsejoClima {
    bool disponible = false;                                                                               
    bool paraguas = false;
    bool abrigo = false;
    int probabilidadLluvia = -1;
    double temperaturaMaxima = 0;
    std::string ciudad;
};

std::optional<Resultado> manejar(const std::string& texto, Contexto& ctx, bool localEntiende);
std::optional<Resultado> wikipedia(const std::string& texto, Contexto& ctx);                                        
std::optional<Pronostico> pronosticoLocal(Contexto& ctx, int dia = 0);
ConsejoClima consejoClima(const Pronostico& p);                                          
ConsejoClima consejoClimaLocal(Contexto& ctx, int dia = 0);                                                                

                                         
std::string norm(const std::string& s);                                          
bool tzValida(const std::string& tz);
void fijarZona(const std::string& tz);                                                                 
std::string ahoraTexto(std::time_t t, const std::string& tz);                                           

struct FechaLocal { int anio, mes, dia, hora, minuto, diaSemana                ; };
FechaLocal fechaEn(std::time_t t, const std::string& tz);

}                          
