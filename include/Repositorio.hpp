                                                                                           
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Estructuras.hpp"
#include "Modelos.hpp"

class Repositorio {
public:
                                                        
    explicit Repositorio(std::string carpeta = "");

    void cargar();
    void guardar() const;

    Tarea agregarTarea(Tarea t);
    bool completarTarea(int id);
    bool eliminarTarea(int id);
    bool reabrirTarea(int id);
    bool reprogramarTarea(int id, std::time_t cuando, bool tieneHora);
    Tarea restaurarTarea(Tarea t);                                                       
    std::optional<Tarea> obtenerTarea(int id) const;
    std::vector<Tarea> tareas() const;                                      
    std::vector<Tarea> pendientes() const;                                                                            
    size_t contarPendientes() const;
                                                                                                              
    const ListaEnlazada<Tarea>& listaTareas() const { return tareas_; }

                                                                           
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

                   
    Prenda agregarPrenda(Prenda p);
    Prenda restaurarPrenda(Prenda p);                                       
    bool eliminarPrenda(int id);
    size_t vaciarCloset();                                              
    std::optional<Prenda> obtenerPrenda(int id) const;
    std::vector<Prenda> prendas() const { return prendas_.comoVector(); }
    const ListaEnlazada<Prenda>& listaPrendas() const { return prendas_; }
    bool existePrenda(const Prenda& p) const;                                     
                                                                                                   
    std::optional<int> buscarPrenda(const std::string& consulta) const;

private:
    template <typename F> bool modificar(int id, F&& f);                                                        
    void insertarTareaOrdenada(const Tarea& tarea);
                                                                                           
    void persistirTareas() const;
    void persistirNotas() const;
    void persistirCloset() const;

    std::string carpeta_;
    ListaEnlazada<Tarea> tareas_;                                                                 
    ListaEnlazada<Nota> notas_;
    ListaEnlazada<Prenda> prendas_;                                                               
    mutable std::vector<Nota> notasVista_;
    mutable bool notasVistaValida_ = false;
    int sigIdTarea_ = 1;
    int sigIdNota_ = 1;
    int sigIdPrenda_ = 1;
};
