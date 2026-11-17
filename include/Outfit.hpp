                                                                                            
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Modelos.hpp"

struct Outfit {
    std::string estilo;                                                                         
    std::optional<Prenda> superior, inferior, calzado, abrigo;
    std::vector<std::string> faltantes;                                            
    bool completo() const { return superior && inferior && calzado; }
    bool vacio() const { return !superior && !inferior && !calzado && !abrigo; }
};

                                                                               
                                                                                                     
                                                                                                             
                                                                                                          
Outfit armarOutfit(const std::vector<Prenda>& closet, const std::string& estilo,
                   const std::string& colorPreferido = "", bool necesitaAbrigo = false);

                                                                                                
std::string describirOutfit(const Outfit& outfit);
