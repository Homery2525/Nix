// Outfit.hpp - Motor de recomendación de outfits a partir de las prendas reales del closet.
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Modelos.hpp"

struct Outfit {
    std::string estilo;                                  // formal | informal | "" (sin filtrar)
    std::optional<Prenda> superior, inferior, calzado, abrigo;
    std::vector<std::string> faltantes;                  // p. ej. «calzado formal»
    bool completo() const { return superior && inferior && calzado; }
    bool vacio() const { return !superior && !inferior && !calzado && !abrigo; }
};

// Función pura: no toca archivos ni el reloj, así se prueba sin preparar nada.
//  - filtra por estilo (si se indica) y arma superior + inferior + calzado (+ abrigo si hace falta);
//  - prefiere el color favorito y combina con neutros: evita repetir color entre superior e inferior/abrigo;
//  - a igualdad de puntaje gana la prenda más antigua del closet (id menor), así el resultado es estable.
Outfit armarOutfit(const std::vector<Prenda>& closet, const std::string& estilo,
                   const std::string& colorPreferido = "", bool necesitaAbrigo = false);

// «camisa azul, pantalón gris y zapatos negros» (con abrigo: «… y, por encima, chaqueta gris»).
std::string describirOutfit(const Outfit& outfit);
