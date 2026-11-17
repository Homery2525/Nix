                                                                                                                 
  
                                                                                                                      
                                                                                                                       
                                                                                                                         
                                                                                                                       
#pragma once

#include <list.hpp>                               

#include <cstddef>
#include <utility>
#include <vector>

template <typename T>
class ListaEnlazada {
    using Lista = Designar::DLList<T>;

public:
    using iterator = typename Lista::Iterator;
    using const_iterator = typename Lista::Iterator;                                                                

    ListaEnlazada() = default;
    ListaEnlazada(const ListaEnlazada& otra) : lista_(otra.lista_) {}
    ListaEnlazada(ListaEnlazada&& otra) noexcept { lista_.swap(otra.lista_); }

    ListaEnlazada& operator=(const ListaEnlazada& otra) {
        if (this != &otra) {
            ListaEnlazada copia(otra);                                                           
            intercambiar(copia);
        }
        return *this;
    }

    ListaEnlazada& operator=(ListaEnlazada&& otra) noexcept {
        if (this != &otra) {
            lista_.clear();
            lista_.swap(otra.lista_);
        }
        return *this;
    }

    void intercambiar(ListaEnlazada& otra) noexcept { lista_.swap(otra.lista_); }

    bool vacia() const { return size() == 0; }
    bool empty() const { return vacia(); }
    std::size_t size() const { return static_cast<std::size_t>(lista_.size()); }

    iterator begin() { return lista_.begin(); }
    iterator end() { return lista_.end(); }
    const_iterator begin() const { return lista_.begin(); }
    const_iterator end() const { return lista_.end(); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    T& front() { return lista_.get_first(); }
    const T& front() const { return lista_.get_first(); }
    T& back() { return lista_.get_last(); }
    const T& back() const { return lista_.get_last(); }

    void push_front(const T& valor) { lista_.insert(valor); }                                     
    void push_front(T&& valor) { lista_.insert(std::move(valor)); }
    void push_back(const T& valor) { lista_.append(valor); }
    void push_back(T&& valor) { lista_.append(std::move(valor)); }

                                                                                                                          
    template <typename Comparador>
    iterator insertarOrdenado(const T& valor, Comparador comparar) {
        iterator posicion = begin();
        while (posicion != end() && !comparar(valor, *posicion)) ++posicion;
        return insert(posicion, valor);
    }

    template <typename Comparador>
    iterator insertarOrdenado(T&& valor, Comparador comparar) {
        iterator posicion = begin();
        while (posicion != end() && !comparar(valor, *posicion)) ++posicion;
        return insert(posicion, std::move(valor));
    }

    iterator insert(iterator posicion, const T& valor) {
        posicion.insert_prev(valor);                                         
        return anteriorA(posicion);
    }

    iterator insert(iterator posicion, T&& valor) {
        posicion.insert_prev(std::move(valor));
        return anteriorA(posicion);
    }

                                                                                                               
    iterator erase(iterator posicion) {
        if (posicion == end()) return end();
        iterator siguiente = posicion;
        ++siguiente;                                                                
        posicion.del();
        return siguiente;
    }

    bool pop_front() {
        if (vacia()) return false;
        lista_.remove_first();
        return true;
    }

    bool pop_back() {
        if (vacia()) return false;
        lista_.remove_last();
        return true;
    }

    void limpiar() noexcept { lista_.clear(); }

    std::vector<T> comoVector() const {
        std::vector<T> resultado;
        resultado.reserve(size());
        for (const T& valor : *this) resultado.push_back(valor);
        return resultado;
    }

private:
    static iterator anteriorA(iterator posicion) {
        --posicion;                                                                 
        return posicion;
    }

    Lista lista_;
};
