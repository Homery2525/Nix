// Servidor.hpp (hito 4) - Interfaz web + API JSON del núcleo (tareas, notas, closet) + habilidades sin IA y clima.
// El hito 5 le añade ajustes, cerebros externos y la recomendación de ropa con el closet.
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
    std::vector<std::string> hostsPermitidos;  // además de localhost
    std::function<std::time_t()> reloj;        // solo para pruebas
};

class Servidor {
public:
    explicit Servidor(ConfigServidor cfg, FuncionHttp http = httpCurl);  // las pruebas inyectan una red simulada

    RespuestaServidor manejar(const PeticionHttp& peticion);  // punto de entrada del servidor HTTP
    Json responder(const std::string& texto);                 // un mensaje del usuario -> respuesta (público para pruebas)

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
    habilidades::Memoria memHab_;   // temporizadores, último resultado…
    habilidades::Contexto ctxHab_;
    Json climaCache_;               // el clima se consulta como mucho cada 10 minutos
    std::time_t climaHora_ = 0;
    std::string climaCiudad_;
    mutable std::mutex m_;  // protege el núcleo
    std::unique_ptr<Asistente> asistente_;
};
