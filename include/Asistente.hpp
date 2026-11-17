                                                                              
#pragma once
#include <ctime>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Estructuras.hpp"
#include "Nlu.hpp"
#include "Outfit.hpp"
#include "Repositorio.hpp"

class Asistente {
public:
    explicit Asistente(std::string carpetaDatos = "", std::string usuario = "Homery");

    std::string procesar(const std::string& frase);
    std::optional<std::string> recordarPersona(const std::string& frase);

                                                                                              
                                                                                                              
                                                               
    bool entiende(const std::string& frase) const {
        return analizar(frase).intencion != Intencion::Desconocida;
    }

    std::vector<Tarea> tareas() const { return repo_.tareas(); }
    const std::vector<Nota>& notas() const { return repo_.notas(); }
    bool completarTarea(int id) { return repo_.completarTarea(id); }
    bool eliminarTarea(int id) { return repo_.eliminarTarea(id); }
    bool eliminarNota(int id) { return repo_.eliminarNota(id); }
    bool esperandoConfirmacion() const { return esperandoConfirmacionLimpiarHoy_ || esperandoConfirmacionVaciarCloset_; }

                                                                                                    
    bool esperandoRespuesta() const { return esperandoConfirmacion() || prendaPendiente_ || outfitPendiente_; }

                   
    std::vector<Prenda> prendas() const { return repo_.prendas(); }
                                                                                                                         
    std::optional<Prenda> agregarPrenda(const std::string& nombre, const std::string& categoria,
                                        const std::string& color, const std::string& estilo, std::string& error);
    bool eliminarPrenda(int id) { return repo_.eliminarPrenda(id); }
    Outfit outfitPara(const std::string& estilo, const std::string& color = "", bool abrigo = false) const {
        return armarOutfit(repo_.prendas(), estilo, color, abrigo);
    }
                                                                                                      
    std::string colorPreferido() const;
                                                                                          
    const std::optional<Outfit>& ultimoOutfit() const { return ultimoOutfit_; }

    std::string describirCuando(std::time_t cuando, bool tieneHora) const;
    void usarReloj(std::function<std::time_t()> reloj) { reloj_ = std::move(reloj); }

private:
    std::time_t ahora() const { return reloj_ ? reloj_() : std::time(nullptr); }
    const Resultado& analizar(const std::string& frase) const;
    std::string saludo() const;
    std::string fechaHora() const;
    std::string crearTarea(const Entidades& e);
    std::string crearNota(const Entidades& e);
    std::string listarTareas(const Entidades& e) const;
    std::string listarNotas() const;
    std::string completarOEliminar(const Entidades& e, bool completar);
    std::string reprogramar(const Entidades& e);
    std::string deshacer();
    std::string resumen(bool corto) const;
    std::string limpiarAgendaHoy();
    std::string confirmarLimpiarAgendaHoy();
    std::string avisoDeConflicto(const Tarea& t) const;
    bool vencida(const Tarea& t) const;
    std::optional<int> resolverTarea(const Entidades& e, bool soloPendientes);

    std::string guardarPrendaDicha(const Entidades& e);
    std::string completarPrenda(const PrendaDicha& d);
    std::string listarCloset(const Entidades& e) const;
    std::string quitarPrendaDicha(const Entidades& e);
    std::string quitarPrendaPorId(int id);
    std::string vaciarCloset();
    std::string confirmarVaciarCloset();
    std::string sugerirOutfit(const Entidades& e, const std::string& frase);
    std::string responderOutfit(const std::string& estilo, bool abrigo, const std::string& compromiso);

                                                                                                            
    struct Accion {
        enum Tipo { Crear, Completar, Eliminar, Reprogramar, EliminarDia, AgregarPrenda, QuitarPrenda, VaciarClosetA } tipo;
        int id;
        Tarea antes;
        ListaEnlazada<Tarea> varias{};
        Prenda prenda{};                                                
        ListaEnlazada<Prenda> prendas{};                 
    };
    ListaEnlazada<Accion> historial_;
    mutable int ultimaTareaId_ = 0;                                                                                   
    Intencion ultimaIntencion_ = Intencion::Desconocida;
    bool esperandoConfirmacionLimpiarHoy_ = false;
    bool esperandoConfirmacionVaciarCloset_ = false;
    std::optional<PrendaDicha> prendaPendiente_;                                                            
    bool outfitPendiente_ = false, outfitAbrigoPendiente_ = false;                                                 
    int ultimaPrendaId_ = 0;                                                                              
    std::optional<Outfit> ultimoOutfit_;

    struct CacheAnalisis { std::string frase; std::time_t ahora = 0; bool valido = false; Resultado resultado; };
    mutable CacheAnalisis cache_;
    Nlu nlu_;
    Repositorio repo_;
    std::string usuario_;
    std::function<std::time_t()> reloj_;
    mutable unsigned turno_ = 0;                                                           
};
