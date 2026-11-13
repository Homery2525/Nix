#include "ClosetApi.hpp"

Json prendaJson(const Prenda& p) {
    Json o = Json::objeto();
    o["id"] = p.id();
    o["nombre"] = p.nombre();
    o["categoria"] = p.categoria();
    o["color"] = p.color();
    o["estilo"] = p.estilo();
    return o;
}

Json closetJson(const std::vector<Prenda>& prendas) {
    Json lista = Json::lista();
    for (const Prenda& p : prendas) lista.push(prendaJson(p));
    Json j = Json::objeto();
    j["prendas"] = lista;
    return j;
}

Json outfitJson(const Outfit& o) {
    Json j = Json::objeto();
    j["estilo"] = o.estilo;
    for (const auto& [clave, prenda] : {std::pair<const char*, const std::optional<Prenda>*>{"superior", &o.superior},
                                         {"inferior", &o.inferior}, {"calzado", &o.calzado}, {"abrigo", &o.abrigo}})
        j[clave] = *prenda ? prendaJson(**prenda) : Json();
    Json faltan = Json::lista();
    for (const std::string& f : o.faltantes) faltan.push(f);
    j["faltantes"] = faltan;
    j["completo"] = o.completo();
    j["texto"] = describirOutfit(o);
    return j;
}
