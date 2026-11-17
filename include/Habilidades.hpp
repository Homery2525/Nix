// Habilidades.hpp - Habilidades de Jarvis que NO necesitan inteligencia artificial.
// Cálculo, conversiones, fechas, hora en el mundo, temporizadores, azar, charla, clima (Open-Meteo) y Wikipedia.
// Cada habilidad devuelve nullopt si la frase no es suya, así se encadenan sin pisar las órdenes del núcleo.
#pragma once
#include <ctime>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "Http.hpp"
#include "Json.hpp"

namespace habilidades {

struct Memoria {  // contexto de conversación que sobrevive entre mensajes
    int iChiste = -1, iDato = -1;
    bool esperandoCiudad = false;
    int diaPendiente = 0;
    struct Wiki {
        std::string titulo;
        std::vector<std::string> oraciones;
        size_t pos = 0;
        bool activa = false;
    } wiki;
};

struct EstadoSistema {
    int tareas = 0, notas = 0, minutos = 0;
    bool nube = false;
    std::string proveedor;
};

struct Contexto {
    std::string trato = "señor", tz = "UTC", ciudad;
    std::function<std::time_t()> reloj;          // si está vacío, la hora real
    std::mt19937 rng{std::random_device{}()};
    FuncionHttp http = httpCurl;
    std::function<EstadoSistema()> estado;
    Memoria* memoria = nullptr;

    std::time_t ahora() const { return reloj ? reloj() : std::time(nullptr); }
};

struct Resultado {
    std::string respuesta, habilidad;
    Json accion;                // p. ej. {"tipo":"temporizador","segundos":600,"etiqueta":"10 minutos"}
    std::string ajusteCiudad;   // si no está vacío, el servidor la guarda como ciudad por defecto
    int probabilidadLluvia = -1;
    double temperaturaMaxima = 0;
    std::string ciudadPronostico;
};

struct Pronostico {
    int probabilidadLluvia = -1;
    double temperaturaMaxima = 0;
    std::string ciudad;
};

// Qué conviene llevar según el pronóstico: reglas puras (paraguas desde 30 % de lluvia, abrigo si la máxima es ≤ 18 °C).
struct ConsejoClima {
    bool disponible = false;  // false = sin ciudad, sin red o sin pronóstico: no se debe inventar el clima
    bool paraguas = false;
    bool abrigo = false;
    int probabilidadLluvia = -1;
    double temperaturaMaxima = 0;
    std::string ciudad;
};

std::optional<Resultado> manejar(const std::string& texto, Contexto& ctx, bool localEntiende);
std::optional<Resultado> wikipedia(const std::string& texto, Contexto& ctx);  // «investiga X» sin cerebro conectado
std::optional<Pronostico> pronosticoLocal(Contexto& ctx, int dia = 0);
ConsejoClima consejoClima(const Pronostico& p);                  // reglas puras, sin red
ConsejoClima consejoClimaLocal(Contexto& ctx, int dia = 0);      // consulta el pronóstico y aplica las reglas; nunca lanza

// Utilidades compartidas con el servidor
std::string norm(const std::string& s);   // minúsculas, sin tildes ni puntuación
bool tzValida(const std::string& tz);
void fijarZona(const std::string& tz);    // zona horaria de todo el proceso (afecta también al núcleo)
std::string ahoraTexto(std::time_t t, const std::string& tz);  // «domingo 11 de octubre de 2026, 15:00»

struct FechaLocal { int anio, mes, dia, hora, minuto, diaSemana /* 0 = lunes */; };
FechaLocal fechaEn(std::time_t t, const std::string& tz);

}  // namespace habilidades
