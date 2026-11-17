                                                                                                                       
                                                                            
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#include "Habilidades.hpp"
#include "Servidor.hpp"

namespace fs = std::filesystem;

static int fallos = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) { std::printf("  FALLO línea %d: %s\n", __LINE__, #cond); ++fallos; } \
    } while (0)

static bool tiene(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }

                                                      
static std::time_t AHORA() {
    std::tm t{}; t.tm_year = 126; t.tm_mon = 9; t.tm_mday = 11; t.tm_hour = 15; t.tm_isdst = -1;
    return std::mktime(&t);
}

static fs::path carpeta(const char* nombre) {
    auto dir = fs::temp_directory_path() / (std::string("nix_api_") + nombre + "_" + std::to_string(std::time(nullptr)) + "_" +
                                            std::to_string(reinterpret_cast<std::uintptr_t>(&fallos) & 0xFFFFF));
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

static PeticionHttp get(const std::string& ruta, const std::string& host = "localhost:8080") {
    PeticionHttp p;
    p.metodo = "GET"; p.ruta = ruta; p.cabeceras["host"] = host;
    return p;
}

static PeticionHttp post(const std::string& ruta, const std::string& cuerpo, bool conCabecera = true) {
    PeticionHttp p;
    p.metodo = "POST"; p.ruta = ruta; p.cuerpo = cuerpo; p.cabeceras["host"] = "localhost:8080";
    if (conCabecera) p.cabeceras["x-jarvis"] = "1";
    return p;
}

static Json json(const RespuestaServidor& r) {
    auto j = Json::parse(r.cuerpo);
    return j ? *j : Json();
}

static Json decir(Servidor& s, const std::string& texto) {
    Json cuerpo = Json::objeto();
    cuerpo["texto"] = texto;
    return json(s.manejar(post("/api/mensaje", cuerpo.dump())));
}

                                                                        
static std::string http_crudo(int puerto, const std::string& peticion) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET; a.sin_port = htons(static_cast<uint16_t>(puerto));
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    timeval tv{5, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    if (connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) { close(fd); return ""; }
    send(fd, peticion.data(), peticion.size(), 0);
    std::string r; char b[4096]; ssize_t n;
    while ((n = recv(fd, b, sizeof b, 0)) > 0) r.append(b, static_cast<size_t>(n));
    close(fd);
    return r;
}

int main() {
    setenv("TZ", "UTC", 1);
    tzset();

    const fs::path datos = carpeta("datos"), web = carpeta("web");
    { std::ofstream(web / "index.html") << "<h1>Nix</h1>"; }
    { std::ofstream(web.parent_path() / "secreto.txt") << "no debe salir"; }

    ConfigServidor cfg;
    cfg.web = web.string(); cfg.datos = datos.string(); cfg.tratoInicial = "señor"; cfg.reloj = AHORA;

    {
        Servidor s(cfg);

                                                
        CHECK(s.manejar(get("/salud")).estado == 200 && s.manejar(get("/salud")).cuerpo == "ok");
        CHECK(s.manejar(get("/api/tareas", "evil.com")).estado == 403);                     
        CHECK(s.manejar(get("/api/tareas", "127.0.0.1:8080")).estado == 200);
        CHECK(s.manejar(post("/api/mensaje", R"({"texto":"hola"})", false)).estado == 403);
        CHECK(s.manejar(post("/api/mensaje", "{roto")).estado == 400);
        CHECK(s.manejar(get("/api/nada")).estado == 404);
        PeticionHttp put = get("/api/tareas"); put.metodo = "PUT";
        CHECK(s.manejar(put).estado == 404);

                                   
        CHECK(tiene(s.manejar(get("/")).cuerpo, "<h1>Nix</h1>"));
        CHECK(s.manejar(get("/../secreto.txt")).estado == 404);                               
        CHECK(s.manejar(get("/%2e%2e/secreto.txt")).estado == 404);
        CHECK(s.manejar(get("/no-existe.js")).estado == 404);

                                         
        Json r = decir(s, "agendar reunión con el cliente mañana a las 4");
        CHECK(r.at("origen").texto() == "local" && tiene(r.at("respuesta").texto(), "eunión"));
        Json t = json(s.manejar(get("/api/tareas")));
        CHECK(t.at("tareas").size() == 1 && t.at("tareas").at(0).at("estiloEvento").texto() == "formal");
        decir(s, "anota que el parcial es el viernes");
        CHECK(json(s.manejar(get("/api/notas"))).at("notas").size() == 1);
        CHECK(decir(s, "   ").at("respuesta").texto().empty());
        CHECK(decir(s, std::string(5000, 'a')).at("respuesta").texto().size() > 0);                             

        const int idTarea = static_cast<int>(t.at("tareas").at(0).at("id").entero());
        CHECK(json(s.manejar(post("/api/tarea/completar", "{\"id\":" + std::to_string(idTarea) + "}"))).at("ok").booleano());
        CHECK(!json(s.manejar(post("/api/tarea/completar", R"({"id":999})"))).at("ok").booleano());

                                         
        CHECK(json(s.manejar(get("/api/closet"))).at("prendas").size() == 0);
        r = decir(s, "guarda mi camisa azul, es formal");
        CHECK(tiene(r.at("respuesta").texto(), "Guardada") && r.at("outfit").esNulo());
        CHECK(tiene(decir(s, "agrega un pantalón gris").at("respuesta").texto(), "formal o informal"));
        decir(s, "formal");
        decir(s, "guarda mis zapatos negros formales");
        decir(s, "guarda mi camiseta roja informal");
        Json closet = json(s.manejar(get("/api/closet")));
        CHECK(closet.at("prendas").size() == 4);
        CHECK(closet.at("prendas").at(0).at("nombre").texto() == "camisa azul" &&
              closet.at("prendas").at(0).at("categoria").texto() == "superior" &&
              closet.at("prendas").at(0).at("estilo").texto() == "formal");

                                   
        Json nueva = json(s.manejar(post("/api/closet/agregar",
                          R"({"nombre":"Chaqueta gris","categoria":"abrigo","color":"Gris","estilo":"formal"})")));
        CHECK(nueva.at("ok").booleano() && nueva.at("prenda").at("color").texto() == "gris" && nueva.at("prenda").at("id").entero() == 5);
        CHECK(s.manejar(post("/api/closet/agregar", R"({"nombre":"Chaqueta gris","categoria":"abrigo","color":"gris","estilo":"formal"})")).estado == 400);
        CHECK(s.manejar(post("/api/closet/agregar", R"({"nombre":"Gorra","categoria":"sombrero","color":"azul","estilo":"formal"})")).estado == 400);
        CHECK(s.manejar(post("/api/closet/agregar", R"({"nombre":"","categoria":"superior","color":"azul","estilo":"formal"})")).estado == 400);
        CHECK(s.manejar(post("/api/closet/agregar", R"({"nombre":"Gorra","categoria":"superior","color":"azul","estilo":"raro"})")).estado == 400);

                                                
        r = decir(s, "qué me pongo para una entrevista");
        CHECK(tiene(r.at("respuesta").texto(), "camisa azul, pantalón gris y zapatos negros"));
        CHECK(r.at("outfit").at("completo").booleano() && r.at("outfit").at("superior").at("nombre").texto() == "camisa azul");
        CHECK(r.at("outfit").at("abrigo").esNulo() && r.at("outfit").at("faltantes").size() == 0);
        CHECK(decir(s, "hola").at("outfit").esNulo());

        Json frio = json(s.manejar(post("/api/outfit", R"({"estilo":"formal","abrigo":true})")));
        CHECK(frio.at("abrigo").at("nombre").texto() == "Chaqueta gris" && tiene(frio.at("texto").texto(), "por encima"));
        Json inf = json(s.manejar(post("/api/outfit", R"({"estilo":"informal"})")));
        CHECK(!inf.at("completo").booleano() && inf.at("faltantes").size() == 2);
        CHECK(s.manejar(post("/api/outfit", R"({"estilo":"raro"})")).estado == 400);

                               
        CHECK(json(s.manejar(post("/api/closet/eliminar", R"({"id":5})"))).at("ok").booleano());
        CHECK(!json(s.manejar(post("/api/closet/eliminar", R"({"id":5})"))).at("ok").booleano());
        CHECK(tiene(decir(s, "quita la camiseta roja del closet").at("respuesta").texto(), "He retirado"));
        CHECK(json(s.manejar(get("/api/closet"))).at("prendas").size() == 3);
    }

    {                                                                                          
        Servidor s(cfg);
        CHECK(json(s.manejar(get("/api/closet"))).at("prendas").size() == 3);
        CHECK(json(s.manejar(get("/api/tareas"))).at("tareas").size() == 1);
                                                 
        CHECK(tiene(decir(s, "vacía mi closet").at("respuesta").texto(), "Confirma"));
        decir(s, "sí");
        CHECK(json(s.manejar(get("/api/closet"))).at("prendas").size() == 0);
    }

    {                                                          
        auto redConFrio = [](double maxima) {
            return [maxima](const std::string&, const std::string& url, const std::string&, const std::vector<std::string>&, int) -> RespuestaHttp {
                if (tiene(url, "geocoding-api")) return {200, R"({"results":[{"name":"Mérida","timezone":"America/Caracas"}]})"};
                if (tiene(url, "api.open-meteo.com"))
                    return {200, R"({"current":{"temperature_2m":19.4,"apparent_temperature":18.2,"weather_code":2,"wind_speed_10m":7.6},)"
                                 R"("daily":{"weather_code":[2,61,3],"temperature_2m_max":[)" + std::to_string(maxima) + R"(,21,22],)"
                                 R"("temperature_2m_min":[14.2,13,14],"precipitation_probability_max":[40,80,10]}})"};
                throw ErrorRed("URL inesperada: " + url);
            };
        };
        const fs::path d2 = carpeta("habilidades");
        ConfigServidor c2 = cfg;
        c2.datos = d2.string(); c2.ciudadInicial = "Mérida";
        Servidor s(c2, redConFrio(23.1));
        Json clima = json(s.manejar(get("/api/clima")));
        CHECK(clima.at("disponible").booleano() && clima.at("ciudad").texto() == "Mérida");
        CHECK(clima.at("probabilidadLluvia").entero() == 40 && clima.at("paraguas").booleano() && !clima.at("abrigo").booleano());

        Servidor frio(c2, redConFrio(15.0));
        CHECK(json(frio.manejar(get("/api/clima"))).at("abrigo").booleano());

        ConfigServidor sinCiudad = c2;
        sinCiudad.ciudadInicial = "";
        Servidor sc(sinCiudad, redConFrio(23.1));
        CHECK(!json(sc.manejar(get("/api/clima"))).at("disponible").booleano());                                          

        Servidor roto(c2, [](const std::string&, const std::string&, const std::string&, const std::vector<std::string>&, int) -> RespuestaHttp {
            throw ErrorRed("sin red");
        });
        CHECK(roto.manejar(get("/api/clima")).estado == 200 && !json(roto.manejar(get("/api/clima"))).at("disponible").booleano());

        Json r = decir(s, "calcula 12 más 30");
        CHECK(r.at("origen").texto() == "habilidad" && tiene(r.at("respuesta").texto(), "42"));
        r = decir(s, "pon un temporizador de 10 minutos");
        CHECK(r.at("accion").at("tipo").texto() == "temporizador" && r.at("accion").at("segundos").entero() == 600);

                                                                                                      
        r = decir(s, "guarda mi chaqueta gris formal");
        CHECK(r.at("origen").texto() == "local" && tiene(r.at("respuesta").texto(), "Guardada"));
                                                                                
        CHECK(tiene(decir(s, "agrega un pantalón negro").at("respuesta").texto(), "formal o informal"));
        r = decir(s, "informal");
        CHECK(r.at("origen").texto() == "local" && tiene(r.at("respuesta").texto(), "Guardada"));
        CHECK(json(s.manejar(get("/api/closet"))).at("prendas").size() == 2);
        fs::remove_all(d2);
    }

    {                                               
        Servidor s(cfg);
        ServidorHttp http(0, [&s](const PeticionHttp& p) { return s.manejar(p); });
        CHECK(http.iniciar());
        std::thread hilo([&] { http.ejecutar(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const int puerto = http.puerto();
        std::string resp = http_crudo(puerto, "GET /salud HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
        CHECK(tiene(resp, "200") && tiene(resp, "ok"));
        const std::string cuerpo = R"({"texto":"guarda mis tenis blancos informales"})";
        resp = http_crudo(puerto, "POST /api/mensaje HTTP/1.1\r\nHost: localhost\r\nX-Jarvis: 1\r\nContent-Length: " +
                                      std::to_string(cuerpo.size()) + "\r\nConnection: close\r\n\r\n" + cuerpo);
        CHECK(tiene(resp, "200") && tiene(resp, "Guardada"));
        resp = http_crudo(puerto, "GET /api/closet HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
        CHECK(tiene(resp, "tenis blancos"));
        resp = http_crudo(puerto, "GET /api/closet HTTP/1.1\r\nHost: otro.com\r\nConnection: close\r\n\r\n");
        CHECK(tiene(resp, "403"));
        http.detener();
        hilo.join();
    }

    fs::remove_all(datos);
    fs::remove_all(web);
    fs::remove(web.parent_path() / "secreto.txt");
    if (fallos) {
        std::printf("\nAPI del hito 4: %d prueba(s) fallaron\n", fallos);
        return 1;
    }
    std::printf("\nAPI del hito 4: todas las pruebas pasaron\n");
    return 0;
}
