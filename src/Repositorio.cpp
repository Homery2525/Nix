#include "Repositorio.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ostream>
#include <set>

#include "Texto.hpp"

namespace {

                                                                                                     
                                                                                    
bool escribirAtomico(const std::string& ruta, const std::function<void(std::ostream&)>& escribir) {
    const std::string temporal = ruta + ".tmp";
    {
        std::ofstream f(temporal, std::ios::trunc);
        if (!f) return false;
        escribir(f);
        f.flush();
        if (!f) return false;
    }
    std::error_code ec;
    std::filesystem::rename(temporal, ruta, ec);
    if (ec) { std::filesystem::remove(temporal, ec); return false; }
    return true;
}

}              

Repositorio::Repositorio(std::string carpeta) : carpeta_(std::move(carpeta)) {
    if (!carpeta_.empty()) cargar();
}

void Repositorio::insertarTareaOrdenada(const Tarea& tarea) {
    tareas_.insertarOrdenado(tarea, [](const Tarea& a, const Tarea& b) { return a < b; });
}

void Repositorio::cargar() {
    tareas_.limpiar();
    notas_.limpiar();
    prendas_.limpiar();
    notasVistaValida_ = false;
    std::string linea;
    {
                                                                                                          
                                                                  
        std::vector<Tarea> leidas;
        std::ifstream f(carpeta_ + "/tareas.txt");
        while (std::getline(f, linea))
            if (auto t = Tarea::deserializar(linea)) {
                sigIdTarea_ = std::max(sigIdTarea_, t->id() + 1);
                leidas.push_back(std::move(*t));
            }
        std::sort(leidas.begin(), leidas.end());
        for (Tarea& t : leidas) tareas_.push_back(std::move(t));
    }
    {
        std::ifstream f(carpeta_ + "/notas.txt");
        while (std::getline(f, linea))
            if (auto n = Nota::deserializar(linea)) {
                sigIdNota_ = std::max(sigIdNota_, n->id() + 1);
                notas_.push_back(*n);
            }
    }
    {
        std::vector<Prenda> leidas;
        std::ifstream f(carpeta_ + "/closet.txt");
        while (std::getline(f, linea))
            if (auto p = Prenda::deserializar(linea)) {
                sigIdPrenda_ = std::max(sigIdPrenda_, p->id() + 1);
                leidas.push_back(std::move(*p));
            }
        std::sort(leidas.begin(), leidas.end(), [](const Prenda& a, const Prenda& b) { return a.id() < b.id(); });
        for (Prenda& p : leidas) prendas_.push_back(std::move(p));
    }
}

void Repositorio::persistirTareas() const {
    if (carpeta_.empty()) return;
    escribirAtomico(carpeta_ + "/tareas.txt", [&](std::ostream& f) {
        for (const Tarea& t : tareas_) f << t.serializar() << '\n';
    });
}

void Repositorio::persistirNotas() const {
    if (carpeta_.empty()) return;
    escribirAtomico(carpeta_ + "/notas.txt", [&](std::ostream& f) {
        for (const Nota& n : notas_) f << n.serializar() << '\n';
    });
}

void Repositorio::persistirCloset() const {
    if (carpeta_.empty()) return;
    escribirAtomico(carpeta_ + "/closet.txt", [&](std::ostream& f) {
        for (const Prenda& p : prendas_) f << p.serializar() << '\n';
    });
}

void Repositorio::guardar() const {
    persistirTareas();
    persistirNotas();
    persistirCloset();
}

Tarea Repositorio::agregarTarea(Tarea t) {
    t.setId(sigIdTarea_++);
    insertarTareaOrdenada(t);
    persistirTareas();
    return t;
}

template <typename F>
bool Repositorio::modificar(int id, F&& f) {
    for (auto it = tareas_.begin(); it != tareas_.end(); ++it) {
        if (it->id() == id) {
            Tarea t = std::move(*it);                                                                  
            tareas_.erase(it);
            f(t);
            insertarTareaOrdenada(t);
            persistirTareas();
            return true;
        }
    }
    return false;
}

bool Repositorio::completarTarea(int id) { return modificar(id, [](Tarea& t) { t.completar(); }); }

bool Repositorio::reabrirTarea(int id) { return modificar(id, [](Tarea& t) { t.reabrir(); }); }

bool Repositorio::reprogramarTarea(int id, std::time_t cuando, bool tieneHora) {
    return modificar(id, [&](Tarea& t) { t.reprogramar(cuando, tieneHora); });
}

Tarea Repositorio::restaurarTarea(Tarea t) {
    sigIdTarea_ = std::max(sigIdTarea_, t.id() + 1);
    insertarTareaOrdenada(t);
    persistirTareas();
    return t;
}

bool Repositorio::eliminarTarea(int id) {
    for (auto it = tareas_.begin(); it != tareas_.end(); ++it) {
        if (it->id() == id) {
            tareas_.erase(it);
            persistirTareas();
            return true;
        }
    }
    return false;
}

std::optional<Tarea> Repositorio::obtenerTarea(int id) const {
    for (const Tarea& t : tareas_) if (t.id() == id) return t;
    return std::nullopt;
}

std::vector<Tarea> Repositorio::tareas() const { return tareas_.comoVector(); }

std::vector<Tarea> Repositorio::pendientes() const {
    std::vector<Tarea> r;
    for (const Tarea& t : tareas_) if (!t.completada()) r.push_back(t);
    return r;
}

size_t Repositorio::contarPendientes() const {
    size_t n = 0;
    for (const Tarea& t : tareas_) if (!t.completada()) ++n;
    return n;
}

std::optional<int> Repositorio::buscarTarea(const std::string& consulta, bool soloPendientes) const {
    std::string q = texto::normalizar(consulta);
    if (q.empty()) return std::nullopt;

    if (q.size() <= 3 && std::all_of(q.begin(), q.end(), [](unsigned char c) { return std::isdigit(c); })) {
                                                                                                              
        const size_t idx = static_cast<size_t>(std::stoi(q));
        size_t n = 0;
        if (idx >= 1)
            for (const Tarea& t : tareas_)
                if (!t.completada() && ++n == idx) return t.id();
        return std::nullopt;
    }

    static const std::set<std::string> vacias = {"de", "la", "el", "los", "las", "del", "un", "una", "que",
                                                 "para", "con", "por", "mi", "en", "y", "a", "tarea"};
    std::vector<std::string> palabras;
    for (auto& w : texto::dividir(q)) if (!vacias.count(w)) palabras.push_back(w);
    if (palabras.empty()) return std::nullopt;

    int mejorId = -1, mejorPuntaje = 0;
    for (const Tarea& t : tareas_) {                                   
        if (soloPendientes && t.completada()) continue;
        const std::string titulo = texto::normalizar(t.titulo());
        int puntaje = 0;
        for (auto& w : palabras) if (texto::contiene(titulo, w)) ++puntaje;
        if (puntaje > mejorPuntaje) { mejorPuntaje = puntaje; mejorId = t.id(); }
    }
    if (mejorId < 0) return std::nullopt;
    return mejorId;
}

Nota Repositorio::agregarNota(Nota n) {
    n.setId(sigIdNota_++);
    notas_.push_back(n);
    notasVistaValida_ = false;
    persistirNotas();
    return n;
}

bool Repositorio::eliminarNota(int id) {
    for (auto it = notas_.begin(); it != notas_.end(); ++it) {
        if (it->id() == id) {                                                        
            notas_.erase(it);
            notasVistaValida_ = false;
            persistirNotas();
            return true;
        }
    }
    return false;
}

Prenda Repositorio::agregarPrenda(Prenda p) {
    p.setId(sigIdPrenda_++);
    prendas_.push_back(p);
    persistirCloset();
    return p;
}

Prenda Repositorio::restaurarPrenda(Prenda p) {
    sigIdPrenda_ = std::max(sigIdPrenda_, p.id() + 1);
    prendas_.insertarOrdenado(p, [](const Prenda& a, const Prenda& b) { return a.id() < b.id(); });
    persistirCloset();
    return p;
}

bool Repositorio::eliminarPrenda(int id) {
    for (auto it = prendas_.begin(); it != prendas_.end(); ++it) {
        if (it->id() == id) {
            prendas_.erase(it);
            persistirCloset();
            return true;
        }
    }
    return false;
}

size_t Repositorio::vaciarCloset() {
    const size_t n = prendas_.size();
    prendas_.limpiar();
    persistirCloset();
    return n;
}

std::optional<Prenda> Repositorio::obtenerPrenda(int id) const {
    for (const Prenda& p : prendas_) if (p.id() == id) return p;
    return std::nullopt;
}

bool Repositorio::existePrenda(const Prenda& nueva) const {
    const std::string nombre = texto::normalizar(nueva.nombre());
    for (const Prenda& p : prendas_)
        if (p.categoria() == nueva.categoria() && p.estilo() == nueva.estilo() && texto::normalizar(p.nombre()) == nombre)
            return true;
    return false;
}

std::optional<int> Repositorio::buscarPrenda(const std::string& consulta) const {
    const std::string q = texto::normalizar(consulta);
    if (q.empty()) return std::nullopt;
    if (q.size() <= 3 && std::all_of(q.begin(), q.end(), [](unsigned char c) { return std::isdigit(c); })) {
        const size_t idx = static_cast<size_t>(std::stoi(q));                                                   
        size_t n = 0;
        if (idx >= 1)
            for (const Prenda& p : prendas_)
                if (++n == idx) return p.id();
        return std::nullopt;
    }
    static const std::set<std::string> vacias = {"de", "la", "el", "los", "las", "del", "un", "una", "mi", "mis", "en", "y", "a", "prenda"};
    std::vector<std::string> palabras;
    for (auto& w : texto::dividir(q)) if (!vacias.count(w)) palabras.push_back(w);
    if (palabras.empty()) return std::nullopt;
    int mejorId = -1, mejorPuntaje = 0;
    for (const Prenda& p : prendas_) {
        const std::string campo = texto::normalizar(p.nombre() + " " + p.categoria() + " " + p.color());
        int puntaje = 0;
        for (auto& w : palabras) if (texto::contiene(campo, w)) ++puntaje;
        if (puntaje > mejorPuntaje) { mejorPuntaje = puntaje; mejorId = p.id(); }
    }
    if (mejorId < 0) return std::nullopt;
    return mejorId;
}
