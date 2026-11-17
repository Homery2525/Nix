// Pruebas del servidor: JSON, HTTP, habilidades sin IA (red simulada) y API. Se ejecutan al construir la imagen de Docker.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <iostream>
#include <set>
#include <thread>

#include "Habilidades.hpp"
#include "Http.hpp"
#include "Json.hpp"

#ifdef PRUEBAS_API
#include "Servidor.hpp"
#endif

using namespace habilidades;

static int fallos = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) { ++fallos; std::printf("  FALLO línea %d: %s\n", __LINE__, #cond); } \
    } while (0)

static bool contiene(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }

// 2026-10-11 15:00 en Caracas (UTC-4) = domingo
static std::time_t AHORA() { std::tm t{}; t.tm_year = 126; t.tm_mon = 9; t.tm_mday = 11; t.tm_hour = 19; return timegm(&t); }

static RespuestaHttp ok(const std::string& cuerpo) { return {200, cuerpo}; }

static RespuestaHttp falsoHttp(const std::string&, const std::string& url, const std::string&, const std::vector<std::string>&, int) {
    if (contiene(url, "geocoding-api")) {
        if (contiene(url, "atlantida")) return ok("{}");
        if (contiene(url, "alemania")) return ok(R"({"results":[{"name":"Berlín","timezone":"Europe/Berlin"}]})");
        if (contiene(url, "estados%20unidos")) return ok(R"({"results":[{"name":"Nueva York","timezone":"America/New_York"},{"name":"Los Ángeles","timezone":"America/Los_Angeles"}]})");
        if (contiene(url, "oslo")) return ok(R"({"results":[{"name":"Oslo","timezone":"Europe/Oslo"}]})");
        return ok(R"({"results":[{"name":"Mérida","latitude":8.59,"longitude":-71.14,"timezone":"America/Caracas"}]})");
    }
    if (contiene(url, "api.open-meteo.com"))
        return ok(R"({"current":{"temperature_2m":19.4,"apparent_temperature":18.2,"weather_code":2,"wind_speed_10m":7.6},
                      "daily":{"weather_code":[2,61,3],"temperature_2m_max":[23.1,21,22],"temperature_2m_min":[14.2,13,14],
                               "precipitation_probability_max":[40,80,10]}})");
    if (contiene(url, "action=query")) {
        if (contiene(url, "inexistente")) return ok(R"({"query":{"search":[]}})");
        return ok(contiene(url, "mercurio") ? R"({"query":{"search":[{"title":"Mercurio"}]}})" : R"({"query":{"search":[{"title":"Fotosíntesis"}]}})");
    }
    if (contiene(url, "page/summary")) {
        if (contiene(url, "Mercurio")) return ok(R"({"type":"disambiguation","extract":"Mercurio puede referirse a…"})");
        return ok(R"({"type":"standard","extract":"La fotosíntesis es un proceso (del griego photos) mediante el cual las plantas convierten la luz en energía química. Ocurre en los cloroplastos. Libera oxígeno a la atmósfera. Es esencial para la vida en la Tierra."})");
    }
    throw ErrorRed("URL inesperada: " + url);
}

static RespuestaHttp roto(const std::string&, const std::string&, const std::string&, const std::vector<std::string>&, int) {
    throw ErrorRed("sin red");
}

struct Banco {
    Memoria mem;
    Contexto ctx;
    explicit Banco(FuncionHttp http = falsoHttp, const std::string& ciudad = "") {
        ctx.tz = "America/Caracas";
        ctx.reloj = AHORA;
        ctx.rng.seed(7);
        ctx.http = http;
        ctx.ciudad = ciudad;
        ctx.memoria = &mem;
        ctx.estado = [] { EstadoSistema e; e.tareas = 3; e.notas = 1; e.minutos = 12; return e; };
    }
    std::string di(const std::string& t, bool entiende = false) {
        auto r = manejar(t, ctx, entiende);
        return r ? r->respuesta : "<nada>";
    }
    std::optional<habilidades::Resultado> raw(const std::string& t) { return manejar(t, ctx, false); }
};


#ifdef PRUEBAS_API
// ── API completa del servidor (sin red real): seguridad, enrutado, persistencia, calendario y cerebros ──
static int probarApi() {
    const int antes = fallos;
    namespace fs = std::filesystem;
    const fs::path base = fs::temp_directory_path() / "jarvis_test_api";
    fs::remove_all(base);
    fs::create_directories(base / "web");
    fs::create_directories(base / "datos");
    { std::ofstream(base / "web" / "index.html") << "<html>jarvis</html>"; std::ofstream(base / "secreto.txt") << "secreto"; }

    using Red = std::function<RespuestaHttp(const std::string&, const std::string&, const std::vector<std::string>&)>;
    Red red = [](const std::string& url, const std::string& cuerpo, const std::vector<std::string>&) { return falsoHttp("GET", url, cuerpo, {}, 0); };
    FuncionHttp http = [&](const std::string&, const std::string& url, const std::string& cuerpo, const std::vector<std::string>& cab, int) {
        return red(url, cuerpo, cab);
    };

    ConfigServidor cfg;
    cfg.web = (base / "web").string();
    cfg.datos = (base / "datos").string();
    cfg.claveClaude = "clave-claude";
    cfg.claveGemini = "clave-gemini";
    cfg.zonaInicial = "America/Caracas";
    cfg.reloj = AHORA;
    auto app = std::make_unique<Servidor>(cfg, http);

    auto peticion = [&](const std::string& metodo, const std::string& ruta, const std::string& cuerpo = "",
                        std::map<std::string, std::string> cab = {}) {
        PeticionHttp p;
        p.metodo = metodo; p.ruta = ruta; p.cuerpo = cuerpo;
        p.cabeceras = {{"host", "localhost:8080"}};
        if (metodo == "POST") p.cabeceras["x-jarvis"] = "1";
        for (const auto& [k, v] : cab) { if (v.empty()) p.cabeceras.erase(k); else p.cabeceras[k] = v; }
        return p;
    };
    auto msg = [&](const std::string& t) {
        Json j = Json::objeto();
        j["texto"] = t;
        return *Json::parse(app->manejar(peticion("POST", "/api/mensaje", j.dump())).cuerpo);
    };
    auto ajustar = [&](const std::string& json) { return app->manejar(peticion("POST", "/api/ajustes", json)); };

    {
        ConfigServidor cfgConfirm = cfg;
        cfgConfirm.datos = (base / "datos-confirmacion").string();
        fs::create_directories(cfgConfirm.datos);
        Servidor confirmApp(cfgConfirm, http);
        auto confirmar = [&](const std::string& texto) { return confirmApp.responder(texto); };
        auto inicio = confirmar("Agendar tarea de prueba hoy");
        CHECK(inicio.at("origen").texto() == "local");
        auto pregunta = confirmar("Quiero mi día libre");
        CHECK(pregunta.at("origen").texto() == "local" &&
              contiene(pregunta.at("respuesta").texto(), "Confirma"));
        auto resultado = confirmar("Confirmo");
        CHECK(resultado.at("origen").texto() == "local" &&
              contiene(resultado.at("respuesta").texto(), "1 tarea pendiente de hoy"));
    }
    {
        ConfigServidor cfgPersonal = cfg;
        cfgPersonal.datos = (base / "datos-personales").string();
        fs::create_directories(cfgPersonal.datos);
        Servidor personal(cfgPersonal, http);
        auto decir = [&](const std::string& texto) { return personal.responder(texto); };
        decir("Mi color favorito es morado");
        decir("Mi bebida favorita es el mate");
        decir("Mi actividad favorita es leer");
        auto preferencias = decir("¿Cuáles son mis cosas favoritas?");
        CHECK(preferencias.at("origen").texto() == "local" &&
              contiene(preferencias.at("respuesta").texto(), "color favorito es morado") &&
              contiene(preferencias.at("respuesta").texto(), "bebida favorita es el mate") &&
              contiene(preferencias.at("respuesta").texto(), "actividad favorita es leer"));
        auto consultaGusto = decir("¿Qué actividades favoritas tengo?");
        CHECK(consultaGusto.at("origen").texto() == "local" &&
              contiene(consultaGusto.at("respuesta").texto(), "actividad favorita es leer") &&
              !contiene(consultaGusto.at("respuesta").texto(), "bebida favorita") &&
              !contiene(consultaGusto.at("respuesta").texto(), "color favorito"));
        consultaGusto = decir("¿Qué me gusta hacer?");
        CHECK(consultaGusto.at("origen").texto() == "local" &&
              contiene(consultaGusto.at("respuesta").texto(), "actividad favorita es leer") &&
              !contiene(consultaGusto.at("respuesta").texto(), "bebida favorita"));
        consultaGusto = decir("¿Cuáles son mis actividades favoritas?");
        CHECK(consultaGusto.at("origen").texto() == "local" &&
              contiene(consultaGusto.at("respuesta").texto(), "actividad favorita es leer"));
        consultaGusto = decir("¿Qué bebidas prefiero?");
        CHECK(consultaGusto.at("origen").texto() == "local" &&
              contiene(consultaGusto.at("respuesta").texto(), "bebida favorita es el mate") &&
              !contiene(consultaGusto.at("respuesta").texto(), "actividad favorita") &&
              !contiene(consultaGusto.at("respuesta").texto(), "color favorito"));
        decir("Mi comida favorita es la pasta");
        consultaGusto = decir("¿Qué comida me gusta?");
        CHECK(consultaGusto.at("origen").texto() == "local" &&
              contiene(consultaGusto.at("respuesta").texto(), "comida favorita es la pasta") &&
              !contiene(consultaGusto.at("respuesta").texto(), "bebida favorita") &&
              !contiene(consultaGusto.at("respuesta").texto(), "actividad favorita"));
        PeticionHttp notas;
        notas.metodo = "GET"; notas.ruta = "/api/notas"; notas.cabeceras = {{"host", "localhost:8080"}};
        const auto preferenciasEnNotas = *Json::parse(personal.manejar(notas).cuerpo);
        CHECK(contiene(preferenciasEnNotas.dump(), "\"categoria\":\"Preferencias\"") &&
              contiene(preferenciasEnNotas.dump(), "Mi color favorito es morado"));
        auto persona = decir("Mi mejor amigo se llama Diego");
        CHECK(contiene(persona.at("respuesta").texto(), "Diego"));
        persona = decir("Nix, mi hermana es Valeria");
        CHECK(contiene(persona.at("respuesta").texto(), "Valeria"));
        persona = decir("Mi mejor amigo es Mateo");
        CHECK(contiene(persona.at("respuesta").texto(), "Mateo"));
        decir("Mi amigo se llama Ana");
        decir("Mi amigo se llama Luis");
        decir("Oye Nix, mi amiga es Sofía");
        decir("Mi mamá se llama Elena");
        decir("Mi hermano se llama Pablo");
        decir("Mi papá se llama Roberto");
        auto nombres = decir("¿Cómo se llama mi mamá?");
        CHECK(contiene(nombres.at("respuesta").texto(), "Elena"));
        nombres = decir("¿Cómo se llama mi hermano y mi hermana?");
        CHECK(contiene(nombres.at("respuesta").texto(), "Pablo") &&
              contiene(nombres.at("respuesta").texto(), "Valeria"));
        nombres = decir("¿Cómo se llaman mis amigos?");
        CHECK(contiene(nombres.at("respuesta").texto(), "Ana") &&
              contiene(nombres.at("respuesta").texto(), "Luis") &&
              contiene(nombres.at("respuesta").texto(), "Sofía"));
        nombres = decir("¿Cómo se llaman mis mejores amigos?");
        CHECK(contiene(nombres.at("respuesta").texto(), "Mateo"));
        nombres = decir("¿Cómo se llama mi papá?");
        CHECK(contiene(nombres.at("respuesta").texto(), "Roberto"));
        auto regalo = decir("¿Qué regalo le recomiendas a mi mejor amigo?");
        CHECK(contiene(regalo.at("respuesta").texto(), "Mateo") &&
              contiene(regalo.at("respuesta").texto(), "Qué le gusta"));
        regalo = decir("¿Qué regalo le recomiendas a mi amigo?");
        CHECK(contiene(regalo.at("respuesta").texto(), "Ana") &&
              contiene(regalo.at("respuesta").texto(), "Luis"));
        auto ropa = decir("¿Qué me recomiendas ponerme para una reunión de trabajo?");
        CHECK(contiene(ropa.at("respuesta").texto(), "camisa morada"));
        auto bebida = decir("Quiero conversar");
        CHECK(contiene(bebida.at("respuesta").texto(), "mate"));
        bebida = decir("Me gustaría salir a beber algo, ¿qué me recomiendas?");
        CHECK(bebida.at("origen").texto() == "local" &&
              contiene(bebida.at("respuesta").texto(), "café tranquilo") &&
              contiene(bebida.at("respuesta").texto(), "mate"));
        auto actividad = decir("Me gustaría hacer algo hoy");
        CHECK(actividad.at("origen").texto() == "local" &&
              contiene(actividad.at("respuesta").texto(), "leer un rato") &&
              contiene(actividad.at("respuesta").texto(), "actividad que ya disfruta"));
        auto animo = decir("Hoy tuve un día terrible");
        CHECK(contiene(animo.at("respuesta").texto(), "leer un rato") &&
              contiene(animo.at("respuesta").texto(), "Cuál le apetece"));
        auto eleccion = decir("la primera");
        CHECK(contiene(eleccion.at("respuesta").texto(), "qué hora le gustaría"));
        auto hora = decir("mañana a las 7");
        CHECK(contiene(hora.at("respuesta").texto(), "agendado") &&
              contiene(hora.at("respuesta").texto(), "Leer un rato"));
        PeticionHttp agenda;
        agenda.metodo = "GET"; agenda.ruta = "/api/tareas"; agenda.cabeceras = {{"host", "localhost:8080"}};
        const auto tareasPersonales = *Json::parse(personal.manejar(agenda).cuerpo);
        CHECK(tareasPersonales.at("tareas").size() == 1 &&
              tareasPersonales.at("tareas").at(0).at("titulo").texto() == "Leer un rato" &&
              tareasPersonales.at("tareas").at(0).at("tieneHora").booleano());
        Servidor personalReiniciado(cfgPersonal, http);
        auto gustosPersistentes = personalReiniciado.responder("¿Qué recuerdas de mis gustos?");
        CHECK(gustosPersistentes.at("origen").texto() == "local" &&
              contiene(gustosPersistentes.at("respuesta").texto(), "color favorito es morado") &&
              contiene(gustosPersistentes.at("respuesta").texto(), "bebida favorita es el mate") &&
              contiene(gustosPersistentes.at("respuesta").texto(), "actividad favorita es leer"));
        auto recomendacionPersistente = personalReiniciado.responder("¿Qué me recomiendas ponerme para una reunión?");
        CHECK(contiene(recomendacionPersistente.at("respuesta").texto(), "camisa morada"));
        CHECK(contiene(personalReiniciado.responder("¿Qué regalo le recomiendas a mi mejor amigo?")
                           .at("respuesta").texto(), "Mateo"));
        CHECK(contiene(personalReiniciado.responder("¿Qué regalo le recomiendas a mi amigo?")
                           .at("respuesta").texto(), "Ana") &&
              contiene(personalReiniciado.responder("¿Qué regalo le recomiendas a mi amigo?")
                           .at("respuesta").texto(), "Luis"));
        CHECK(contiene(personalReiniciado.responder("¿Cómo se llaman mis amigos?")
                           .at("respuesta").texto(), "Sofía"));
        const auto notasPersonalesPersistentes =
            *Json::parse(personalReiniciado.manejar(notas).cuerpo);
        CHECK(contiene(notasPersonalesPersistentes.dump(), "Valeria") &&
              contiene(notasPersonalesPersistentes.dump(), "Sofía"));
    }
    {
        ConfigServidor cfgSinActividad = cfg;
        cfgSinActividad.datos = (base / "datos-sin-actividad").string();
        fs::create_directories(cfgSinActividad.datos);
        Servidor sinActividad(cfgSinActividad, http);
        auto decir = [&](const std::string& texto) { return sinActividad.responder(texto); };
        CHECK(contiene(decir("¿Cuáles son mis gustos?").at("respuesta").texto(),
                       "Aún no tengo preferencias guardadas"));
        CHECK(contiene(decir("Hoy no ha sido mi día").at("respuesta").texto(), "no tengo guardada"));
        CHECK(contiene(decir("Me gusta ir al parque").at("respuesta").texto(), "paseo por el parque"));
        CHECK(contiene(decir("la primera").at("respuesta").texto(), "qué hora le gustaría"));
        auto agenda = decir("hoy a las 6");
        CHECK(contiene(agenda.at("respuesta").texto(), "agendado «Dar un paseo por el parque»"));
    }
    // Seguridad
    CHECK(app->manejar(peticion("GET", "/api/tareas", "", {{"host", "evil.example.com"}})).estado == 403);
    CHECK(app->manejar(peticion("POST", "/api/mensaje", "{}", {{"x-jarvis", ""}})).estado == 403);
    CHECK(app->manejar(peticion("GET", "/../secreto.txt")).estado == 404);
    CHECK(app->manejar(peticion("GET", "//etc/passwd")).estado == 404);
    CHECK(app->manejar(peticion("GET", "/%2e%2e/secreto.txt")).estado == 404);
    CHECK(app->manejar(peticion("POST", "/api/mensaje", "{no es json")).estado == 400);
    CHECK(app->manejar(peticion("GET", "/api/no-existe")).estado == 404);
    CHECK(app->manejar(peticion("GET", "/salud")).cuerpo == "ok");
    { auto r = app->manejar(peticion("GET", "/")); CHECK(r.estado == 200 && contiene(r.tipo, "text/html") && contiene(r.cuerpo, "jarvis")); }

    // Habilidades, núcleo y memoria de conversación (modo sin IA)
    CHECK(contiene(ajustar("{\"proveedor\":\"local\"}").cuerpo, "\"proveedor\":\"local\""));
    CHECK(contiene(msg("hola Jarvis").at("respuesta").texto(), "Todos los sistemas en línea"));
    Json r = msg("cuánto es quince por ocho");
    CHECK(r.at("origen").texto() == "habilidad" && contiene(r.at("respuesta").texto(), "El resultado es 120"));
    Contexto ctxRaiz;
    ctxRaiz.tz = "UTC";
    ctxRaiz.reloj = [] { return 0; };
    auto raiz = manejar("cuánto es la raíz cúbica de -8", ctxRaiz, false);
    CHECK(raiz && contiene(raiz->respuesta, "El resultado es -2"));
    Contexto ctxRaizNeg;
    ctxRaizNeg.tz = "UTC";
    ctxRaizNeg.reloj = [] { return 0; };
    CHECK(!manejar("cuánto es la raíz cuadrada de -9", ctxRaizNeg, false));
    r = msg("pon un temporizador de 10 minutos");
    CHECK(r.at("accion").at("tipo").texto() == "temporizador" && r.at("accion").at("segundos").entero() == 600);
    r = msg("Agendar reunión de Programación 3 mañana a las 4");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Programación 3") &&
          contiene(r.at("respuesta").texto(), "Evento formal"));
    auto tareasIniciales = *Json::parse(app->manejar(peticion("GET", "/api/tareas")).cuerpo);
    CHECK(tareasIniciales.at("tareas").at(0).at("estiloEvento").texto() == "formal");
    CHECK(contiene(msg("complétala").at("respuesta").texto(), "completada"));
    CHECK(contiene(msg("deshaz eso").at("respuesta").texto(), "vuelve a estar pendiente"));
    CHECK(contiene(msg("blablabla").at("respuesta").texto(), "no he comprendido"));
    CHECK(contiene(msg("qué es la fotosíntesis").at("respuesta").texto(), "Según Wikipedia"));
    CHECK(contiene(msg("investiga sobre la fotosíntesis").at("respuesta").texto(), "Según Wikipedia"));
    msg("Anota que me gusta vestir de azul y prefiero zapatos cómodos");
    const auto notasPreferidas = *Json::parse(app->manejar(peticion("GET", "/api/notas")).cuerpo);
    CHECK(contiene(notasPreferidas.dump(), "Me gusta vestir de azul y prefiero zapatos cómodos"));
    r = msg("Oye Nix, me gusta la fotografía");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Preferencias"));
    r = msg("Nix, me gustan los rompecabezas");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Preferencias"));
    r = msg("Hey, Nix, me fascina caminar");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Preferencias"));
    const auto preferenciasConInvocacion = *Json::parse(app->manejar(peticion("GET", "/api/notas")).cuerpo);
    CHECK(contiene(preferenciasConInvocacion.dump(), "Me gusta la fotografía") &&
          contiene(preferenciasConInvocacion.dump(), "Me gustan los rompecabezas") &&
          contiene(preferenciasConInvocacion.dump(), "Me fascina caminar") &&
          contiene(preferenciasConInvocacion.dump(), "\"categoria\":\"Preferencias\""));
    r = msg("Quisiera hacer algo hoy");
    CHECK(r.at("origen").texto() == "local" &&
          contiene(r.at("respuesta").texto(), "salir a caminar") &&
          contiene(r.at("respuesta").texto(), "actividad que ya disfruta"));
    msg("Anota que mi comida favorita es la pasta");
    msg("Anota que mi bebida favorita es el té");
    msg("Anota que el proyecto confidencial usa clave alfa");
    ajustar("{\"ciudad\":\"Mérida\"}");
    r = msg("Quiero comer");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "restaurante de pasta") &&
          contiene(r.at("respuesta").texto(), "40%") && contiene(r.at("respuesta").texto(), "paraguas"));
    r = msg("Me gustaría salir a beber algo, ¿qué me recomiendas?");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "café tranquilo") &&
          contiene(r.at("respuesta").texto(), "té"));
    r = msg("Quisiera hacer algo hoy");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "salir a caminar") &&
          contiene(r.at("respuesta").texto(), "actividad que ya disfruta"));
    r = msg("Quiero conversar con mi pareja");
    CHECK(contiene(r.at("respuesta").texto(), "café tranquilo") && contiene(r.at("respuesta").texto(), "té"));
    r = msg("Quiero salir a pasear");
    CHECK(contiene(r.at("respuesta").texto(), "parque") && contiene(r.at("respuesta").texto(), "40%") &&
          contiene(r.at("respuesta").texto(), "paraguas"));
    r = msg("¿Qué me recomiendas ponerme para una reunión de trabajo?");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "camisa azul") &&
          contiene(r.at("respuesta").texto(), "zapatos cómodos") && contiene(r.at("respuesta").texto(), "conjunto sobrio") &&
          !contiene(r.at("respuesta").texto(), "Me gusta vestir") && !contiene(r.at("respuesta").texto(), "prefiero zapatos"));
    r = msg("¿Qué me recomiendas ponerme para una cena familiar?");
    CHECK(contiene(r.at("respuesta").texto(), "qué estilo o prenda prefiere"));
    r = msg("Será una cena informal en casa");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "camiseta azul") &&
          contiene(r.at("respuesta").texto(), "calzado cómodo"));
    r = msg("¿Qué me recomiendas ponerme para salir con mis amigos?");
    CHECK(contiene(r.at("respuesta").texto(), "¿Qué plan tienen"));
    r = msg("Iremos a un bar");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "camiseta azul") &&
          contiene(r.at("respuesta").texto(), "plan informal"));
    r = msg("¿Qué me recomiendas ponerme?");
    CHECK(contiene(r.at("respuesta").texto(), "Qué plan tienen"));
    r = msg("Me gusta vestir de verde y prefiero zapatos cómodos");
    CHECK(contiene(r.at("respuesta").texto(), "Qué plan tienen"));
    r = msg("Iré a una boda");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "camisa verde") &&
          contiene(r.at("respuesta").texto(), "zapatos cómodos"));
    r = msg("Me gusta vestir de rojo");
    CHECK(contiene(r.at("respuesta").texto(), "Preferencias"));
    r = msg("¿Qué me recomiendas ponerme para una reunión de trabajo?");
    CHECK(contiene(r.at("respuesta").texto(), "camisa roja"));
    r = msg("¿Qué me recomiendas hacer?");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Programación 3") &&
          contiene(r.at("respuesta").texto(), "próximo compromiso"));
    r = msg("¿Qué plan me recomiendas hacer con mis amigos?");
    CHECK(!contiene(r.at("respuesta").texto(), "Programación 3") && contiene(r.at("respuesta").texto(), "Qué tipo de plan"));
    r = msg("Preferimos comer");
    CHECK(contiene(r.at("respuesta").texto(), "restaurante"));
    r = msg("¿Me conviene aceptar un trabajo nuevo?");
    CHECK(contiene(r.at("respuesta").texto(), "¿Qué opciones considera"));
    r = msg("Me importa la estabilidad");
    CHECK(contiene(r.at("respuesta").texto(), "prioriza la estabilidad"));
    r = msg("Una oferta es estable y otra paga más, ¿debería elegir la estable?");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "opción más estable"));
    msg("¿Cuál me conviene?");
    r = msg("Me importa el precio");
    CHECK(contiene(r.at("respuesta").texto(), "Describa las dos alternativas"));
    r = msg("Una bicicleta cuesta 300 y otra cuesta 220");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "la segunda") &&
          contiene(r.at("respuesta").texto(), "precio"));
    r = msg("Compara una bicicleta que cuesta 300 frente a otra que cuesta 220; priorizo el precio");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "la segunda") &&
          contiene(r.at("respuesta").texto(), "precio"));
    r = msg("Compara una cena en restaurante que cuesta 40 frente a otra que cuesta 25; priorizo el precio");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "la segunda") &&
          contiene(r.at("respuesta").texto(), "precio"));
    msg("¿Qué opción de transporte me conviene?");
    msg("Me importa la comodidad");
    r = msg("Una bicicleta es incómoda y la otra es cómoda");
    CHECK(contiene(r.at("respuesta").texto(), "la segunda") &&
          contiene(r.at("respuesta").texto(), "comodidad"));
    msg("¿Cuál me conviene?");
    msg("Me importa la compatibilidad");
    r = msg("Opción A tiene más compatibilidad y opción B es más barata");
    CHECK(contiene(r.at("respuesta").texto(), "la primera") &&
          contiene(r.at("respuesta").texto(), "compatibilidad"));
    msg("¿Me conviene cambiar de trabajo?");
    r = msg("ayuda");
    CHECK(contiene(r.at("respuesta").texto(), "Puedo gestionar tareas y notas"));
    r = msg("Me importa la estabilidad");
    CHECK(!contiene(r.at("respuesta").texto(), "prioriza la estabilidad"));
    r = msg("Mi mamá se llama Ana");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Ana"));
    auto notasPersonales = *Json::parse(app->manejar(peticion("GET", "/api/notas")).cuerpo);
    CHECK(contiene(notasPersonales.dump(), "Contexto personal") && contiene(notasPersonales.dump(), "Mi mamá se llama Ana"));
    r = msg("¿Qué regalo le recomiendas a mi mamá?");
    CHECK(contiene(r.at("respuesta").texto(), "Ana") && contiene(r.at("respuesta").texto(), "Qué le gusta"));
    r = msg("Le gustan los libros");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Si disfruta la lectura"));

    // Datos, zona horaria y calendario
    auto tareas = *Json::parse(app->manejar(peticion("GET", "/api/tareas")).cuerpo);
    CHECK(tareas.at("tareas").size() == 1 && !tareas.at("tareas").at(0).at("completada").booleano());
    const int id = static_cast<int>(tareas.at("tareas").at(0).at("id").entero());
    auto ics = app->manejar(peticion("GET", "/api/calendario.ics"));
    CHECK(contiene(ics.cuerpo, "BEGIN:VEVENT") && contiene(ics.cuerpo, "DTSTART:20261012T200000Z") &&
          contiene(ics.cuerpo, "CATEGORIES:formal") && contiene(ics.cuerpo, "TRIGGER:-PT10M"));
    CHECK(contiene(ics.tipo, "text/calendar"));
    CHECK(contiene(app->manejar(peticion("POST", "/api/tarea/completar", "{\"id\":" + std::to_string(id) + "}")).cuerpo, "true"));
    CHECK(contiene(app->manejar(peticion("POST", "/api/tarea/completar", "{\"id\":9999}")).cuerpo, "false"));
    CHECK(!contiene(app->manejar(peticion("GET", "/api/calendario.ics")).cuerpo, "BEGIN:VEVENT"));
    CHECK(contiene(ajustar("{\"tz\":\"Asia/Tokyo\"}").cuerpo, "Asia/Tokyo"));
    CHECK(contiene(ajustar("{\"tz\":\"../../etc/passwd\"}").cuerpo, "Asia/Tokyo"));  // zona inválida: se ignora
    ajustar("{\"tz\":\"America/Caracas\"}");

    // Nombre y ajustes persistentes
    CHECK(contiene(msg("Jarvis, llámame Daniel").at("respuesta").texto(), "Daniel"));
    CHECK(contiene(ajustar("{}").cuerpo, "\"trato\":\"Daniel\""));
    CHECK(contiene(msg("gracias").at("respuesta").texto(), "Daniel"));
    CHECK(contiene(msg("ok").at("respuesta").texto(), "Hasta pronto"));
    CHECK(contiene(msg("gracias Nix").at("respuesta").texto(), "Hasta pronto"));
    CHECK(contiene(msg("Muchas gracias, Nix").at("respuesta").texto(), "Hasta pronto"));
    CHECK(contiene(msg("te lo agradezco").at("respuesta").texto(), "Hasta pronto"));
    CHECK(contiene(msg("vale").at("respuesta").texto(), "Hasta pronto"));
    CHECK(contiene(msg("ok, ¿qué hora es?").at("respuesta").texto(), "Son las"));
    ajustar("{\"ciudad\":\"Mérida\",\"proveedor\":\"inventado\",\"modeloOllama\":\"../x\"}");
    app.reset();
    app = std::make_unique<Servidor>(cfg, http);  // reinicio: se recupera todo desde /data
    auto aj = *Json::parse(ajustar("{}").cuerpo);
    CHECK(aj.at("trato").texto() == "Daniel" && aj.at("ciudad").texto() == "Mérida" && aj.at("proveedor").texto() == "local");
    CHECK(aj.at("modeloOllama").texto() == "llama3.2" && aj.at("claves").at("claude").booleano());
    { std::ifstream f(base / "datos" / "ajustes.json"); std::stringstream ss; ss << f.rdbuf(); CHECK(Json::parse(ss.str())); }

    // Cerebros en la nube (red simulada)
    msg("Agendar cena familiar mañana a las 7");
    std::vector<std::string> cuerpos;
    int llamadasClaude = 0;
    red = [&](const std::string& url, const std::string& cuerpo, const std::vector<std::string>& cab) -> RespuestaHttp {
        cuerpos.push_back(cuerpo);
        if (contiene(url, "api.anthropic.com")) {
            bool llave = false;
            for (const auto& c : cab) llave = llave || c == "x-api-key: clave-claude";
            if (!llave) return {401, R"({"error":{"message":"x-api-key"}})"};
            if (llamadasClaude++ == 0)
                return {200, R"({"content":[{"type":"server_tool_use","name":"web_search"},{"type":"text","text":"Parte uno. "}],"stop_reason":"pause_turn"})"};
            return {200, R"({"content":[{"type":"text","text":"Parte dos."}],"stop_reason":"end_turn"})"};
        }
        if (contiene(url, "generativelanguage.googleapis.com")) {
            if (contiene(cuerpo, "google_search")) return {400, R"({"error":{"message":"Search Grounding is not supported on the free tier"}})"};
            return {200, R"({"candidates":[{"content":{"parts":[{"thought":true,"text":"pienso"},{"text":"Respuesta de Gemini"}]}}]})"};
        }
        if (contiene(url, "ollama:11434")) {
            if (contiene(cuerpo, "\"model\":\"falta\"")) return {404, R"({"error":"model 'falta' not found"})"};
            if (contiene(cuerpo, "\"model\":\"caido\"")) throw ErrorRed("Couldn't connect to server");
            return {200, R"({"message":{"content":"<think>razono</think>Hola desde Ollama"}})"};
        }
        return falsoHttp("GET", url, cuerpo, {}, 0);
    };
    ajustar("{\"proveedor\":\"claude\"}");
    cuerpos.clear();
    r = msg("¿Qué me recomiendas ponerme para una reunión?");
    CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "roja") && cuerpos.empty());
    r = msg("compara dos laptops");
    CHECK(r.at("origen").texto() == "nube" && r.at("proveedor").texto() == "claude" && r.at("respuesta").texto() == "Parte uno. Parte dos.");
    CHECK(cuerpos.size() == 2 && contiene(cuerpos[0], "web_search_20250305") && contiene(cuerpos[1], "server_tool_use"));  // pause_turn continuó
    CHECK(contiene(cuerpos[0], "Al recomendar o elegir por el usuario"));
    CHECK(contiene(cuerpos[0], "Me gusta vestir de azul y prefiero zapatos cómodos"));
    CHECK(contiene(cuerpos[0], "Cena familiar") && !contiene(cuerpos[0], "clave alfa") && !contiene(cuerpos[0], "Ana"));
    cuerpos.clear(); llamadasClaude = 1;
    msg("explícame los agujeros negros");
    CHECK(contiene(cuerpos.back(), "compara dos laptops") && contiene(cuerpos.back(), "Parte uno. Parte dos."));  // memoria de la conversación

    ajustar("{\"proveedor\":\"gemini\"}");
    cuerpos.clear();
    r = msg("piensa en algo difícil");
    CHECK(r.at("respuesta").texto() == "Respuesta de Gemini" && cuerpos.size() == 2 && contiene(cuerpos[0], "google_search") && !contiene(cuerpos[1], "google_search"));
    cuerpos.clear();
    msg("analiza otra cosa");
    CHECK(cuerpos.size() == 1 && !contiene(cuerpos[0], "google_search"));  // recuerda que la cuenta no admite búsqueda

    ajustar("{\"proveedor\":\"ollama\"}");
    CHECK(msg("explica la gravedad").at("respuesta").texto() == "Hola desde Ollama");
    ajustar("{\"modeloOllama\":\"falta\"}");
    CHECK(contiene(msg("explica la gravedad").at("respuesta").texto(), "ollama pull falta"));
    ajustar("{\"modeloOllama\":\"caido\"}");
    CHECK(contiene(msg("explica la gravedad").at("respuesta").texto(), "No encuentro Ollama"));

    cfg.claveClaude.clear();
    app = std::make_unique<Servidor>(cfg, http);
    ajustar("{\"proveedor\":\"claude\"}");
    r = msg("compara dos laptops");
    CHECK(r.at("origen").texto() == "error" && contiene(r.at("respuesta").texto(), "Falta ANTHROPIC_API_KEY"));
    ajustar("{\"proveedor\":\"local\"}");
    red = [](const std::string&, const std::string&, const std::vector<std::string>&) -> RespuestaHttp { throw ErrorRed("sin red"); };
    CHECK(contiene(msg("compara dos laptops").at("respuesta").texto(), "no he comprendido"));
    {  // ── Closet: recomendación con las prendas del usuario, API y privacidad ──
        std::vector<std::string> cuerpos;
        FuncionHttp httpCloset = [&](const std::string&, const std::string& url, const std::string& cuerpo,
                                     const std::vector<std::string>&, int) -> RespuestaHttp {
            cuerpos.push_back(cuerpo);
            if (contiene(url, "api.anthropic.com")) return {200, R"({"content":[{"type":"text","text":"Respuesta de la nube."}],"stop_reason":"end_turn"})"};
            return falsoHttp("GET", url, cuerpo, {}, 0);   // lluvia 40 % y máxima 23 °C en Mérida
        };
        auto nuevo = [&](const char* nombre, const std::string& ciudad) {
            ConfigServidor c = cfg;
            c.datos = (base / nombre).string();
            c.ciudadInicial = ciudad;
            c.claveClaude = "clave-claude";
            fs::create_directories(c.datos);
            return std::make_unique<Servidor>(c, httpCloset);
        };
        auto vestuario = [&](Servidor& s) {
            for (const char* f : {"guarda mi camisa azul formal", "guarda mi camisa verde formal", "guarda mi pantalón gris formal",
                                  "guarda mis zapatos negros formales", "guarda mi chaqueta gris formal",
                                  "guarda mi camiseta roja informal", "guarda mis tenis blancos informales"})
                s.responder(f);
        };

        auto conLluvia = nuevo("datos-closet", "Mérida");
        vestuario(*conLluvia);
        Json r = conLluvia->responder("qué me pongo para una entrevista");
        CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "con las prendas de su closet"));
        CHECK(contiene(r.at("respuesta").texto(), "camisa azul") && contiene(r.at("respuesta").texto(), "pantalón gris") &&
              contiene(r.at("respuesta").texto(), "zapatos negros"));
        CHECK(contiene(r.at("respuesta").texto(), "40% de lluvia") && contiene(r.at("respuesta").texto(), "chaqueta gris"));  // paraguas -> abrigo
        CHECK(r.at("outfit").at("completo").booleano() && r.at("outfit").at("abrigo").at("nombre").texto() == "chaqueta gris");
        CHECK(conLluvia->responder("hola").at("outfit").esNulo());

        Json inf = conLluvia->responder("qué me pongo para una fiesta con amigos");
        CHECK(contiene(inf.at("respuesta").texto(), "camiseta roja") && contiene(inf.at("respuesta").texto(), "tenis blancos"));
        CHECK(contiene(inf.at("respuesta").texto(), "Le falta: parte de abajo informal") && !inf.at("outfit").at("completo").booleano());

        conLluvia->responder("Mi color favorito es verde");
        CHECK(contiene(conLluvia->responder("qué me pongo para una entrevista").at("respuesta").texto(), "camisa verde"));
        CHECK(contiene(conLluvia->responder("qué me pongo para una entrevista").at("respuesta").texto(), "siguiendo sus preferencias de color"));

        conLluvia->responder("agendar reunión con el cliente hoy a las 6 de la tarde");
        r = conLluvia->responder("qué me pongo");
        CHECK(contiene(r.at("respuesta").texto(), "su compromiso «Reunión con el cliente»") && !r.at("outfit").esNulo());

        auto sinCiudad = nuevo("datos-closet-sin-ciudad", "");
        vestuario(*sinCiudad);
        r = sinCiudad->responder("qué me pongo para una entrevista");
        CHECK(!contiene(r.at("respuesta").texto(), "lluvia") && r.at("outfit").at("abrigo").esNulo());   // sin ciudad no se inventa el clima

        auto vacio = nuevo("datos-closet-vacio", "");
        r = vacio->responder("qué me pongo para una entrevista");
        CHECK(contiene(r.at("respuesta").texto(), "le recomiendo una camisa") && r.at("outfit").esNulo());   // respaldo genérico de siempre

        auto soloInformal = nuevo("datos-closet-informal", "");
        soloInformal->responder("guarda mi camiseta roja informal");
        r = soloInformal->responder("qué me pongo para una entrevista");
        CHECK(contiene(r.at("respuesta").texto(), "le recomiendo una camisa") && contiene(r.at("respuesta").texto(), "no tiene prendas formales"));

        // El núcleo pregunta el estilo y la respuesta corta no se pierde
        auto pregunta2 = nuevo("datos-closet-pregunta2", "");
        CHECK(contiene(pregunta2->responder("agrega unos zapatos negros").at("respuesta").texto(), "formal o informal"));
        r = pregunta2->responder("formal");
        CHECK(r.at("origen").texto() == "local" && contiene(r.at("respuesta").texto(), "Guardada"));

        // API
        auto api = nuevo("datos-closet-api", "");
        Json lista = *Json::parse(api->manejar(peticion("GET", "/api/closet")).cuerpo);
        CHECK(lista.at("prendas").size() == 0);
        auto alta = api->manejar(peticion("POST", "/api/closet/agregar", R"({"nombre":"Camisa blanca","categoria":"superior","color":"blanca","estilo":"formal"})"));
        CHECK(alta.estado == 200 && contiene(alta.cuerpo, "\"color\":\"blanco\""));
        CHECK(api->manejar(peticion("POST", "/api/closet/agregar", R"({"nombre":"Camisa blanca","categoria":"superior","color":"blanca","estilo":"formal"})")).estado == 400);
        CHECK(api->manejar(peticion("POST", "/api/closet/agregar", R"({"nombre":"X","categoria":"gorro","color":"","estilo":"formal"})")).estado == 400);
        CHECK(api->manejar(peticion("POST", "/api/closet/agregar", R"({"nombre":"X","categoria":"superior","color":"","estilo":"formal"})", {{"x-jarvis", ""}})).estado == 403);
        Json o = *Json::parse(api->manejar(peticion("POST", "/api/outfit", R"({"estilo":"formal"})")).cuerpo);
        CHECK(o.at("superior").at("nombre").texto() == "Camisa blanca" && !o.at("completo").booleano() && o.at("faltantes").size() == 2);
        CHECK(api->manejar(peticion("POST", "/api/outfit", R"({"estilo":"raro"})")).estado == 400);
        CHECK(contiene(api->manejar(peticion("POST", "/api/closet/eliminar", R"({"id":1})")).cuerpo, "\"ok\":true"));
        CHECK(Json::parse(api->manejar(peticion("GET", "/api/closet")).cuerpo)->at("prendas").size() == 0);
        CHECK(!Json::parse(api->manejar(peticion("GET", "/api/clima")).cuerpo)->at("disponible").booleano());   // sin ciudad

        // Privacidad: lo que hay en el closet no viaja a la nube
        cuerpos.clear();
        auto nube = nuevo("datos-closet-nube", "");
        nube->responder("guarda mi camisa azul formal");
        nube->manejar(peticion("POST", "/api/ajustes", R"({"proveedor":"claude"})"));
        r = nube->responder("explica la gravedad");
        CHECK(r.at("origen").texto() == "nube");
        bool viajo = false;
        for (const std::string& c : cuerpos) viajo = viajo || contiene(c, "camisa azul");
        CHECK(!cuerpos.empty() && !viajo);
    }

    fs::remove_all(base);
    return fallos - antes;
}
#endif

int main() {
    // ── JSON y HTTP (cliente libcurl real contra el servidor propio) ──
    {
        auto k = Json::parse("{\"a\":[1,2.5,{\"b\":null}],\"s\":\"\\u00e1\\ud83d\\ude00\\n\",\"t\":true}");
        CHECK(k && k->at("a").at(1).numero() == 2.5 && k->at("s").texto() == "á😀\n" && k->at("t").booleano());
        CHECK(!Json::parse("{\"a\":}") && !Json::parse("[1,2") && !Json::parse("{} basura"));
        Json o = Json::objeto(); o["n"] = 3; o["l"].push(1); o["l"].push("dos\"\n");
        CHECK(o.dump() == "{\"n\":3,\"l\":[1,\"dos\\\"\\n\"]}");

        ServidorHttp srv(0, [](const PeticionHttp& p) {
            Json j = Json::objeto();
            j["metodo"] = p.metodo; j["ruta"] = p.ruta; j["consulta"] = p.consulta; j["cuerpo"] = p.cuerpo;
            j["x"] = p.cabeceras.count("x-prueba") ? p.cabeceras.at("x-prueba") : "";
            return RespuestaServidor{200, "application/json; charset=utf-8", j.dump(), {}};
        });
        CHECK(srv.iniciar());
        std::thread hilo([&] { srv.ejecutar(); });
        const std::string base = "http://127.0.0.1:" + std::to_string(srv.puerto());
        auto r = httpCurl("GET", base + "/hola%20mundo?a=1", "", {"X-Prueba: abc"}, 5);
        auto j = Json::parse(r.cuerpo);
        CHECK(r.estado == 200 && j && j->at("ruta").texto() == "/hola mundo" && j->at("consulta").texto() == "a=1" && j->at("x").texto() == "abc");
        const std::string cuerpo = "{\"texto\":\"cálculo ñandú 😀\"}";
        j = Json::parse(httpCurl("POST", base + "/api", cuerpo, {"Content-Type: application/json"}, 5).cuerpo);
        CHECK(j && j->at("cuerpo").texto() == cuerpo);
        CHECK(httpCurl("POST", base + "/enorme", std::string(1200000, 'x'), {}, 5).estado == 413);
        bool error = false;
        try { httpCurl("GET", "http://127.0.0.1:1/", "", {}, 3); } catch (const ErrorRed&) { error = true; }
        CHECK(error);
        CHECK(urlCodificar("qué es?") == "qu%C3%A9%20es%3F");
        srv.detener();
        hilo.join();
    }

    // ── Calculadora ──
    {
        Banco b;
        struct { const char* frase; const char* esperado; } casos[] = {
            {"cuánto es quince por ocho", "120"}, {"calcula 12 más 30", "42"}, {"cuánto es la raíz cuadrada de 144", "12"},
            {"cuánto es el 15 por ciento de 80", "12"}, {"15 por 8", "120"}, {"cuánto es 10 entre 4", "2,5"},
            {"cuánto es dos elevado a diez", "1024"}, {"cuánto es la mitad de 90", "45"}, {"cuánto es 7 al cuadrado", "49"},
            {"cuánto son 3,5 por 2", "7"}, {"calcula treinta y cinco menos doce", "23"},
            {"calcula treinta y cinco más dos mil quinientos", "2535"}, {"cuánto es 2 más 3 por 4", "14"}};
        for (const auto& c : casos) {
            std::string r = b.di(c.frase);
            CHECK(contiene(r, std::string("es ") + c.esperado + ","));
            if (!contiene(r, std::string("es ") + c.esperado + ",")) std::printf("    '%s' -> %s\n", c.frase, r.c_str());
        }
        CHECK(b.di("raíz cuadrada de 144") == "<nada>");  // sin verbo y sin empezar por número
        CHECK(contiene(b.di("cuánto es 5 entre 0"), "cero"));
        CHECK(b.di("recuérdame comprar 2 por 1") == "<nada>");
        CHECK(b.di("tengo 2 por 3 reuniones") == "<nada>");
        CHECK(b.di("agendar calcular 2 por 3 mañana") == "<nada>");
        CHECK(b.di("cuánto es esto") == "<nada>");
        CHECK(b.di("cuánto es __import__('os')") == "<nada>");
        CHECK(b.di("calcula 9 elevado a 99999") == "<nada>");
        std::string polinomio = b.di("resuelve x^3 - 6x^2 + 11x - 6 = 0");
        CHECK(contiene(polinomio, "x1 = 1") && contiene(polinomio, "x2 = 2") && contiene(polinomio, "x3 = 3"));
        std::string complejas = b.di("resuelve x^3 + 1 = 0");
        CHECK(contiene(complejas, "x1 = -1") && contiene(complejas, "0,5 - 0,866025i") && contiene(complejas, "0,5 + 0,866025i"));
        CHECK(contiene(b.di("resuelve x^4 - 5x^2 + 4 = 0"), "x4 = 2"));
        CHECK(contiene(b.di("resuelve x^2 + 1 = 0"), "i"));
        std::string repetidas = b.di("resuelve el polinomio x^3 - 3x^2 + 3x - 1 = 0");
        CHECK(contiene(repetidas, "x1 = 1") && contiene(repetidas, "x2 = 1") && contiene(repetidas, "x3 = 1"));
        std::string multiplicidades = b.di("resuelve x^3 - 3x + 2 = 0");
        CHECK(contiene(multiplicidades, "x1 = -2") && contiene(multiplicidades, "x2 = 1") &&
              contiene(multiplicidades, "x3 = 1"));
        CHECK(contiene(b.di("resuelve 2 + 2"), "El resultado es 4"));  // conserva la evaluación aritmética existente
    }

    // ── Conversor ──
    {
        Banco b;
        CHECK(contiene(b.di("convierte 5 millas a kilómetros"), "8,0467 kilómetros"));
        CHECK(contiene(b.di("cuántos kilómetros son 10 millas"), "16,0934 kilómetros"));
        CHECK(contiene(b.di("convierte 30 grados celsius a fahrenheit"), "86 grados Fahrenheit"));
        CHECK(contiene(b.di("convierte 100 fahrenheit a celsius"), "37,7778 grados Celsius"));
        CHECK(contiene(b.di("cuántos minutos son tres horas"), "180 minutos"));
        CHECK(contiene(b.di("convierte 5 kilos a metros"), "no miden lo mismo"));
    }

    // ── Fechas, hora mundial ──
    {
        Banco b;
        std::string n = b.di("cuántos días faltan para navidad");
        CHECK(contiene(n, "75 días") && contiene(n, "viernes"));
        CHECK(contiene(b.di("qué día cae el 25 de diciembre"), "viernes"));
        CHECK(contiene(b.di("qué fecha será dentro de 30 días"), "martes 10 de noviembre de 2026"));
        CHECK(contiene(b.di("cuántos días faltan para el viernes"), "5 días"));
        CHECK(contiene(b.di("cuántos días faltan para el 12 de octubre"), "Falta 1 día"));
        CHECK(contiene(b.di("qué fecha fue hace 2 semanas"), "domingo 27 de septiembre de 2026"));
        std::string t = b.di("qué hora es en Tokio");
        CHECK(contiene(t, "4 de la madrugada") && contiene(t, "día siguiente"));
        CHECK(contiene(b.di("qué hora es en Nueva York"), "3 de la tarde"));
        CHECK(contiene(b.di("qué hora es en Alemania"), "En Berlín son las 9 de la noche"));
        CHECK(contiene(b.di("qué hora es en Oslo"), "9 de la noche"));
        CHECK(contiene(b.di("qué hora es en Estados Unidos"), "Indíqueme una ciudad o región específica"));
        std::tm invierno{};
        invierno.tm_year = 126;
        invierno.tm_mon = 0;
        invierno.tm_mday = 15;
        invierno.tm_hour = 12;
        const std::time_t mediodiaUtc = timegm(&invierno);
        b.ctx.reloj = [mediodiaUtc] { return mediodiaUtc; };
        CHECK(contiene(b.di("qué hora es en Nueva York"), "7 de la mañana"));
        CHECK(contiene(b.di("qué hora es en Atlantida"), "No encuentro"));
        CHECK(b.di("qué hora es") == "<nada>");
    }

    // ── Temporizador ──
    {
        Banco b;
        struct { const char* frase; long seg; } casos[] = {{"pon un temporizador de 10 minutos", 600}, {"temporizador de una hora y media", 5400},
                                                           {"pon un temporizador de dos horas y treinta minutos", 9000},
                                                           {"temporizador de media hora", 1800}, {"cronómetro de 45 segundos", 45}};
        for (const auto& c : casos) {
            auto r = b.raw(c.frase);
            CHECK(r && r->accion.at("segundos").entero() == c.seg);
        }
        CHECK(contiene(b.di("temporizador de una hora y media"), "1 hora y 30 minutos"));
        auto c = b.raw("cancela el temporizador");
        CHECK(c && c->accion.at("tipo").texto() == "cancelar_temporizadores");
        auto s = b.raw("pon un temporizador");
        CHECK(s && s->accion.esNulo());
        CHECK(b.di("recuérdame el temporizador de la pasta") == "<nada>");
    }

    // ── Azar y charla ──
    {
        Banco b;
        std::string d = b.di("tira un dado");
        CHECK(d.rfind("Ha salido el ", 0) == 0);
        CHECK(contiene(b.di("lanza dos dados"), "suman"));
        CHECK(contiene(b.di("tira un dado de 20 caras"), "Ha salido el "));
        std::string m = b.di("lanza una moneda");
        CHECK(m.rfind("Cara,", 0) == 0 || m.rfind("Cruz,", 0) == 0);
        std::string nr = b.di("dame un número al azar entre 1 y 10");
        CHECK(!nr.empty() && std::isdigit(static_cast<unsigned char>(nr[0])));
        std::string e = b.di("elige entre pizza o pasta");
        CHECK(contiene(e, "pizza") || contiene(e, "pasta"));

        const std::string presentacion = b.di("¿quién eres?");
        CHECK(contiene(presentacion, "Soy Nix.") && contiene(presentacion, "tareas y notas") &&
              contiene(presentacion, "cálculos y conversiones") && !contiene(presentacion, "Intelligent System"));
        std::set<std::string> chistes;
        for (int i = 0; i < 4; ++i) chistes.insert(b.di("cuéntame un chiste"));
        CHECK(chistes.size() == 4);
        CHECK(contiene(b.di("estado de los sistemas"), "Tengo 3 tareas pendientes y 1 nota guardada"));
        CHECK(contiene(b.di("estado de los sistemas"), "desconectado"));
        CHECK(contiene(b.di("adiós"), "Hasta pronto"));
    }

    // ── Clima ──
    {
        Banco b;
        std::string r = b.di("qué clima hace en Mérida");
        CHECK(contiene(r, "Mérida") && contiene(r, "19 grados") && contiene(r, "40 por ciento"));
        r = b.di("va a llover mañana en Mérida");
        CHECK(r.rfind("Mañana", 0) == 0 && contiene(r, "lluvia ligera") && contiene(r, "80 por ciento"));
        CHECK(contiene(b.di("qué clima hace en Atlantida"), "no encuentro"));

        Banco c;
        CHECK(contiene(c.di("qué temperatura hace"), "¿De qué ciudad"));
        CHECK(contiene(c.di("Mérida"), "19 grados"));
        auto v = c.raw("vivo en Mérida");
        CHECK(v && v->ajusteCiudad == "Mérida");
        Banco d(falsoHttp, "Mérida");
        CHECK(contiene(d.di("qué clima hace"), "19 grados"));
        Banco s(roto);
        CHECK(contiene(s.di("qué clima hace en Mérida"), "No logro completar"));
    }

    // ── Wikipedia ──
    {
        Banco b;
        std::string r = b.di("qué es la fotosíntesis");
        CHECK(r.rfind("Según Wikipedia, La fotosíntesis es un proceso mediante", 0) == 0);
        CHECK(!contiene(r, "griego"));
        CHECK(contiene(r, "¿Desea que continúe"));
        CHECK(contiene(b.di("continúa"), "esencial"));
        CHECK(contiene(b.di("sigue"), "Eso es todo"));
        CHECK(contiene(Banco().di("qué es mercurio"), "varios significados"));
        CHECK(contiene(Banco().di("qué es inexistente"), "nada sobre"));
        CHECK(Banco().di("qué es lo que tengo mañana") == "<nada>");
        CHECK(Banco().di("qué es la fotosíntesis", true) == "<nada>");
        CHECK(contiene(Banco(roto).di("quién es Einstein"), "No logro consultar Wikipedia"));
        CHECK(Banco().di("continúa") == "<nada>");
        Banco w;
        auto ex = wikipedia("investiga sobre la fotosíntesis", w.ctx);
        CHECK(ex && contiene(ex->respuesta, "Según Wikipedia"));
    }

    {  // consejoClima: reglas puras (paraguas desde 30 % de lluvia; abrigo si la máxima es ≤ 18 °C)
        ConsejoClima c = consejoClima({40, 23.1, "Mérida"});
        CHECK(c.disponible && c.paraguas && !c.abrigo && c.ciudad == "Mérida");
        c = consejoClima({29, 18.0, "Oslo"});
        CHECK(c.disponible && !c.paraguas && c.abrigo);
        c = consejoClima({10, 25.0, "Cádiz"});
        CHECK(c.disponible && !c.paraguas && !c.abrigo);
        c = consejoClima({-1, 5.0, ""});
        CHECK(!c.disponible && !c.paraguas && !c.abrigo);   // sin pronóstico no se aconseja nada
        Banco sinCiudad;
        CHECK(!consejoClimaLocal(sinCiudad.ctx).disponible);
        Banco conCiudad(falsoHttp, "Mérida");
        const ConsejoClima real = consejoClimaLocal(conCiudad.ctx);
        CHECK(real.disponible && real.paraguas && !real.abrigo && real.probabilidadLluvia == 40);
        Banco caido(roto, "Mérida");
        CHECK(!consejoClimaLocal(caido.ctx).disponible);    // red caída: no lanza, simplemente no hay consejo
    }

#ifdef PRUEBAS_API
    probarApi();
#endif
    std::printf(fallos ? "\n%d prueba(s) del servidor fallaron\n" : "\nTodas las pruebas del servidor pasaron\n", fallos);
    return fallos ? 1 : 0;
}
