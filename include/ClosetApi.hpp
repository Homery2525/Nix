// ClosetApi.hpp - Conversión del closet y de los outfits a JSON para la API y la interfaz.
#pragma once
#include <vector>

#include "Json.hpp"
#include "Modelos.hpp"
#include "Outfit.hpp"

Json prendaJson(const Prenda& p);                          // {id, nombre, categoria, color, estilo}
Json closetJson(const std::vector<Prenda>& prendas);       // {"prendas":[...]}
Json outfitJson(const Outfit& o);                          // {estilo, superior, inferior, calzado, abrigo, faltantes, completo, texto}
