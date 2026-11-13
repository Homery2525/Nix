#pragma once

#include <cstddef>
#include <iterator>
#include <utility>

namespace Designar {

template <typename T>
class DLList {
private:
    struct Node;

public:
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator() = default;
        Iterator(Node* node, DLList* owner) : node_(node), owner_(owner) {}

        reference operator*() const { return node_->value; }
        pointer operator->() const { return &node_->value; }

        Iterator& operator++() {
            node_ = node_ ? node_->next : nullptr;
            return *this;
        }

        Iterator operator++(int) {
            Iterator copia(*this);
            ++(*this);
            return copia;
        }

        Iterator& operator--() {
            if (node_ == nullptr) {
                node_ = owner_ ? owner_->tail_ : nullptr;
            } else {
                node_ = node_->prev;
            }
            return *this;
        }

        Iterator operator--(int) {
            Iterator copia(*this);
            --(*this);
            return copia;
        }

        void insert_prev(const T& valor) { owner_->insert_before(node_, valor); }
        void insert_prev(T&& valor) { owner_->insert_before(node_, std::move(valor)); }
        void del() {
            if (!node_ || !owner_) return;
            owner_->erase_node(node_);
        }

        bool operator==(const Iterator& otra) const { return node_ == otra.node_ && owner_ == otra.owner_; }
        bool operator!=(const Iterator& otra) const { return !(*this == otra); }

    private:
        Node* node_ = nullptr;
        DLList* owner_ = nullptr;
    };

    using iterator = Iterator;
    using const_iterator = Iterator;

    DLList() = default;
    DLList(const DLList& otra) : DLList() {
        for (const auto& valor : otra) append(valor);
    }
    DLList(DLList&& otra) noexcept : head_(otra.head_), tail_(otra.tail_), size_(otra.size_) {
        otra.head_ = nullptr;
        otra.tail_ = nullptr;
        otra.size_ = 0;
    }

    DLList& operator=(const DLList& otra) {
        if (this != &otra) {
            DLList copia(otra);
            swap(copia);
        }
        return *this;
    }

    DLList& operator=(DLList&& otra) noexcept {
        if (this != &otra) {
            clear();
            head_ = otra.head_;
            tail_ = otra.tail_;
            size_ = otra.size_;
            otra.head_ = nullptr;
            otra.tail_ = nullptr;
            otra.size_ = 0;
        }
        return *this;
    }

    ~DLList() { clear(); }

    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }

    T& get_first() { return head_->value; }
    const T& get_first() const { return head_->value; }
    T& get_last() { return tail_->value; }
    const T& get_last() const { return tail_->value; }

    void insert(const T& valor) { insert_before(head_, valor); }
    void insert(T&& valor) { insert_before(head_, std::move(valor)); }
    void append(const T& valor) { insert_after(tail_, valor); }
    void append(T&& valor) { insert_after(tail_, std::move(valor)); }

    void remove_first() {
        if (empty()) return;
        erase_node(head_);
    }

    void remove_last() {
        if (empty()) return;
        erase_node(tail_);
    }

    void clear() {
        while (!empty()) remove_first();
    }

    void swap(DLList& otra) noexcept {
        std::swap(head_, otra.head_);
        std::swap(tail_, otra.tail_);
        std::swap(size_, otra.size_);
    }

    Iterator begin() { return Iterator(head_, this); }
    Iterator end() { return Iterator(nullptr, this); }
    Iterator begin() const { return Iterator(head_, const_cast<DLList*>(this)); }
    Iterator end() const { return Iterator(nullptr, const_cast<DLList*>(this)); }

private:
    struct Node {
        T value;
        Node* prev = nullptr;
        Node* next = nullptr;

        explicit Node(const T& v) : value(v) {}
        explicit Node(T&& v) : value(std::move(v)) {}
    };

    template <typename U>
    void insert_before(Node* nodo, U&& valor) {
        Node* nuevo = new Node(std::forward<U>(valor));
        if (empty()) {
            head_ = nuevo;
            tail_ = nuevo;
            size_ = 1;
            return;
        }

        if (nodo == nullptr) {
            nuevo->prev = tail_;
            tail_->next = nuevo;
            tail_ = nuevo;
            ++size_;
            return;
        }

        if (nodo == head_) {
            nuevo->next = head_;
            head_->prev = nuevo;
            head_ = nuevo;
            ++size_;
            return;
        }

        Node* previo = nodo->prev;
        nuevo->prev = previo;
        nuevo->next = nodo;
        previo->next = nuevo;
        nodo->prev = nuevo;
        ++size_;
    }

    template <typename U>
    void insert_after(Node* nodo, U&& valor) {
        if (empty()) {
            insert_before(nullptr, std::forward<U>(valor));
            return;
        }

        Node* nuevo = new Node(std::forward<U>(valor));
        if (nodo == nullptr) {
            insert_before(head_, std::forward<U>(valor));
            return;
        }

        if (nodo == tail_) {
            nuevo->prev = tail_;
            tail_->next = nuevo;
            tail_ = nuevo;
            ++size_;
            return;
        }

        Node* siguiente = nodo->next;
        nuevo->prev = nodo;
        nuevo->next = siguiente;
        nodo->next = nuevo;
        siguiente->prev = nuevo;
        ++size_;
    }

    void erase_node(Node* nodo) {
        if (nodo == nullptr) return;

        Node* previo = nodo->prev;
        Node* siguiente = nodo->next;
        if (previo) {
            previo->next = siguiente;
        } else {
            head_ = siguiente;
        }

        if (siguiente) {
            siguiente->prev = previo;
        } else {
            tail_ = previo;
        }

        delete nodo;
        --size_;
        if (empty()) {
            head_ = nullptr;
            tail_ = nullptr;
        }
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

}  // namespace Designar
