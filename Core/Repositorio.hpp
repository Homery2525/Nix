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

private:
    template <typename F> bool modificar(int id, F&& f);  // extrae, modifica y reinserta para mantener el orden
    void insertarTareaOrdenada(const Tarea& tarea);
    // Solo se reescribe el archivo que cambió, y de forma atómica (temporal + renombrado).
    void persistirTareas() const;
    void persistirNotas() const;

    std::string carpeta_;
    ListaEnlazada<Tarea> tareas_;  // lista enlazada propia, ordenada por urgencia
    ListaEnlazada<Nota> notas_;
    mutable std::vector<Nota> notasVista_;
    mutable bool notasVistaValida_ = false;
    int sigIdTarea_ = 1;
    int sigIdNota_ = 1;
};
