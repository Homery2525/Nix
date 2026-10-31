#include "Modelos.hpp"

#include <exception>
#include <limits>
#include <vector>

namespace {
std::string limpiar(std::string s) {
    for (char& c : s) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return s;
}

std::vector<std::string> partir(const std::string& linea, size_t max) {
    std::vector<std::string> campos;
    size_t ini = 0;
    while (campos.size() + 1 < max) {
        size_t p = linea.find('|', ini);
        if (p == std::string::npos) break;
        campos.push_back(linea.substr(ini, p - ini));
        ini = p + 1;
    }
    campos.push_back(linea.substr(ini));
    return campos;
}
} 

Tarea::Tarea(std::string titulo, std::time_t cuando, bool tieneHora, Prioridad prioridad, std::string estiloEvento)
    : titulo_(std::move(titulo)), cuando_(cuando), tieneHora_(tieneHora), prioridad_(prioridad), estiloEvento_(std::move(estiloEvento)) {}

bool Tarea::operator<(const Tarea& o) const {
    if (completada_ != o.completada_) return !completada_;
    if (prioridad_ != o.prioridad_) return static_cast<int>(prioridad_) > static_cast<int>(o.prioridad_);
    const std::time_t inf = std::numeric_limits<std::time_t>::max();
    std::time_t a = cuando_ ? cuando_ : inf, b = o.cuando_ ? o.cuando_ : inf;
    if (a != b) return a < b;
    return id_ < o.id_;
}

std::string Tarea::serializar() const {
    return std::to_string(id_) + "|" + limpiar(titulo_) + "|" + std::to_string(static_cast<long long>(cuando_)) + "|" +
           (tieneHora_ ? "1" : "0") + "|" + std::to_string(static_cast<int>(prioridad_)) + "|" +
           (completada_ ? "1" : "0") + "|" + limpiar(estiloEvento_);
}

std::optional<Tarea> Tarea::deserializar(const std::string& linea) {
    auto c = partir(linea, 7);
    if (c.size() != 6 && c.size() != 7) return std::nullopt;
    try {
        const int prio = std::stoi(c[4]);
        const int id = std::stoi(c[0]);
        if (id < 0) return std::nullopt; 
        const Prioridad prioridad = (prio >= 1 && prio <= 3) ? static_cast<Prioridad>(prio) : Prioridad::Media;
        Tarea t(c[1], static_cast<std::time_t>(std::stoll(c[2])), c[3] == "1", prioridad, c.size() == 7 ? c[6] : "");
        t.id_ = id;
        t.completada_ = (c[5] == "1");
        return t;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

Nota::Nota(std::string titulo, std::string contenido, std::string categoria)
    : titulo_(std::move(titulo)), contenido_(std::move(contenido)), categoria_(std::move(categoria)) {}

std::string Nota::serializar() const {
    return std::to_string(id_) + "|" + limpiar(titulo_) + "|" + limpiar(categoria_) + "|" + limpiar(contenido_);
}

std::optional<Nota> Nota::deserializar(const std::string& linea) {
    auto c = partir(linea, 4);
    if (c.size() != 4) return std::nullopt;
    try {
        const int id = std::stoi(c[0]);
        if (id < 0) return std::nullopt;
        Nota n(c[1], c[3], c[2]);
        n.id_ = id;
        return n;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
