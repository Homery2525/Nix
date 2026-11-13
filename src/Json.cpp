#include "Json.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

const Json NULO;

void escribirTexto(const std::string& s, std::string& out) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char b[8];
                    std::snprintf(b, sizeof b, "\\u%04x", c);
                    out += b;
                } else {
                    out += static_cast<char>(c);  // el UTF-8 pasa tal cual
                }
        }
    }
    out += '"';
}

struct Lector {
    const std::string& s;
    size_t i = 0;
    int profundidad = 0;

    void espacios() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i; }
    bool lee(char c) { espacios(); if (i < s.size() && s[i] == c) { ++i; return true; } return false; }

    static void utf8(unsigned cp, std::string& out) {
        if (cp < 0x80) out += static_cast<char>(cp);
        else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18)); out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    bool hex4(unsigned& v) {
        if (i + 4 > s.size()) return false;
        v = 0;
        for (int k = 0; k < 4; ++k) {
            char c = s[i + k];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= c - '0';
            else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
            else return false;
        }
        i += 4;
        return true;
    }

    std::optional<std::string> texto() {
        if (!lee('"')) return std::nullopt;
        std::string out;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') return out;
            if (c != '\\') { out += c; continue; }
            if (i >= s.size()) return std::nullopt;
            char e = s[i++];
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned cp;
                    if (!hex4(cp)) return std::nullopt;
                    if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                        i += 2;
                        unsigned lo;
                        if (!hex4(lo) || lo < 0xDC00 || lo > 0xDFFF) return std::nullopt;
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    }
                    utf8(cp, out);
                    break;
                }
                default: return std::nullopt;
            }
        }
        return std::nullopt;
    }

    std::optional<Json> valor() {
        espacios();
        if (i >= s.size() || ++profundidad > 64) return std::nullopt;
        struct Salir { int& p; ~Salir() { --p; } } salir{profundidad};
        char c = s[i];
        if (c == '{') {
            ++i;
            Json o = Json::objeto();
            if (lee('}')) return o;
            while (true) {
                auto k = texto();
                if (!k || !lee(':')) return std::nullopt;
                auto v = valor();
                if (!v) return std::nullopt;
                o[*k] = std::move(*v);
                if (lee(',')) continue;
                if (lee('}')) return o;
                return std::nullopt;
            }
        }
        if (c == '[') {
            ++i;
            Json l = Json::lista();
            if (lee(']')) return l;
            while (true) {
                auto v = valor();
                if (!v) return std::nullopt;
                l.push(std::move(*v));
                if (lee(',')) continue;
                if (lee(']')) return l;
                return std::nullopt;
            }
        }
        if (c == '"') { auto t = texto(); if (!t) return std::nullopt; return Json(*t); }
        if (s.compare(i, 4, "true") == 0) { i += 4; return Json(true); }
        if (s.compare(i, 5, "false") == 0) { i += 5; return Json(false); }
        if (s.compare(i, 4, "null") == 0) { i += 4; return Json(); }
        char* fin = nullptr;
        double n = std::strtod(s.c_str() + i, &fin);
        if (fin == s.c_str() + i) return std::nullopt;
        i = static_cast<size_t>(fin - s.c_str());
        return Json(n);
    }
};

}  // namespace

const Json& Json::at(const std::string& clave) const {
    if (tipo_ == Tipo::Objeto)
        for (size_t k = 0; k < claves_.size(); ++k)
            if (claves_[k] == clave) return valores_[k];
    return NULO;
}

const Json& Json::at(size_t i) const { return tipo_ == Tipo::Lista && i < lista_.size() ? lista_[i] : NULO; }

bool Json::contiene(const std::string& clave) const {
    if (tipo_ == Tipo::Objeto)
        for (const auto& k : claves_)
            if (k == clave) return true;
    return false;
}

Json& Json::operator[](const std::string& clave) {
    if (tipo_ != Tipo::Objeto) { *this = Json::objeto(); }
    for (size_t k = 0; k < claves_.size(); ++k)
        if (claves_[k] == clave) return valores_[k];
    claves_.push_back(clave);
    valores_.emplace_back();
    return valores_.back();
}

Json& Json::push(Json v) {
    if (tipo_ != Tipo::Lista) { *this = Json::lista(); }
    lista_.push_back(std::move(v));
    return lista_.back();
}

std::string Json::dump() const {
    std::string out;
    switch (tipo_) {
        case Tipo::Nulo: out = "null"; break;
        case Tipo::Bool: out = b_ ? "true" : "false"; break;
        case Tipo::Numero: {
            char b[40];
            if (std::isfinite(n_) && n_ == std::floor(n_) && std::fabs(n_) < 1e15) std::snprintf(b, sizeof b, "%.0f", n_);
            else if (std::isfinite(n_)) std::snprintf(b, sizeof b, "%.15g", n_);
            else std::snprintf(b, sizeof b, "null");
            out = b;
            break;
        }
        case Tipo::Texto: escribirTexto(s_, out); break;
        case Tipo::Lista:
            out = "[";
            for (size_t k = 0; k < lista_.size(); ++k) out += (k ? "," : "") + lista_[k].dump();
            out += "]";
            break;
        case Tipo::Objeto:
            out = "{";
            for (size_t k = 0; k < claves_.size(); ++k) {
                if (k) out += ",";
                escribirTexto(claves_[k], out);
                out += ":" + valores_[k].dump();
            }
            out += "}";
            break;
    }
    return out;
}

std::optional<Json> Json::parse(const std::string& texto) {
    Lector l{texto};
    auto v = l.valor();
    if (!v) return std::nullopt;
    l.espacios();
    if (l.i != texto.size()) return std::nullopt;
    return v;
}
