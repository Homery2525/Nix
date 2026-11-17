#include "HttpServidor.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

                                                               

namespace {

const char* textoEstado(int e) {
    switch (e) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 413: return "Payload Too Large";
        case 503: return "Service Unavailable";
        default: return "Internal Server Error";
    }
}

std::string decodificarUrl(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() && std::isxdigit(static_cast<unsigned char>(s[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
            out += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else {
            out += s[i];
        }
    }
    return out;
}

bool enviarTodo(int fd, const std::string& datos) {
    size_t hecho = 0;
    while (hecho < datos.size()) {
        ssize_t n = send(fd, datos.data() + hecho, datos.size() - hecho, MSG_NOSIGNAL);
        if (n <= 0) return false;
        hecho += static_cast<size_t>(n);
    }
    return true;
}

void responder(int fd, const RespuestaServidor& r) {
    std::string cab = "HTTP/1.1 " + std::to_string(r.estado) + " " + textoEstado(r.estado) + "\r\nContent-Type: " + r.tipo +
                      "\r\nContent-Length: " + std::to_string(r.cuerpo.size()) +
                      "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n";
    for (const auto& [k, v] : r.extra) cab += k + ": " + v + "\r\n";
    enviarTodo(fd, cab + "\r\n" + r.cuerpo);
}

}              

ServidorHttp::~ServidorHttp() { detener(); if (fd_ >= 0) close(fd_); }

bool ServidorHttp::iniciar() {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) return false;
    int uno = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &uno, sizeof uno);
    sockaddr_in dir{};
    dir.sin_family = AF_INET;
    dir.sin_addr.s_addr = htonl(INADDR_ANY);
    dir.sin_port = htons(static_cast<uint16_t>(puerto_));
    if (bind(fd_, reinterpret_cast<sockaddr*>(&dir), sizeof dir) < 0 || listen(fd_, 64) < 0) return false;
    socklen_t largo = sizeof dir;
    getsockname(fd_, reinterpret_cast<sockaddr*>(&dir), &largo);
    puerto_ = ntohs(dir.sin_port);
    return true;
}

void ServidorHttp::detener() {
    activo_ = false;
}

void ServidorHttp::ejecutar() {
    while (activo_) {
        pollfd escucha{fd_, POLLIN, 0};
        const int listo = poll(&escucha, 1, 100);
        if (listo < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (listo == 0) continue;
        if (escucha.revents & (POLLERR | POLLHUP | POLLNVAL)) break;
        if (!(escucha.revents & POLLIN)) continue;

        int cliente = accept(fd_, nullptr, nullptr);
        if (cliente < 0) {
            if (!activo_) break;
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            continue;
        }
        if (conexiones_ >= 64) {
            responder(cliente, {503, "text/plain", "ocupado", {}});
            close(cliente);
            continue;
        }
        ++conexiones_;
        std::thread([this, cliente] {
            atender(cliente);
            close(cliente);
            --conexiones_;
        }).detach();
    }
                                                                                                               
                                                                                                               
    for (int i = 0; i < 200 && conexiones_ > 0; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void ServidorHttp::atender(int cliente) {
    timeval limite{15, 0};                                              
    setsockopt(cliente, SOL_SOCKET, SO_RCVTIMEO, &limite, sizeof limite);

    std::string datos;
    char buf[8192];
    size_t finCab = std::string::npos;
    while (finCab == std::string::npos && datos.size() < 65536) {
        ssize_t n = recv(cliente, buf, sizeof buf, 0);
        if (n <= 0) return;
        datos.append(buf, static_cast<size_t>(n));
        finCab = datos.find("\r\n\r\n");
    }
    if (finCab == std::string::npos) { responder(cliente, {400, "text/plain", "cabeceras demasiado largas", {}}); return; }

    PeticionHttp p;
    const std::string cabeceras = datos.substr(0, finCab);
    size_t pos = cabeceras.find("\r\n");
    const std::string linea = cabeceras.substr(0, pos);
    size_t s1 = linea.find(' '), s2 = linea.rfind(' ');
    if (s1 == std::string::npos || s2 <= s1) { responder(cliente, {400, "text/plain", "petición inválida", {}}); return; }
    p.metodo = linea.substr(0, s1);
    std::string destino = linea.substr(s1 + 1, s2 - s1 - 1);
    size_t q = destino.find('?');
    p.ruta = decodificarUrl(destino.substr(0, q));
    if (q != std::string::npos) p.consulta = destino.substr(q + 1);
    while (pos != std::string::npos && pos + 2 < cabeceras.size()) {
        size_t fin = cabeceras.find("\r\n", pos + 2);
        std::string l = cabeceras.substr(pos + 2, fin == std::string::npos ? std::string::npos : fin - pos - 2);
        size_t dp = l.find(':');
        if (dp != std::string::npos) {
            std::string k = l.substr(0, dp), v = l.substr(dp + 1);
            std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return std::tolower(c); });
            v.erase(0, v.find_first_not_of(" \t"));
            p.cabeceras[k] = v;
        }
        pos = fin;
    }

    size_t largo = 0;
    if (p.cabeceras.count("content-length")) {
        try { largo = static_cast<size_t>(std::stoul(p.cabeceras["content-length"])); } catch (...) { largo = 0; }
    }
    if (largo > 1000000) { responder(cliente, {413, "text/plain", "cuerpo demasiado grande", {}}); return; }
    p.cuerpo = datos.substr(finCab + 4);
    while (p.cuerpo.size() < largo) {
        ssize_t n = recv(cliente, buf, sizeof buf, 0);
        if (n <= 0) break;
        p.cuerpo.append(buf, static_cast<size_t>(n));
    }
    if (p.cuerpo.size() > largo) p.cuerpo.resize(largo);

    try {
        responder(cliente, manejador_(p));
    } catch (const std::exception&) {
        responder(cliente, {500, "text/plain", "error interno", {}});
    }
}
