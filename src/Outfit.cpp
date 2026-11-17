#include "Outfit.hpp"

#include <set>

namespace {

bool neutro(const std::string& color) {
    static const std::set<std::string> n = {"negro", "blanco", "gris", "beige", "crema", "marron"};
    return n.count(color) > 0;
}

std::optional<Prenda> mejor(const std::vector<Prenda>& closet, const std::string& categoria, const std::string& estilo,
                            const std::string& colorPreferido, const std::string& colorSuperior) {
    const Prenda* elegida = nullptr;
    int mejorPuntaje = -1000;
    for (const Prenda& p : closet) {                                    
        if (p.categoria() != categoria || (!estilo.empty() && p.estilo() != estilo)) continue;
        int puntaje = 0;
        if (!colorPreferido.empty() && p.color() == colorPreferido) puntaje += categoria == "superior" ? 3 : 1;
        if (categoria != "superior") {
            if (neutro(p.color())) puntaje += 2;
            if (!colorSuperior.empty() && p.color() == colorSuperior && !neutro(p.color())) puntaje -= 3;
        }
        if (puntaje > mejorPuntaje) { mejorPuntaje = puntaje; elegida = &p; }
    }
    if (!elegida) return std::nullopt;
    return *elegida;
}

std::string etiquetaFaltante(const std::string& categoria, const std::string& estilo) {
    const std::string base = categoria == "superior" ? "parte de arriba" : categoria == "inferior" ? "parte de abajo" : categoria;
    return estilo.empty() ? base : base + " " + estilo;
}

}              

Outfit armarOutfit(const std::vector<Prenda>& closet, const std::string& estilo, const std::string& colorPreferido,
                   bool necesitaAbrigo) {
    Outfit o;
    o.estilo = estilo;
    o.superior = mejor(closet, "superior", estilo, colorPreferido, "");
    const std::string colorSup = o.superior ? o.superior->color() : "";
    o.inferior = mejor(closet, "inferior", estilo, colorPreferido, colorSup);
    o.calzado = mejor(closet, "calzado", estilo, colorPreferido, colorSup);
    if (necesitaAbrigo) o.abrigo = mejor(closet, "abrigo", estilo, colorPreferido, colorSup);
    if (!o.superior) o.faltantes.push_back(etiquetaFaltante("superior", estilo));
    if (!o.inferior) o.faltantes.push_back(etiquetaFaltante("inferior", estilo));
    if (!o.calzado) o.faltantes.push_back(etiquetaFaltante("calzado", estilo));
    if (necesitaAbrigo && !o.abrigo) o.faltantes.push_back(etiquetaFaltante("abrigo", estilo));
    return o;
}

std::string describirOutfit(const Outfit& o) {
    std::vector<std::string> piezas;
    for (const auto* p : {&o.superior, &o.inferior, &o.calzado})
        if (*p) piezas.push_back((*p)->nombre());
    std::string r;
    for (size_t i = 0; i < piezas.size(); ++i)
        r += (i == 0 ? "" : i + 1 == piezas.size() ? " y " : ", ") + piezas[i];
    if (o.abrigo) r += (r.empty() ? "" : " y, por encima, ") + o.abrigo->nombre();
    return r;
}
