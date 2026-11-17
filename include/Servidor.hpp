                                                                                                                   
                                                                                          
#pragma once
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Asistente.hpp"
#include "Habilidades.hpp"
#include "Http.hpp"
#include "Json.hpp"

struct ConfigServidor {
    std::string web = "./web", datos = "./datos", tratoInicial = "señor", ciudadInicial, zonaInicial = "UTC";
    std::vector<std::string> hostsPermitidos;                        
    std::function<std::time_t()> reloj;                            
};

class Servidor {
public:
    explicit Servidor(ConfigServidor cfg, FuncionHttp http = httpCurl);                                          

    RespuestaServidor manejar(const PeticionHttp& peticion);                                       
    Json responder(const std::string& texto);                                                                              

private:
    RespuestaServidor get(const PeticionHttp&);
    RespuestaServidor post(const PeticionHttp&);
    RespuestaServidor estatico(const std::string& ruta);
    bool hostPermitido(const PeticionHttp&) const;
    Json tareasJson() const;
    Json notasJson() const;
    Json climaJson();
    habilidades::Contexto& contextoHabilidades();

    ConfigServidor cfg_;
    FuncionHttp http_;
    std::time_t inicio_;
    std::string tz_, ciudad_;
    habilidades::Memoria memHab_;                                       
    habilidades::Contexto ctxHab_;
    Json climaCache_;                                                                 
    std::time_t climaHora_ = 0;
    std::string climaCiudad_;
    mutable std::mutex m_;                      
    std::unique_ptr<Asistente> asistente_;
};
