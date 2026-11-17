                                                                                              
#include <csignal>
#include <cstdlib>
#include <iostream>

#include "Servidor.hpp"

namespace {

ServidorHttp* servidorActivo = nullptr;
void alRecibirSenal(int) { if (servidorActivo) servidorActivo->detener(); }

std::string entorno(const char* clave, const std::string& def = "") {
    const char* v = std::getenv(clave);
    return v && *v ? v : def;
}

}              

int main(int argc, char** argv) {
    const int puerto = std::atoi(entorno("PORT", "8080").c_str());

    if (argc > 1 && std::string(argv[1]) == "--salud") {                                  
        try {
            return httpCurl("GET", "http://127.0.0.1:" + std::to_string(puerto) + "/salud", "", {}, 3).estado == 200 ? 0 : 1;
        } catch (...) {
            return 1;
        }
    }
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGTERM, alRecibirSenal);
    std::signal(SIGINT, alRecibirSenal);

    ConfigServidor cfg;
    cfg.web = entorno("JARVIS_WEB", "./web");
    cfg.datos = entorno("JARVIS_DATA", "./datos");
    cfg.tratoInicial = entorno("JARVIS_TRATO", "señor");
    cfg.ciudadInicial = entorno("JARVIS_CIUDAD");
    cfg.zonaInicial = entorno("TZ", "UTC");

    Servidor app(cfg);
    ServidorHttp http(puerto, [&app](const PeticionHttp& p) { return app.manejar(p); });
    if (!http.iniciar()) {
        std::cerr << "No se pudo abrir el puerto " << puerto << "\n";
        return 1;
    }
    servidorActivo = &http;
    std::cout << "Nix en línea: http://localhost:" << http.puerto() << std::endl;
    http.ejecutar();
    return 0;
}
