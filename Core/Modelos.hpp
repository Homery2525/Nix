#pragma once
#include <ctime>
#include <optional>
#include <string>

inline std::tm tmLocal(std::time_t t) {
    std::tm r{};
#ifdef _WIN32
    localtime_s(&r, &t);  // MSVC / MinGW
#else
    localtime_r(&t, &r);  // POSIX
#endif
    return r;
}
inline bool mismoDia(std::time_t a, std::time_t b) {
    std::tm x = tmLocal(a), y = tmLocal(b);
    return x.tm_year == y.tm_year && x.tm_yday == y.tm_yday;
}

enum class Prioridad { Baja = 1, Media = 2, Alta = 3 };

class Tarea {
public:
    Tarea() = default;
    Tarea(std::string titulo, std::time_t cuando, bool tieneHora, Prioridad prioridad, std::string estiloEvento = "");

    int id() const { return id_; }
    const std::string& titulo() const { return titulo_; }
    std::time_t cuando() const { return cuando_; }  // 0 = sin fecha
    bool tieneHora() const { return tieneHora_; }
    Prioridad prioridad() const { return prioridad_; }
    const std::string& estiloEvento() const { return estiloEvento_; }
    bool completada() const { return completada_; }

    void setId(int id) { id_ = id; }
    void completar() { completada_ = true; }
    void reabrir() { completada_ = false; }
    void reprogramar(std::time_t cuando, bool tieneHora) { cuando_ = cuando; tieneHora_ = tieneHora; }

    bool operator<(const Tarea& otra) const;

    std::string serializar() const;  // id|titulo|cuando|tieneHora|prioridad|completada|estiloEvento
    static std::optional<Tarea> deserializar(const std::string& linea);

private:
    int id_ = 0;
    std::string titulo_;
    std::time_t cuando_ = 0;
    bool tieneHora_ = false;
    Prioridad prioridad_ = Prioridad::Media;
    bool completada_ = false;
    std::string estiloEvento_;
};

class Nota {
public:
    Nota() = default;
    Nota(std::string titulo, std::string contenido, std::string categoria);

    int id() const { return id_; }
    const std::string& titulo() const { return titulo_; }
    const std::string& contenido() const { return contenido_; }
    const std::string& categoria() const { return categoria_; }
    void setId(int id) { id_ = id; }

    std::string serializar() const;  // id|titulo|categoria|contenido
    static std::optional<Nota> deserializar(const std::string& linea);

private:
    int id_ = 0;
    std::string titulo_, contenido_, categoria_;
};
