// Servidor.hpp (hito 3) - Servidor mínimo: interfaz web + API JSON del núcleo (tareas, notas y closet).
// No necesita red ni claves: solo sockets. Los hitos 4 y 5 le añaden habilidades, ajustes y cerebros.
#pragma once
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Asistente.hpp"
#include "HttpServidor.hpp"
#include "Json.hpp"

struct ConfigServidor {
    std::string web = "./web", datos = "./datos", tratoInicial = "señor";
    std::vector<std::string> hostsPermitidos;  // además de localhost
    std::function<std::time_t()> reloj;        // solo para pruebas
};

class Servidor {
public:
    explicit Servidor(ConfigServidor cfg);

    RespuestaServidor manejar(const PeticionHttp& peticion);  // punto de entrada del servidor HTTP
    Json responder(const std::string& texto);                 // un mensaje del usuario -> respuesta (público para pruebas)

private:
    RespuestaServidor get(const PeticionHttp&);
    RespuestaServidor post(const PeticionHttp&);
    RespuestaServidor estatico(const std::string& ruta);
    bool hostPermitido(const PeticionHttp&) const;
    Json tareasJson() const;
    Json notasJson() const;

    ConfigServidor cfg_;
    mutable std::mutex m_;  // protege el núcleo
    std::unique_ptr<Asistente> asistente_;
};
