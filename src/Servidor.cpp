#include "Servidor.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "ClosetApi.hpp"

namespace fs = std::filesystem;

namespace {

RespuestaServidor respuestaJson(const Json& j, int estado = 200) {
    return {estado, "application/json; charset=utf-8", j.dump(), {}};
}

RespuestaServidor error(int estado, const std::string& mensaje) {
    Json j = Json::objeto();
    j["error"] = mensaje;
    return respuestaJson(j, estado);
}

std::string recortar(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

std::string recortarUtf8(const std::string& s, size_t maxBytes) {                                      
    if (s.size() <= maxBytes) return s;
    size_t n = maxBytes;
    while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) --n;
    return s.substr(0, n);
}

long idDe(const Json& j) {
    if (j.tipo() == Json::Tipo::Numero) return j.entero(-1);
    try { return std::stol(j.texto("-1")); } catch (...) { return -1; }
}

std::string tipoDe(const fs::path& p) {
    static const std::map<std::string, std::string> tipos = {
        {".html", "text/html; charset=utf-8"}, {".js", "text/javascript; charset=utf-8"}, {".css", "text/css; charset=utf-8"},
        {".svg", "image/svg+xml"}, {".png", "image/png"}, {".ico", "image/x-icon"}, {".json", "application/json"}};
    auto it = tipos.find(p.extension().string());
    return it == tipos.end() ? "application/octet-stream" : it->second;
}

}              

Servidor::Servidor(ConfigServidor cfg, FuncionHttp http)
    : cfg_(std::move(cfg)), http_(std::move(http)), inicio_(std::time(nullptr)) {
    fs::create_directories(cfg_.datos);
    tz_ = habilidades::tzValida(cfg_.zonaInicial) ? cfg_.zonaInicial : "UTC";
    habilidades::fijarZona(tz_);
    ciudad_ = cfg_.ciudadInicial;
    asistente_ = std::make_unique<Asistente>(cfg_.datos, cfg_.tratoInicial.empty() ? "señor" : cfg_.tratoInicial);
    if (cfg_.reloj) asistente_->usarReloj(cfg_.reloj);
    ctxHab_.memoria = &memHab_;
    ctxHab_.http = http_;
    ctxHab_.reloj = cfg_.reloj;
}

habilidades::Contexto& Servidor::contextoHabilidades() {
    ctxHab_.trato = cfg_.tratoInicial.empty() ? "señor" : cfg_.tratoInicial;
    ctxHab_.tz = tz_;
    ctxHab_.ciudad = ciudad_;
    ctxHab_.estado = [this] {
        habilidades::EstadoSistema e;
        for (const Tarea& t : asistente_->tareas()) e.tareas += t.completada() ? 0 : 1;
        e.notas = static_cast<int>(asistente_->notas().size());
        e.minutos = static_cast<int>((std::time(nullptr) - inicio_) / 60);
        return e;
    };
    return ctxHab_;
}

                                                                      

Json Servidor::tareasJson() const {
    Json lista = Json::lista();
    for (const Tarea& t : asistente_->tareas()) {
        Json o = Json::objeto();
        o["id"] = t.id(); o["titulo"] = t.titulo();
        o["cuando"] = t.cuando() ? asistente_->describirCuando(t.cuando(), t.tieneHora()) : "";
        o["epoch"] = static_cast<long long>(t.cuando()); o["tieneHora"] = t.tieneHora();
        o["prioridad"] = static_cast<int>(t.prioridad()); o["completada"] = t.completada();
        o["estiloEvento"] = t.estiloEvento();
        lista.push(o);
    }
    Json j = Json::objeto();
    j["tareas"] = lista;
    return j;
}

Json Servidor::notasJson() const {
    Json lista = Json::lista();
    for (const Nota& n : asistente_->notas()) {
        Json o = Json::objeto();
        o["id"] = n.id(); o["titulo"] = n.titulo(); o["contenido"] = n.contenido(); o["categoria"] = n.categoria();
        lista.push(o);
    }
    Json j = Json::objeto();
    j["notas"] = lista;
    return j;
}

Json Servidor::climaJson() {
    const std::time_t t = std::time(nullptr);
    if (!climaCache_.esNulo() && climaCiudad_ == ciudad_ && t - climaHora_ < 600) return climaCache_;
    const habilidades::ConsejoClima c = habilidades::consejoClimaLocal(contextoHabilidades());
    Json j = Json::objeto();
    j["disponible"] = c.disponible;
    j["ciudad"] = c.ciudad;
    j["probabilidadLluvia"] = c.probabilidadLluvia;
    j["temperaturaMaxima"] = c.temperaturaMaxima;
    j["paraguas"] = c.paraguas;
    j["abrigo"] = c.abrigo;
    if (c.disponible) { climaCache_ = j; climaHora_ = t; climaCiudad_ = ciudad_; }                                        
    return j;
}

                                                               

Json Servidor::responder(const std::string& entrada) {
    const std::string texto = recortarUtf8(recortar(entrada), 2000);
    Json out = Json::objeto();
    out["origen"] = "local";
    if (texto.empty()) { out["respuesta"] = ""; return out; }
    std::lock_guard<std::mutex> g(m_);
    if (!asistente_->esperandoRespuesta()) {                                                                          
        const bool entiende = asistente_->entiende(texto);
        if (auto h = habilidades::manejar(texto, contextoHabilidades(), entiende)) {                                      
            if (!h->ajusteCiudad.empty()) { ciudad_ = h->ajusteCiudad; climaHora_ = 0; }
            out["respuesta"] = h->respuesta;
            out["origen"] = "habilidad";
            out["habilidad"] = h->habilidad;
            if (!h->accion.esNulo()) out["accion"] = h->accion;
            return out;
        }
    }
    out["respuesta"] = asistente_->procesar(texto);
    if (const auto& outfit = asistente_->ultimoOutfit()) out["outfit"] = outfitJson(*outfit);                                       
    return out;
}

                                                           

bool Servidor::hostPermitido(const PeticionHttp& p) const {
    auto it = p.cabeceras.find("host");
    const std::string host = it == p.cabeceras.end() ? "" : it->second;
    std::string nombre = host;
    if (!host.empty() && host[0] == '[') nombre = host.substr(0, host.find(']') + 1);
    else nombre = host.substr(0, host.find(':'));
    if (nombre == "localhost" || nombre == "127.0.0.1" || nombre == "[::1]") return true;                             
    return std::find(cfg_.hostsPermitidos.begin(), cfg_.hostsPermitidos.end(), nombre) != cfg_.hostsPermitidos.end();
}

RespuestaServidor Servidor::manejar(const PeticionHttp& p) {
    if (!hostPermitido(p)) return error(403, "host no permitido");
    if (p.metodo == "GET") return get(p);
    if (p.metodo == "POST") {
        auto it = p.cabeceras.find("x-jarvis");
        if (it == p.cabeceras.end() || it->second != "1") return error(403, "falta la cabecera X-Jarvis");                                    
        return post(p);
    }
    return error(404, "no existe");
}

RespuestaServidor Servidor::get(const PeticionHttp& p) {
    if (p.ruta == "/salud") return {200, "text/plain", "ok", {}};
    std::lock_guard<std::mutex> g(m_);
    if (p.ruta == "/api/tareas") return respuestaJson(tareasJson());
    if (p.ruta == "/api/notas") return respuestaJson(notasJson());
    if (p.ruta == "/api/closet") return respuestaJson(closetJson(asistente_->prendas()));
    if (p.ruta == "/api/clima") return respuestaJson(climaJson());
    if (p.ruta.rfind("/api/", 0) == 0) return error(404, "no existe");
    return estatico(p.ruta);
}

RespuestaServidor Servidor::post(const PeticionHttp& p) {
    auto datos = Json::parse(p.cuerpo.empty() ? "{}" : p.cuerpo);
    if (!datos) return error(400, "JSON inválido");
    if (p.ruta == "/api/mensaje") return respuestaJson(responder(datos->at("texto").texto()));

    std::lock_guard<std::mutex> g(m_);
    if (p.ruta == "/api/tarea/completar" || p.ruta == "/api/tarea/eliminar" || p.ruta == "/api/nota/eliminar") {
        const int id = static_cast<int>(idDe(datos->at("id")));
        bool ok = p.ruta == "/api/tarea/completar" ? asistente_->completarTarea(id)
                  : p.ruta == "/api/tarea/eliminar" ? asistente_->eliminarTarea(id) : asistente_->eliminarNota(id);
        Json j = Json::objeto();
        j["ok"] = ok;
        return respuestaJson(j);
    }
    if (p.ruta == "/api/closet/agregar") {                              
        std::string motivo;
        auto prenda = asistente_->agregarPrenda(datos->at("nombre").texto(), datos->at("categoria").texto(),
                                                datos->at("color").texto(), datos->at("estilo").texto(), motivo);
        if (!prenda) return error(400, motivo);
        Json j = Json::objeto();
        j["ok"] = true;
        j["prenda"] = prendaJson(*prenda);
        return respuestaJson(j);
    }
    if (p.ruta == "/api/closet/eliminar") {
        Json j = Json::objeto();
        j["ok"] = asistente_->eliminarPrenda(static_cast<int>(idDe(datos->at("id"))));
        return respuestaJson(j);
    }
    if (p.ruta == "/api/outfit") {                                                                                    
        const std::string estilo = datos->at("estilo").texto();
        if (!estilo.empty() && !Prenda::estiloValido(estilo)) return error(400, "El estilo debe ser formal o informal.");
        std::string color = datos->at("color").texto();
        if (color.empty()) color = asistente_->colorPreferido();
        Json j = outfitJson(asistente_->outfitPara(estilo, color, datos->at("abrigo").booleano()));
        j["colorPreferido"] = color;
        return respuestaJson(j);
    }
    return error(404, "no existe");
}

RespuestaServidor Servidor::estatico(const std::string& ruta) {
    if (ruta.find('\0') != std::string::npos) return error(404, "no existe");
    const std::string rel = ruta == "/" ? "index.html" : ruta.substr(1);
    std::error_code ec;
    const fs::path raiz = fs::weakly_canonical(cfg_.web, ec);
    const fs::path archivo = fs::weakly_canonical(raiz / rel, ec);
    const std::string r = raiz.string(), a = archivo.string();
    if (ec || a.compare(0, r.size() + 1, r + "/") != 0 || !fs::is_regular_file(archivo, ec)) return error(404, "no existe");                       
    std::ifstream in(archivo, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return {200, tipoDe(archivo), ss.str(), {}};
}
