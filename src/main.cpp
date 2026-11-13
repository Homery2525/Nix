// Nix (hito 3): servidor web mínimo con interfaz, tareas, notas y closet. Solo sockets: no necesita libcurl.
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

}  // namespace

int main() {
    const int puerto = std::atoi(entorno("PORT", "8080").c_str());
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGTERM, alRecibirSenal);
    std::signal(SIGINT, alRecibirSenal);

    ConfigServidor cfg;
    cfg.web = entorno("JARVIS_WEB", "./web");
    cfg.datos = entorno("JARVIS_DATA", "./datos");
    cfg.tratoInicial = entorno("JARVIS_TRATO", "señor");

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
