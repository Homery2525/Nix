// Closet.hpp - Vocabulario de prendas (categorías, colores y estilos) para el NLU y el motor de outfit.
#pragma once
#include <string>

namespace closet {

// Cada función recibe una palabra YA normalizada (minúsculas, sin tildes) y acepta plurales y género.
// Devuelven "" si la palabra no pertenece al vocabulario.
std::string categoriaDePalabra(const std::string& palabra);  // superior | inferior | calzado | abrigo
std::string colorDePalabra(const std::string& palabra);      // forma canónica masculina: «blanca» -> «blanco»
std::string estiloDePalabra(const std::string& palabra);     // formal | informal («casual» -> informal)

// Texto de una frase libre: primer estilo que aparezca («es formal», «algo casual»).
std::string estiloEnTexto(const std::string& frase);

}  // namespace closet
