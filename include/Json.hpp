// Json.hpp - Valor JSON mínimo (lectura y escritura), sin dependencias.
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class Json {
public:
    enum class Tipo { Nulo, Bool, Numero, Texto, Lista, Objeto };

    Json() = default;
    Json(bool b) : tipo_(Tipo::Bool), b_(b) {}
    Json(int n) : tipo_(Tipo::Numero), n_(n) {}
    Json(long n) : tipo_(Tipo::Numero), n_(static_cast<double>(n)) {}
    Json(long long n) : tipo_(Tipo::Numero), n_(static_cast<double>(n)) {}
    Json(size_t n) : tipo_(Tipo::Numero), n_(static_cast<double>(n)) {}
    Json(double n) : tipo_(Tipo::Numero), n_(n) {}
    Json(const char* s) : tipo_(Tipo::Texto), s_(s) {}
    Json(std::string s) : tipo_(Tipo::Texto), s_(std::move(s)) {}

    static Json lista() { Json j; j.tipo_ = Tipo::Lista; return j; }
    static Json objeto() { Json j; j.tipo_ = Tipo::Objeto; return j; }

    Tipo tipo() const { return tipo_; }
    bool esNulo() const { return tipo_ == Tipo::Nulo; }
    bool esObjeto() const { return tipo_ == Tipo::Objeto; }
    bool esLista() const { return tipo_ == Tipo::Lista; }
    bool esTexto() const { return tipo_ == Tipo::Texto; }

    // Lectura tolerante: si falta la clave o el tipo no coincide, devuelve el valor por defecto.
    const Json& at(const std::string& clave) const;
    const Json& at(size_t i) const;
    bool contiene(const std::string& clave) const;
    size_t size() const { return tipo_ == Tipo::Lista ? lista_.size() : tipo_ == Tipo::Objeto ? valores_.size() : 0; }
    std::string texto(const std::string& def = "") const { return tipo_ == Tipo::Texto ? s_ : def; }
    double numero(double def = 0) const { return tipo_ == Tipo::Numero ? n_ : def; }
    long long entero(long long def = 0) const { return tipo_ == Tipo::Numero ? static_cast<long long>(n_) : def; }
    bool booleano(bool def = false) const { return tipo_ == Tipo::Bool ? b_ : def; }
    const std::vector<Json>& elementos() const { return lista_; }

    // Escritura
    Json& operator[](const std::string& clave);  // objeto: crea la clave si no existe
    Json& push(Json v);                          // lista: añade al final

    std::string dump() const;
    static std::optional<Json> parse(const std::string& texto);

private:
    Tipo tipo_ = Tipo::Nulo;
    bool b_ = false;
    double n_ = 0;
    std::string s_;
    std::vector<Json> lista_;
    std::vector<std::string> claves_;  // objeto: claves y valores en paralelo (conserva el orden)
    std::vector<Json> valores_;
};
