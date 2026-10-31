#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class ListaEnlazada {
    struct Nodo {
        template <typename U>
        explicit Nodo(U&& valor) : valor(std::forward<U>(valor)) {}

        T valor;
        Nodo* anterior = nullptr;
        Nodo* siguiente = nullptr;
    };

public:
    template <bool Constante>
    class Iterador {
        using NodoPtr = typename std::conditional<Constante, const Nodo*, Nodo*>::type;
        NodoPtr actual_ = nullptr;
        friend class ListaEnlazada;
        template <bool>
        friend class Iterador;

        explicit Iterador(NodoPtr actual) : actual_(actual) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = typename std::conditional<Constante, const T*, T*>::type;
        using reference = typename std::conditional<Constante, const T&, T&>::type;

        Iterador() = default;

        template <bool C = Constante, typename = typename std::enable_if<C>::type>
        Iterador(const Iterador<false>& otro) : actual_(otro.actual_) {}

        reference operator*() const { return actual_->valor; }
        pointer operator->() const { return &actual_->valor; }

        Iterador& operator++() {
            actual_ = actual_->siguiente;
            return *this;
        }

        Iterador operator++(int) {
            Iterador copia = *this;
            ++*this;
            return copia;
        }

        template <bool Otra>
        bool operator==(const Iterador<Otra>& otro) const { return actual_ == otro.actual_; }

        template <bool Otra>
        bool operator!=(const Iterador<Otra>& otro) const { return !(*this == otro); }
    };

    using iterator = Iterador<false>;
    using const_iterator = Iterador<true>;

    ListaEnlazada() = default;

    ListaEnlazada(const ListaEnlazada& otra) {
        try {
            for (const T& valor : otra) push_back(valor);
        } catch (...) {
            limpiar();
            throw;
        }
    }

    ListaEnlazada(ListaEnlazada&& otra) noexcept
        : primero_(otra.primero_), ultimo_(otra.ultimo_), cantidad_(otra.cantidad_) {
        otra.primero_ = nullptr;
        otra.ultimo_ = nullptr;
        otra.cantidad_ = 0;
    }

    ListaEnlazada& operator=(const ListaEnlazada& otra) {
        if (this != &otra) {
            ListaEnlazada copia(otra);
            intercambiar(copia);
        }
        return *this;
    }

    ListaEnlazada& operator=(ListaEnlazada&& otra) noexcept {
        if (this != &otra) {
            limpiar();
            primero_ = otra.primero_;
            ultimo_ = otra.ultimo_;
            cantidad_ = otra.cantidad_;
            otra.primero_ = nullptr;
            otra.ultimo_ = nullptr;
            otra.cantidad_ = 0;
        }
        return *this;
    }

    ~ListaEnlazada() { limpiar(); }

    void intercambiar(ListaEnlazada& otra) noexcept {
        using std::swap;
        swap(primero_, otra.primero_);
        swap(ultimo_, otra.ultimo_);
        swap(cantidad_, otra.cantidad_);
    }

    bool vacia() const { return cantidad_ == 0; }
    bool empty() const { return vacia(); }
    std::size_t size() const { return cantidad_; }

    iterator begin() { return iterator(primero_); }
    iterator end() { return iterator(nullptr); }
    const_iterator begin() const { return const_iterator(primero_); }
    const_iterator end() const { return const_iterator(nullptr); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    T& front() { return primero_->valor; }
    const T& front() const { return primero_->valor; }
    T& back() { return ultimo_->valor; }
    const T& back() const { return ultimo_->valor; }

    void push_front(const T& valor) { insertarAntes(primero_, valor); }
    void push_front(T&& valor) { insertarAntes(primero_, std::move(valor)); }
    void push_back(const T& valor) { insertarAntes(nullptr, valor); }
    void push_back(T&& valor) { insertarAntes(nullptr, std::move(valor)); }

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
        return iterator(insertarAntes(posicion.actual_, valor));
    }

    iterator insert(iterator posicion, T&& valor) {
        return iterator(insertarAntes(posicion.actual_, std::move(valor)));
    }

    iterator erase(iterator posicion) {
        Nodo* nodo = posicion.actual_;
        if (!nodo) return end();
        Nodo* siguiente = nodo->siguiente;
        if (nodo->anterior) nodo->anterior->siguiente = nodo->siguiente;
        else primero_ = nodo->siguiente;
        if (nodo->siguiente) nodo->siguiente->anterior = nodo->anterior;
        else ultimo_ = nodo->anterior;
        delete nodo;
        --cantidad_;
        return iterator(siguiente);
    }

    bool pop_front() {
        if (!primero_) return false;
        erase(begin());
        return true;
    }

    bool pop_back() {
        if (!ultimo_) return false;
        erase(iterator(ultimo_));
        return true;
    }

    void limpiar() noexcept {
        while (primero_) {
            Nodo* siguiente = primero_->siguiente;
            delete primero_;
            primero_ = siguiente;
        }
        ultimo_ = nullptr;
        cantidad_ = 0;
    }

    std::vector<T> comoVector() const {
        std::vector<T> resultado;
        resultado.reserve(cantidad_);
        for (const T& valor : *this) resultado.push_back(valor);
        return resultado;
    }

private:
    template <typename U>
    Nodo* insertarAntes(Nodo* posicion, U&& valor) {
        Nodo* nuevo = new Nodo(std::forward<U>(valor));
        if (!posicion) {
            nuevo->anterior = ultimo_;
            if (ultimo_) ultimo_->siguiente = nuevo;
            else primero_ = nuevo;
            ultimo_ = nuevo;
        } else {
            nuevo->siguiente = posicion;
            nuevo->anterior = posicion->anterior;
            if (posicion->anterior) posicion->anterior->siguiente = nuevo;
            else primero_ = nuevo;
            posicion->anterior = nuevo;
        }
        ++cantidad_;
        return nuevo;
    }

    Nodo* primero_ = nullptr;
    Nodo* ultimo_ = nullptr;
    std::size_t cantidad_ = 0;
};
