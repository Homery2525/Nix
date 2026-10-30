#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace texto {

// Texto normalizado + mapa hacia el texto original, para poder recuperar
// las palabras con tildes y mayúsculas al extraer el "asunto".
struct Normalizado {
    std::string s;                                  // minúsculas, sin tildes ni puntuación
    std::vector<std::pair<size_t, size_t>> origen;  // por cada char de s: (offset, longitud) en el original
};

inline Normalizado normalizarConMapa(const std::string& in) {
    Normalizado r;
    auto push = [&](char c, size_t off, size_t len) {
        if (c == ' ' && (r.s.empty() || r.s.back() == ' ')) return;  // colapsa espacios
        r.s.push_back(c);
        r.origen.emplace_back(off, len);
    };
    for (size_t i = 0; i < in.size();) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c == 0xC3 && i + 1 < in.size()) {
            unsigned char d = static_cast<unsigned char>(in[i + 1]);
            char m = ' ';
            switch (d) {
                case 0xA1: case 0x81: m = 'a'; break;  // á Á
                case 0xA9: case 0x89: m = 'e'; break;  // é É
                case 0xAD: case 0x8D: m = 'i'; break;  // í Í
                case 0xB3: case 0x93: m = 'o'; break;  // ó Ó
                case 0xBA: case 0x9A: case 0xBC: case 0x9C: m = 'u'; break;  // ú Ú ü Ü
                case 0xB1: case 0x91: m = 'n'; break;  // ñ Ñ
                default: break;
            }
            push(m, i, 2);
            i += 2;
            continue;
        }
        if (c < 0x80) {
            bool entreDigitos = i > 0 && i + 1 < in.size() &&
                                std::isdigit(static_cast<unsigned char>(in[i - 1])) &&
                                std::isdigit(static_cast<unsigned char>(in[i + 1]));
            if (std::isalnum(c)) push(static_cast<char>(std::tolower(c)), i, 1);
            else if ((c == ':' || c == '.') && entreDigitos) push(':', i, 1);  // 4:30 / 4.30
            else if (c == '.' || c == '\'') { /* "p.m." -> "pm" */ }
            else push(' ', i, 1);
            ++i;
            continue;
        }
        size_t len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;  // ¿ ¡ emojis...
        push(' ', i, len);
        i += len;
    }
    if (!r.s.empty() && r.s.back() == ' ') { r.s.pop_back(); r.origen.pop_back(); }
    return r;
}

inline std::string normalizar(const std::string& s) { return normalizarConMapa(s).s; }

inline std::vector<std::string> dividir(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ' ') { if (!cur.empty()) out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline bool contiene(std::string_view norm, std::string_view expr) {
    if (expr.empty()) return false;
    for (size_t pos = norm.find(expr); pos != std::string_view::npos; pos = norm.find(expr, pos + 1)) {
        const size_t fin = pos + expr.size();
        const bool izquierda = pos == 0 || norm[pos - 1] == ' ';
        const bool derecha = fin == norm.size() || norm[fin] == ' ';
        if (izquierda && derecha) return true;
    }
    return false;
}

}
