// HttpServidor.hpp - Servidor HTTP mínimo (sockets POSIX), sin dependencias externas.
#pragma once
#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

// ───────── Servidor ─────────
struct PeticionHttp {
    std::string metodo, ruta, consulta, cuerpo;
    std::map<std::string, std::string> cabeceras;  // nombres en minúsculas
};

struct RespuestaServidor {
    int estado = 200;
    std::string tipo = "text/plain; charset=utf-8";
    std::string cuerpo;
    std::vector<std::pair<std::string, std::string>> extra;
};

class ServidorHttp {
public:
    using Manejador = std::function<RespuestaServidor(const PeticionHttp&)>;
    ServidorHttp(int puerto, Manejador manejador) : puerto_(puerto), manejador_(std::move(manejador)) {}
    ~ServidorHttp();

    bool iniciar();    // abre el puerto (0 = uno libre); false si falla
    void ejecutar();   // bucle de aceptación (bloquea hasta detener())
    void detener();
    int puerto() const { return puerto_; }

private:
    void atender(int cliente);
    int puerto_;
    int fd_ = -1;
    std::atomic<bool> activo_{true};
    std::atomic<int> conexiones_{0};
    Manejador manejador_;
};
