// Http.hpp - Cliente HTTP(S) (libcurl). El servidor de sockets está en HttpServidor.hpp (se incluye aquí por comodidad).
#pragma once
#include "HttpServidor.hpp"

#include <atomic>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// ───────── Cliente ─────────
struct RespuestaHttp {
    long estado = 0;
    std::string cuerpo;
};

struct ErrorRed : std::runtime_error {  // no hubo conexión, tiempo agotado, TLS, etc.
    using std::runtime_error::runtime_error;
};

// metodo = "GET" | "POST". Las pruebas sustituyen esta función por una falsa.
using FuncionHttp = std::function<RespuestaHttp(const std::string& metodo, const std::string& url, const std::string& cuerpo,
                                                const std::vector<std::string>& cabeceras, int timeoutSeg)>;

RespuestaHttp httpCurl(const std::string& metodo, const std::string& url, const std::string& cuerpo,
                       const std::vector<std::string>& cabeceras, int timeoutSeg);

std::string urlCodificar(const std::string& s);  // porcentaje-codificación (RFC 3986)
