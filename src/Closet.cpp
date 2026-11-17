#include "Closet.hpp"

#include <map>

#include "Texto.hpp"

namespace closet {
namespace {

const std::map<std::string, std::string>& categorias() {
    static const std::map<std::string, std::string> m = {
        {"camisa", "superior"}, {"camiseta", "superior"}, {"playera", "superior"}, {"blusa", "superior"},
        {"polo", "superior"}, {"sudadera", "superior"}, {"sueter", "superior"}, {"jersey", "superior"},
        {"top", "superior"}, {"remera", "superior"},
        {"pantalon", "inferior"}, {"jean", "inferior"}, {"jeans", "inferior"}, {"vaquero", "inferior"},
        {"falda", "inferior"}, {"short", "inferior"}, {"bermuda", "inferior"},
        {"zapato", "calzado"}, {"zapatilla", "calzado"}, {"tenis", "calzado"}, {"bota", "calzado"},
        {"botin", "calzado"}, {"sandalia", "calzado"}, {"mocasin", "calzado"}, {"sneaker", "calzado"},
        {"chaqueta", "abrigo"}, {"chamarra", "abrigo"}, {"abrigo", "abrigo"}, {"saco", "abrigo"},
        {"blazer", "abrigo"}, {"gabardina", "abrigo"}, {"cazadora", "abrigo"}, {"campera", "abrigo"},
        {"impermeable", "abrigo"}, {"parka", "abrigo"}, {"chaleco", "abrigo"}};
    return m;
}

const std::map<std::string, std::string>& colores() {
    static const std::map<std::string, std::string> m = {
        {"azul", "azul"}, {"negro", "negro"}, {"negra", "negro"}, {"blanco", "blanco"}, {"blanca", "blanco"},
        {"verde", "verde"}, {"rojo", "rojo"}, {"roja", "rojo"}, {"gris", "gris"}, {"beige", "beige"},
        {"morado", "morado"}, {"morada", "morado"}, {"rosado", "rosado"}, {"rosada", "rosado"},
        {"rosa", "rosado"}, {"amarillo", "amarillo"}, {"amarilla", "amarillo"}, {"naranja", "naranja"},
        {"marron", "marron"}, {"cafe", "marron"}, {"celeste", "celeste"}, {"crema", "crema"}, {"vino", "vino"}};
    return m;
}

const std::map<std::string, std::string>& estilos() {
    static const std::map<std::string, std::string> m = {
        {"formal", "formal"}, {"elegante", "formal"}, {"informal", "informal"}, {"casual", "informal"},
        {"deportivo", "informal"}, {"deportiva", "informal"}};
    return m;
}

                                                                                                 
std::string buscar(const std::map<std::string, std::string>& tabla, const std::string& p) {
    if (auto it = tabla.find(p); it != tabla.end()) return it->second;
    if (p.size() > 3 && p.back() == 's') {
        if (auto it = tabla.find(p.substr(0, p.size() - 1)); it != tabla.end()) return it->second;
        if (p.size() > 4 && p.compare(p.size() - 2, 2, "es") == 0)
            if (auto it = tabla.find(p.substr(0, p.size() - 2)); it != tabla.end()) return it->second;
    }
    return "";
}

}              

std::string categoriaDePalabra(const std::string& palabra) { return buscar(categorias(), palabra); }
std::string colorDePalabra(const std::string& palabra) { return buscar(colores(), palabra); }
std::string estiloDePalabra(const std::string& palabra) { return buscar(estilos(), palabra); }

std::string estiloEnTexto(const std::string& frase) {
    for (const std::string& palabra : texto::dividir(texto::normalizar(frase)))
        if (auto e = estiloDePalabra(palabra); !e.empty()) return e;
    return "";
}

}                     
