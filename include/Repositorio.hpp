// Repositorio.hpp - Almacena tareas y notas con listas propias y persistencia en archivos.
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Estructuras.hpp"
#include "Modelos.hpp"

class Repositorio {
public:
    // carpeta vacía = solo memoria (útil para pruebas).
    explicit Repositorio(std::string carpeta = "");

    void cargar();
    void guardar() const;

    Tarea agregarTarea(Tarea t);
    bool completarTarea(int id);
    bool eliminarTarea(int id);
    bool reabrirTarea(int id);
    bool reprogramarTarea(int id, std::time_t cuando, bool tieneHora);
    Tarea restaurarTarea(Tarea t);  // conserva el id original (deshacer una eliminación)
    std::optional<Tarea> obtenerTarea(int id) const;
    std::vector<Tarea> tareas() const;      // todas, ordenadas por urgencia
    std::vector<Tarea> pendientes() const;  // solo no completadas (copia: para recorrer sin copiar use listaTareas())
    size_t contarPendientes() const;
    // Recorrido de solo lectura sin copiar nada (las copias de pendientes() reservan memoria por cada tarea).
    const ListaEnlazada<Tarea>& listaTareas() const { return tareas_; }

    // Busca por número (posición en pendientes) o por palabras del título.
    std::optional<int> buscarTarea(const std::string& consulta, bool soloPendientes) const;

    Nota agregarNota(Nota n);
    bool eliminarNota(int id);
    const std::vector<Nota>& notas() const {
        if (!notasVistaValida_) {
            notasVista_ = notas_.comoVector();
            notasVistaValida_ = true;
        }
        return notasVista_;
    }

    // ── Closet ──
    Prenda agregarPrenda(Prenda p);
    Prenda restaurarPrenda(Prenda p);  // conserva el id original (deshacer)
    bool eliminarPrenda(int id);
    size_t vaciarCloset();             // devuelve cuántas prendas había
    std::optional<Prenda> obtenerPrenda(int id) const;
    std::vector<Prenda> prendas() const { return prendas_.comoVector(); }
    const ListaEnlazada<Prenda>& listaPrendas() const { return prendas_; }
    bool existePrenda(const Prenda& p) const;  // mismo nombre, categoría y estilo
    // Busca por número (posición en el closet) o por palabras del nombre, la categoría o el color.
    std::optional<int> buscarPrenda(const std::string& consulta) const;

private:
    template <typename F> bool modificar(int id, F&& f);  // extrae, modifica y reinserta para mantener el orden
    void insertarTareaOrdenada(const Tarea& tarea);
    // Solo se reescribe el archivo que cambió, y de forma atómica (temporal + renombrado).
    void persistirTareas() const;
    void persistirNotas() const;
    void persistirCloset() const;

    std::string carpeta_;
    ListaEnlazada<Tarea> tareas_;  // lista de DeSiGNAR (vía ListaEnlazada), ordenada por urgencia
    ListaEnlazada<Nota> notas_;
    ListaEnlazada<Prenda> prendas_;  // closet: lista de DeSiGNAR, en orden de alta (id creciente)
    mutable std::vector<Nota> notasVista_;
    mutable bool notasVistaValida_ = false;
    int sigIdTarea_ = 1;
    int sigIdNota_ = 1;
    int sigIdPrenda_ = 1;
};
