                                                                                                                         
#pragma once
#include "HttpServidor.hpp"

#include <atomic>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

                              
struct RespuestaHttp {
    long estado = 0;
    std::string cuerpo;
};

struct ErrorRed : std::runtime_error {                                                
    using std::runtime_error::runtime_error;
};

                                                                              
using FuncionHttp = std::function<RespuestaHttp(const std::string& metodo, const std::string& url, const std::string& cuerpo,
                                                const std::vector<std::string>& cabeceras, int timeoutSeg)>;

RespuestaHttp httpCurl(const std::string& metodo, const std::string& url, const std::string& cuerpo,
                       const std::vector<std::string>& cabeceras, int timeoutSeg);

std::string urlCodificar(const std::string& s);                                       
