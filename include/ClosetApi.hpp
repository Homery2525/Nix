                                                                                           
#pragma once
#include <vector>

#include "Json.hpp"
#include "Modelos.hpp"
#include "Outfit.hpp"

Json prendaJson(const Prenda& p);                                                                   
Json closetJson(const std::vector<Prenda>& prendas);                           
Json outfitJson(const Outfit& o);                                                                                                      
