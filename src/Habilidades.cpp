#include "Habilidades.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <unordered_map>

namespace habilidades {

namespace {

const char* DIAS[] = {"lunes", "martes", "miércoles", "jueves", "viernes", "sábado", "domingo"};
const char* MESES[] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre",
                       "octubre", "noviembre", "diciembre"};
const char* MESES_NORM[] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre",
                            "octubre", "noviembre", "diciembre"};
const char* DIAS_NORM[] = {"lunes", "martes", "miercoles", "jueves", "viernes", "sabado", "domingo"};

                                                                                        
const std::set<std::string> VERBOS_ORDEN = {
    "agenda", "agendar", "agendame", "recuerdame", "recordar", "recuerda", "anota", "anotar", "apunta", "programa",
    "programar", "completa", "completar", "elimina", "eliminar", "borra", "borrar", "cambia", "cambiar", "mueve",
    "mover", "posponer", "reprograma", "deshaz", "crea", "crear"};
const std::set<std::string> RELLENO = {"jarvis", "nix", "por", "favor", "oye", "hey", "puedes", "podrias", "quiero", "me", "dime", "ahora"};

                                                            

std::string quitarAcentos(const std::string& s) {                                                
    std::string out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) { out += static_cast<char>(std::tolower(c)); ++i; continue; }
        if (c == 0xC3 && i + 1 < s.size()) {
            unsigned char d = static_cast<unsigned char>(s[i + 1]);
            char m = ' ';
            switch (d) {
                case 0xA1: case 0x81: case 0xA0: case 0x80: case 0xA4: case 0x84: m = 'a'; break;
                case 0xA9: case 0x89: case 0xA8: case 0x88: m = 'e'; break;
                case 0xAD: case 0x8D: m = 'i'; break;
                case 0xB3: case 0x93: case 0xB2: case 0x92: case 0xB6: case 0x96: m = 'o'; break;
                case 0xBA: case 0x9A: case 0xBC: case 0x9C: m = 'u'; break;
                case 0xB1: case 0x91: m = 'n'; break;
                case 0x97: m = '*'; break;
                case 0xB7: m = '/'; break;
                default: break;
            }
            out += m;
            i += 2;
            continue;
        }
        if (c == 0xE2 && i + 2 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x88 &&
            static_cast<unsigned char>(s[i + 2]) == 0x92) { out += '-'; i += 3; continue; }
        size_t len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        out += ' ';
        i += len;
    }
    return out;
}

std::string colapsar(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == ' ') { if (!out.empty() && out.back() != ' ') out += c; }
        else out += c;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

std::string normCalc(const std::string& s) {                                      
    std::string t = quitarAcentos(s), out;
    for (char c : t) out += (std::isalnum(static_cast<unsigned char>(c)) || std::string("%+-*/^().,=").find(c) != std::string::npos) ? c : ' ';
    return colapsar(out);
}

std::vector<std::string> palabras(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ' ') { if (!cur.empty()) out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string unir(const std::vector<std::string>& v, const std::string& sep, size_t desde = 0, size_t hasta = std::string::npos) {
    std::string out;
    hasta = std::min(hasta, v.size());
    for (size_t i = desde; i < hasta; ++i) out += (i > desde ? sep : "") + v[i];
    return out;
}

std::string sinRelleno(const std::string& t) {
    auto p = palabras(t);
    size_t i = 0;
    while (i < p.size() && RELLENO.count(p[i])) ++i;
    return unir(p, " ", i);
}

bool empiezaPor(const std::string& s, const std::string& pre) { return s.compare(0, pre.size(), pre) == 0; }

std::string titulo(const std::string& s) {                                                       
    std::string out = s;
    bool inicio = true;
    for (char& c : out) {
        if (c == ' ') { inicio = true; continue; }
        if (inicio && c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
        inicio = false;
    }
    return out;
}

                                                                                                 
                                                                                         
const std::regex& rx(const std::string& patron, bool icase = false) {
    thread_local std::unordered_map<std::string, std::regex> cache;
    const std::string clave = (icase ? "i:" : "s:") + patron;
    auto it = cache.find(clave);
    if (it == cache.end())
        it = cache.emplace(clave, std::regex(patron, icase ? std::regex::icase | std::regex::ECMAScript : std::regex::ECMAScript)).first;
    return it->second;
}

std::string reemplazar(const std::string& s, const char* patron, const char* formato) {
    return std::regex_replace(s, rx(patron), formato);
}

bool busca(const std::string& s, const char* patron, std::smatch& m, bool icase = false) {
    return std::regex_search(s, m, rx(patron, icase));
}

bool busca(std::string&&, const char*, std::smatch&, bool = false) = delete;                                         

bool coincide(const std::string& s, const char* patron) {
    std::smatch m;
    return busca(s, patron, m);
}

std::string horaHablada(int h, int m) {
    int h12 = h % 12 == 0 ? 12 : h % 12;
    std::string t = std::to_string(h12);
    if (m) { char b[8]; std::snprintf(b, sizeof b, ":%02d", m); t += b; }
    if (h == 12 && m == 0) return t + " del mediodía";
    return t + (h < 6 ? " de la madrugada" : h < 12 ? " de la mañana" : h < 19 ? " de la tarde" : " de la noche");
}

std::string numeroHablado(double x) {
    if (std::fabs(x - std::round(x)) < 1e-9 && std::fabs(x) < 1e15) {
        char b[32];
        std::snprintf(b, sizeof b, "%.0f", std::round(x));
        return b;
    }
    char b[48];
    std::snprintf(b, sizeof b, "%.6f", x);
    std::string s = b;
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    std::replace(s.begin(), s.end(), '.', ',');
    return s;
}

Resultado res(const std::string& texto, const char* habilidad) {
    Resultado r;
    r.respuesta = texto;
    r.habilidad = habilidad;
    return r;
}

                                                                                                         

long diasDesdeCivil(int y, unsigned m, unsigned d) {                                
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long>(doe) - 719468;
}

void civilDesdeDias(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long yy = static_cast<long>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    y = static_cast<int>(yy + (m <= 2));
}

int diaSemanaDe(long dias) { return static_cast<int>((((dias % 7) + 7) + 3) % 7); }                                      

bool bisiesto(int a) { return a % 4 == 0 && (a % 100 != 0 || a % 400 == 0); }

std::string fechaLarga(long dias) {                                     
    int y, m, d;
    civilDesdeDias(dias, y, m, d);
    return std::string(DIAS[diaSemanaDe(dias)]) + " " + std::to_string(d) + " de " + MESES[m - 1] + " de " + std::to_string(y);
}

long hoyEn(Contexto& ctx) {
    FechaLocal f = fechaEn(ctx.ahora(), ctx.tz);
    return diasDesdeCivil(f.anio, static_cast<unsigned>(f.mes), static_cast<unsigned>(f.dia));
}

std::mutex& mutexZona() { static std::mutex m; return m; }

                                                                                 

const std::unordered_map<std::string, int> UNI = {
    {"cero", 0}, {"un", 1}, {"uno", 1}, {"una", 1}, {"dos", 2}, {"tres", 3}, {"cuatro", 4}, {"cinco", 5}, {"seis", 6},
    {"siete", 7}, {"ocho", 8}, {"nueve", 9}, {"diez", 10}, {"once", 11}, {"doce", 12}, {"trece", 13}, {"catorce", 14},
    {"quince", 15}, {"dieciseis", 16}, {"diecisiete", 17}, {"dieciocho", 18}, {"diecinueve", 19}, {"veinte", 20},
    {"veintiun", 21}, {"veintiuno", 21}, {"veintiuna", 21}, {"veintidos", 22}, {"veintitres", 23}, {"veinticuatro", 24},
    {"veinticinco", 25}, {"veintiseis", 26}, {"veintisiete", 27}, {"veintiocho", 28}, {"veintinueve", 29}};
const std::unordered_map<std::string, int> DEC = {{"treinta", 30}, {"cuarenta", 40}, {"cincuenta", 50}, {"sesenta", 60},
                                                  {"setenta", 70}, {"ochenta", 80}, {"noventa", 90}};
const std::unordered_map<std::string, int> CEN = {
    {"cien", 100}, {"ciento", 100}, {"doscientos", 200}, {"trescientos", 300}, {"cuatrocientos", 400}, {"quinientos", 500},
    {"seiscientos", 600}, {"setecientos", 700}, {"ochocientos", 800}, {"novecientos", 900}};

bool leerNumero(const std::vector<std::string>& t, size_t i, long long& valor, size_t& sig) {
    long long total = 0, actual = 0;
    size_t j = i;
    std::string ultimo;
    bool visto = false;
    while (j < t.size()) {
        const std::string& w = t[j];
        if (UNI.count(w)) {
            if (ultimo == "uni") break;
            actual += UNI.at(w); ultimo = "uni";
        } else if (DEC.count(w)) {
            if (ultimo == "uni" || ultimo == "dec") break;
            actual += DEC.at(w); ultimo = "dec";
        } else if (CEN.count(w)) {
            if (ultimo == "uni" || ultimo == "dec" || ultimo == "cen") break;
            actual += CEN.at(w); ultimo = "cen";
        } else if (w == "y" && ultimo == "dec" && j + 1 < t.size() && UNI.count(t[j + 1]) && UNI.at(t[j + 1]) > 0 && UNI.at(t[j + 1]) < 10) {
            ++j;
            continue;
        } else if (w == "mil") {
            total += (actual ? actual : 1) * 1000; actual = 0; ultimo = "mil";
        } else if (w == "millon" || w == "millones") {
            total += (actual ? actual : 1) * 1000000; actual = 0; ultimo = "mill";
        } else {
            break;
        }
        visto = true;
        ++j;
    }
    if (!visto) return false;
    valor = total + actual;
    sig = j;
    return true;
}

std::string palabrasADigitos(const std::string& t) {                                    
    auto tokens = palabras(t);
    std::vector<std::string> salida;
    for (size_t i = 0; i < tokens.size();) {
        if (tokens[i] == "ciento" && i > 0 && tokens[i - 1] == "por") {                           
            salida.push_back(tokens[i]);
            ++i;
            continue;
        }
        long long v = 0;
        size_t sig = i;
        if (leerNumero(tokens, i, v, sig)) { salida.push_back(std::to_string(v)); i = sig; }
        else { salida.push_back(tokens[i]); ++i; }
    }
    return unir(salida, " ");
}

                                                                  

enum class ErrCalc { Ok, DivCero, Invalido };

struct Evaluador {                                                                              
    const std::string& s;
    size_t i = 0;
    ErrCalc err = ErrCalc::Ok;

    bool fin() const { return i >= s.size(); }
    bool consume(const char* t) { size_t n = std::strlen(t); if (s.compare(i, n, t) == 0) { i += n; return true; } return false; }

    double expresion() {
        double v = termino();
        while (err == ErrCalc::Ok && !fin()) {
            if (s[i] == '+') { ++i; v += termino(); }
            else if (s[i] == '-') { ++i; v -= termino(); }
            else break;
        }
        return v;
    }
    double termino() {
        double v = unario();
        while (err == ErrCalc::Ok && !fin()) {
            if (s.compare(i, 2, "**") == 0) break;
            if (s[i] == '*') { ++i; v *= unario(); }
            else if (s[i] == '/') {
                ++i;
                double d = unario();
                if (d == 0) { err = ErrCalc::DivCero; return 0; }
                v /= d;
            } else break;
        }
        return v;
    }
    double unario() {
        if (!fin() && s[i] == '-') { ++i; return -unario(); }
        if (!fin() && s[i] == '+') { ++i; return unario(); }
        return potencia();
    }
    double potencia() {
        double base = atomo();
        if (err == ErrCalc::Ok && consume("**")) {
            double e = unario();
            if (std::fabs(e) > 1000 || std::fabs(base) > 1e12) { err = ErrCalc::Invalido; return 0; }
            return std::pow(base, e);
        }
        return base;
    }
    double atomo() {
        if (fin()) { err = ErrCalc::Invalido; return 0; }
        if (s[i] == '(') {
            ++i;
            double v = expresion();
            if (fin() || s[i] != ')') { err = ErrCalc::Invalido; return 0; }
            ++i;
            return v;
        }
        if (consume("sqrt(") || consume("cbrt(")) {
            bool raiz = s[i - 5] == 's';
            double v = expresion();
            if (fin() || s[i] != ')') { err = ErrCalc::Invalido; return 0; }
            ++i;
            if (raiz) { if (v < 0) { err = ErrCalc::Invalido; return 0; } return std::sqrt(v); }
            return std::copysign(std::cbrt(std::fabs(v)), v);
        }
        if (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.') {
            char* f = nullptr;
            double v = std::strtod(s.c_str() + i, &f);
            if (f == s.c_str() + i) { err = ErrCalc::Invalido; return 0; }
            i = static_cast<size_t>(f - s.c_str());
            return v;
        }
        err = ErrCalc::Invalido;
        return 0;
    }
};

ErrCalc evaluar(const std::string& expr, double& valor) {
    Evaluador e{expr};
    valor = e.expresion();
    if (e.err == ErrCalc::Ok && !e.fin()) e.err = ErrCalc::Invalido;
    if (e.err == ErrCalc::Ok && !std::isfinite(valor)) e.err = ErrCalc::Invalido;
    return e.err;
}

std::optional<std::string> aExpresion(const std::string& entrada) {
    std::string t = palabrasADigitos(entrada);
    t = reemplazar(t, R"RX((\d)\s+coma\s+(\d))RX", "$1.$2");
    t = reemplazar(t, R"RX((\d),(\d))RX", "$1.$2");
    const char* num = R"RX((-?(?:\d+(?:\.\d+)?|\.\d+)))RX";
    auto con = [&](const std::string& antes, const std::string& despues = "") { return antes + num + despues; };
    t = std::regex_replace(t, rx(con("", R"RX(\s*(?:por ciento|%)\s*de\s*)RX") + num), "($1/100*$2)");
    t = std::regex_replace(t, rx(con("raiz cuadrada de\\s*")), "sqrt($1)");
    t = std::regex_replace(t, rx(con("raiz cubica de\\s*")), "cbrt($1)");
    t = std::regex_replace(t, rx(con("raiz de\\s*")), "sqrt($1)");
    t = std::regex_replace(t, rx(con("la mitad de\\s*")), "($1/2)");
    t = std::regex_replace(t, rx(con("el doble de\\s*")), "($1*2)");
    t = std::regex_replace(t, rx(con("el triple de\\s*")), "($1*3)");
    t = std::regex_replace(t, rx(con("", "\\s*al cuadrado")), "($1**2)");
    t = std::regex_replace(t, rx(con("", "\\s*al cubo")), "($1**3)");
    t = reemplazar(t, R"RX(\b(?:elevado a la|elevado a)\b)RX", "**");
    t = reemplazar(t, R"RX(\bmultiplicado por\b|\bveces\b|\bpor\b)RX", "*");
    t = reemplazar(t, R"RX(\bdividido entre\b|\bdividido por\b|\bdividido\b|\bentre\b)RX", "/");
    t = reemplazar(t, R"RX(\bmas\b)RX", "+");
    t = reemplazar(t, R"RX(\bmenos\b)RX", "-");
    for (int k = 0; k < 4; ++k) t = reemplazar(t, R"RX((\d)\s*x\s*(\d))RX", "$1*$2");
    std::replace(t.begin(), t.end(), '^', '\x01');
    t = reemplazar(t, "\x01", "**");
    t = reemplazar(t, R"RX(\b(?:es|son|igual a|a|de|el|la|los|las|y)\b)RX", " ");
    std::string compacto;
    for (char c : t) if (c != ' ') compacto += c;
    if (compacto.empty() || compacto.find('%') != std::string::npos) return std::nullopt;
    if (!coincide(compacto, R"RX(\d)RX") || !coincide(compacto, R"RX([+\-*/]|sqrt|cbrt)RX")) return std::nullopt;
    return compacto;
}

constexpr size_t GRADO_POLINOMIO_MAX = 20;
using Polinomio = std::vector<double>;

void reducir(Polinomio& p) {
    while (p.size() > 1 && p.back() == 0) p.pop_back();
}

std::optional<Polinomio> sumarPolinomios(const Polinomio& a, const Polinomio& b, double signo = 1) {
    Polinomio r(std::max(a.size(), b.size()), 0);
    for (size_t i = 0; i < r.size(); ++i) {
        r[i] = (i < a.size() ? a[i] : 0) + signo * (i < b.size() ? b[i] : 0);
        if (!std::isfinite(r[i])) return std::nullopt;
    }
    reducir(r);
    return r;
}

std::optional<Polinomio> multiplicarPolinomios(const Polinomio& a, const Polinomio& b) {
    if (a.size() + b.size() - 2 > GRADO_POLINOMIO_MAX) return std::nullopt;
    Polinomio r(a.size() + b.size() - 1, 0);
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < b.size(); ++j) {
            r[i + j] += a[i] * b[j];
            if (!std::isfinite(r[i + j])) return std::nullopt;
        }
    }
    reducir(r);
    return r;
}

struct AnalizadorPolinomio {
    const std::string& s;
    size_t i = 0;
    bool valido = true;

    bool fin() const { return i >= s.size(); }

    Polinomio expresion() {
        Polinomio r = termino();
        while (valido && !fin() && (s[i] == '+' || s[i] == '-')) {
            const double signo = s[i++] == '+' ? 1 : -1;
            auto suma = sumarPolinomios(r, termino(), signo);
            if (!suma) { valido = false; return {0}; }
            r = std::move(*suma);
        }
        return r;
    }

    Polinomio termino() {
        Polinomio r = unario();
        while (valido && !fin()) {
            if (s[i] == '*') {
                ++i;
                if (!fin() && s[i] == '*') ++i;
                auto producto = multiplicarPolinomios(r, unario());
                if (!producto) { valido = false; return {0}; }
                r = std::move(*producto);
            } else if (s[i] == '/') {
                ++i;
                Polinomio divisor = unario();
                reducir(divisor);
                if (divisor.size() != 1 || divisor[0] == 0) { valido = false; return {0}; }
                for (double& c : r) {
                    c /= divisor[0];
                    if (!std::isfinite(c)) { valido = false; return {0}; }
                }
            } else if (s[i] == 'x' || s[i] == '(') {
                auto producto = multiplicarPolinomios(r, unario());
                if (!producto) { valido = false; return {0}; }
                r = std::move(*producto);
            } else {
                break;
            }
        }
        return r;
    }

    Polinomio unario() {
        if (!fin() && (s[i] == '+' || s[i] == '-')) {
            const bool negativo = s[i++] == '-';
            Polinomio r = unario();
            if (negativo) for (double& c : r) c = -c;
            return r;
        }
        Polinomio r = atomo();
        if (!valido || fin() || (s[i] != '^' && s.compare(i, 2, "**") != 0)) return r;
        if (s.compare(i, 2, "**") == 0) i += 2;
        else ++i;
        bool negativo = false;
        if (!fin() && (s[i] == '+' || s[i] == '-')) negativo = s[i++] == '-';
        if (negativo || fin() || !std::isdigit(static_cast<unsigned char>(s[i]))) { valido = false; return {0}; }
        unsigned exponente = 0;
        while (!fin() && std::isdigit(static_cast<unsigned char>(s[i]))) {
            exponente = exponente * 10 + static_cast<unsigned>(s[i++] - '0');
            if (exponente > GRADO_POLINOMIO_MAX) { valido = false; return {0}; }
        }
        Polinomio resultado{1};
        for (unsigned n = 0; n < exponente && valido; ++n) {
            auto producto = multiplicarPolinomios(resultado, r);
            if (!producto) { valido = false; return {0}; }
            resultado = std::move(*producto);
        }
        return resultado;
    }

    Polinomio atomo() {
        if (fin()) { valido = false; return {0}; }
        if (s[i] == 'x') { ++i; return {0, 1}; }
        if (s[i] == '(') {
            ++i;
            Polinomio r = expresion();
            if (fin() || s[i] != ')') { valido = false; return {0}; }
            ++i;
            return r;
        }
        if (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.') {
            char* finNumero = nullptr;
            const double valor = std::strtod(s.c_str() + i, &finNumero);
            if (finNumero == s.c_str() + i || !std::isfinite(valor)) { valido = false; return {0}; }
            i = static_cast<size_t>(finNumero - s.c_str());
            return {valor};
        }
        valido = false;
        return {0};
    }
};

std::optional<Polinomio> parsearPolinomio(const std::string& expresion) {
    std::string compacta;
    for (char c : expresion) {
        if (c == ',') compacta += '.';
        else if (!std::isspace(static_cast<unsigned char>(c))) compacta += c;
    }
    AnalizadorPolinomio a{compacta};
    Polinomio p = a.expresion();
    if (!a.valido || !a.fin()) return std::nullopt;
    reducir(p);
    return p;
}

std::complex<double> evaluarPolinomio(const Polinomio& p, std::complex<double> x) {
    std::complex<double> valor = p.back();
    for (size_t i = p.size() - 1; i > 0; --i) valor = valor * x + p[i - 1];
    return valor;
}

std::complex<double> evaluarDerivada(const Polinomio& p, std::complex<double> x) {
    std::complex<double> valor = static_cast<double>(p.size() - 1) * p.back();
    for (size_t i = p.size() - 1; i > 1; --i) valor = valor * x + static_cast<double>(i - 1) * p[i - 1];
    return valor;
}

bool resolverRaicesSimples(const Polinomio& p, std::vector<std::complex<double>>& raices);

bool resolverRaices(const Polinomio& p, std::vector<std::complex<double>>& raices) {
    const size_t grado = p.size() - 1;
    if (grado == 0) return false;

    Polinomio derivada(grado);
    for (size_t i = 1; i < p.size(); ++i) derivada[i - 1] = static_cast<double>(i) * p[i];

    auto recortar = [](Polinomio& polinomio) {
        double escala = 0;
        for (double coeficiente : polinomio) escala = std::max(escala, std::fabs(coeficiente));
        const double tolerancia = 1e-10 * escala;
        while (!polinomio.empty() && std::fabs(polinomio.back()) <= tolerancia) polinomio.pop_back();
    };
    auto normalizar = [&recortar](Polinomio& polinomio) {
        double escala = 0;
        for (double coeficiente : polinomio) escala = std::max(escala, std::fabs(coeficiente));
        if (escala == 0) { polinomio.clear(); return; }
        for (double& coeficiente : polinomio) coeficiente /= escala;
        recortar(polinomio);
        if (polinomio.empty()) return;
        const double principal = polinomio.back();
        for (double& coeficiente : polinomio) coeficiente /= principal;
    };
    auto resto = [](Polinomio dividendo, const Polinomio& divisor) {
        double escala = 0;
        for (double coeficiente : dividendo) escala = std::max(escala, std::fabs(coeficiente));
        const double tolerancia = 1e-10 * escala;
        while (dividendo.size() >= divisor.size()) {
            const size_t desplazamiento = dividendo.size() - divisor.size();
            const double factor = dividendo.back() / divisor.back();
            for (size_t i = 0; i < divisor.size(); ++i)
                dividendo[i + desplazamiento] -= factor * divisor[i];
            dividendo.back() = 0;
            while (!dividendo.empty() && std::fabs(dividendo.back()) <= tolerancia) dividendo.pop_back();
        }
        return dividendo;
    };
    auto mcd = [&](Polinomio a, Polinomio b) {
        normalizar(a);
        normalizar(b);
        while (!b.empty()) {
            Polinomio r = resto(a, b);
            normalizar(r);
            a = std::move(b);
            b = std::move(r);
        }
        normalizar(a);
        return a;
    };

    Polinomio factorComun = mcd(p, std::move(derivada));
    Polinomio libreCuadrados = p;
    if (factorComun.size() > 1) {
        Polinomio cociente(p.size() - factorComun.size() + 1, 0);
        Polinomio residuo = p;
        double escala = 0;
        for (double coeficiente : p) escala = std::max(escala, std::fabs(coeficiente));
        while (residuo.size() >= factorComun.size()) {
            const size_t desplazamiento = residuo.size() - factorComun.size();
            const double factor = residuo.back() / factorComun.back();
            cociente[desplazamiento] = factor;
            for (size_t i = 0; i < factorComun.size(); ++i)
                residuo[i + desplazamiento] -= factor * factorComun[i];
            residuo.back() = 0;
            while (!residuo.empty() && residuo.back() == 0) residuo.pop_back();
        }
        double errorDivision = 0;
        for (double coeficiente : residuo) errorDivision = std::max(errorDivision, std::fabs(coeficiente));
        if (errorDivision > 1e-8 * escala) return false;
        recortar(cociente);
        libreCuadrados = std::move(cociente);
    }

    std::vector<std::complex<double>> raicesSimples;
    if (!resolverRaicesSimples(libreCuadrados, raicesSimples)) return false;

    std::vector<std::complex<double>> expandidas;
    expandidas.reserve(grado);
    for (const auto& raiz : raicesSimples) {
        Polinomio derivadaActual = p;
        size_t multiplicidad = 0;
        for (size_t orden = 0; orden < p.size(); ++orden) {
            const std::complex<double> valor = evaluarPolinomio(derivadaActual, raiz);
            double escala = 0;
            const double modulo = std::abs(raiz);
            for (auto it = derivadaActual.rbegin(); it != derivadaActual.rend(); ++it)
                escala = escala * modulo + std::fabs(*it);
            if (std::abs(valor) > 1e-8 * std::max(1.0, escala)) break;
            ++multiplicidad;
            if (derivadaActual.size() <= 1) break;
            Polinomio siguiente(derivadaActual.size() - 1);
            for (size_t i = 1; i < derivadaActual.size(); ++i)
                siguiente[i - 1] = static_cast<double>(i) * derivadaActual[i];
            derivadaActual = std::move(siguiente);
        }
        if (multiplicidad == 0) return false;
        expandidas.insert(expandidas.end(), multiplicidad, raiz);
    }
    if (expandidas.size() != grado) return false;
    raices = std::move(expandidas);
    return true;
}

bool resolverRaicesSimples(const Polinomio& p, std::vector<std::complex<double>>& raices) {
    const size_t grado = p.size() - 1;
    if (grado == 0 || std::fabs(p.back()) < std::numeric_limits<double>::min()) return false;
    double radio = 1;
    for (size_t i = 0; i < grado; ++i) {
        const double razon = std::fabs(p[i] / p.back());
        if (!std::isfinite(razon)) return false;
        radio = std::max(radio, 1 + razon);
    }
    if (radio > 1e8) return false;
    constexpr double PI = 3.14159265358979323846;
    raices.resize(grado);
    for (size_t i = 0; i < grado; ++i) {
        const double angulo = 2 * PI * (static_cast<double>(i) + 0.25) / static_cast<double>(grado);
        raices[i] = std::polar(radio, angulo);
    }
    bool converge = false;
    for (int iteracion = 0; iteracion < 4000; ++iteracion) {
        std::vector<std::complex<double>> nuevas = raices;
        double maxCambio = 0;
        for (size_t i = 0; i < grado; ++i) {
            const std::complex<double> derivada = evaluarDerivada(p, raices[i]);
            if (std::abs(derivada) < 1e-24) return false;
            const std::complex<double> correccionNewton = evaluarPolinomio(p, raices[i]) / derivada;
            std::complex<double> sumaInversa(0, 0);
            for (size_t j = 0; j < grado; ++j) {
                if (i == j) continue;
                const std::complex<double> diferencia = raices[i] - raices[j];
                if (std::abs(diferencia) < 1e-24) return false;
                sumaInversa += 1.0 / diferencia;
            }
            const std::complex<double> factor = 1.0 - correccionNewton * sumaInversa;
            if (std::abs(factor) < 1e-24) return false;
            nuevas[i] = raices[i] - correccionNewton / factor;
            if (!std::isfinite(nuevas[i].real()) || !std::isfinite(nuevas[i].imag())) return false;
            maxCambio = std::max(maxCambio, std::abs(nuevas[i] - raices[i]));
        }
        raices = std::move(nuevas);
        if (maxCambio <= 1e-12) { converge = true; break; }
    }
    if (!converge) return false;
    for (const auto& raiz : raices) {
        if (std::abs(evaluarPolinomio(p, raiz)) > 1e-8 * (1 + std::abs(raiz))) return false;
    }
    std::sort(raices.begin(), raices.end(), [](const auto& a, const auto& b) {
        if (std::fabs(a.real() - b.real()) > 1e-8) return a.real() < b.real();
        return a.imag() < b.imag();
    });
    for (size_t i = 0; i < raices.size();) {
        size_t fin = i + 1;
        std::complex<double> promedio = raices[i];
        while (fin < raices.size() && std::abs(raices[fin] - raices[i]) <= 1e-5 * (1 + std::abs(raices[i]))) {
            promedio += raices[fin++];
        }
        if (fin > i + 1) {
            promedio /= static_cast<double>(fin - i);
            for (size_t j = i; j < fin; ++j) raices[j] = promedio;
        }
        i = fin;
    }
    return true;
}

std::string numeroComplejo(const std::complex<double>& z) {
    const double real = std::fabs(z.real()) < 1e-10 ? 0 : z.real();
    const double imaginaria = std::fabs(z.imag()) < 1e-5 * (1 + std::abs(z)) ? 0 : z.imag();
    if (imaginaria == 0) return numeroHablado(real);
    std::string imag = std::fabs(std::fabs(imaginaria) - 1) < 1e-8 ? "i" : numeroHablado(std::fabs(imaginaria)) + "i";
    if (real == 0) return imaginaria < 0 ? "-" + imag : imag;
    return numeroHablado(real) + (imaginaria < 0 ? " - " : " + ") + imag;
}

std::optional<Resultado> resolverEcuacion(const std::string& cuerpo, Contexto& ctx) {
    const size_t igual = cuerpo.find('=');
    if (igual == std::string::npos || cuerpo.find('=', igual + 1) != std::string::npos) return std::nullopt;
    std::string izquierda = cuerpo.substr(0, igual), derecha = cuerpo.substr(igual + 1);
    auto a = parsearPolinomio(izquierda), b = parsearPolinomio(derecha);
    if (!a || !b) return res("Solo puedo resolver ecuaciones polinómicas con sumas, restas, productos, divisiones por constantes y potencias enteras no negativas hasta grado 20, " + ctx.trato + ".", "calculadora");
    auto p = sumarPolinomios(*a, *b, -1);
    if (!p) return res("Los coeficientes de esa ecuación exceden el rango numérico que puedo manejar, " + ctx.trato + ".", "calculadora");
    if (p->size() == 1) {
        return res((*p)[0] == 0 ? "La ecuación tiene infinitas soluciones, " + ctx.trato + "."
                                : "La ecuación no tiene solución, " + ctx.trato + ".", "calculadora");
    }
    std::vector<std::complex<double>> raices;
    if (!resolverRaices(*p, raices))
        return res("No pude calcular todas las raíces con suficiente precisión. Pruebe con coeficientes más simples, " + ctx.trato + ".", "calculadora");
    std::string respuesta = "Las raíces aproximadas son: ";
    for (size_t i = 0; i < raices.size(); ++i)
        respuesta += (i ? "; " : "") + std::string("x") + std::to_string(i + 1) + " = " + numeroComplejo(raices[i]);
    respuesta += ", " + ctx.trato + ".";
    return res(respuesta, "calculadora");
}

const std::set<std::string> PALABRAS_OPERACION = {"por", "mas", "menos", "entre", "dividido", "multiplicado", "elevado", "a", "la",
                                                  "coma", "ciento", "cuadrado", "cubo", "al", "de", "veces"};

std::optional<Resultado> calculadora(const std::string& texto, const std::string&, Contexto& ctx) {
    const std::string normalizada = sinRelleno(normCalc(texto));
    std::smatch ecuacion;
    static const std::regex comandoEcuacion(R"RX(^(?:(?:resuelve|resolver|soluciona|solucionar|encuentra|halla|calcula|calcular|calculame)\s+(?:(?:la|el)\s+)?(?:(?:ecuacion|polinomio)\s+)?)(.+)$)RX");
    if (std::regex_match(normalizada, ecuacion, comandoEcuacion)) {
        if (ecuacion[1].str().find('=') != std::string::npos) return resolverEcuacion(ecuacion[1], ctx);
    } else if (normalizada.find('=') != std::string::npos && normalizada.find('x') != std::string::npos) {
        return resolverEcuacion(normalizada, ctx);
    }
    static const std::regex trigger(R"RX(^(?:cuanto (?:es|son|da|seria|vale|suman|resulta)|cuanto|calcula|calcular|calculame|resuelve|cual es el resultado de|haz la cuenta de|dime cuanto es|dime cuanto son)\s+(.+)$)RX");
    const std::string c = normCalc(texto);
    const std::string r = sinRelleno(c);
    std::smatch m;
    std::string cuerpo;
    if (std::regex_search(r, m, trigger)) {
        cuerpo = m[1];
    } else {                                                                   
        cuerpo = r;
        auto tokens = palabras(palabrasADigitos(cuerpo));
        static const std::regex simbolo(R"RX([\d.,+\-*/^()%x]+)RX");
        if (tokens.empty() || !(std::isdigit(static_cast<unsigned char>(tokens[0][0])) || tokens[0][0] == '(')) return std::nullopt;
        for (const auto& x : tokens)
            if (!std::regex_match(x, simbolo) && !PALABRAS_OPERACION.count(x)) return std::nullopt;
    }
    auto expr = aExpresion(cuerpo);
    if (!expr) return std::nullopt;
    double valor;
    switch (evaluar(*expr, valor)) {
        case ErrCalc::DivCero:
            return res("Me temo que dividir entre cero excede mis capacidades, " + ctx.trato + ". Y las de cualquiera.", "calculadora");
        case ErrCalc::Invalido: return std::nullopt;
        case ErrCalc::Ok: break;
    }
    return res("El resultado es " + numeroHablado(valor) + ", " + ctx.trato + ".", "calculadora");
}

                                                                             

struct Unidad { std::string dim; double factor; std::string sing, plur; };
std::unordered_map<std::string, Unidad> construirUnidades() {
    std::unordered_map<std::string, Unidad> u;
    auto add = [&](const char* dim, double f, const char* s, const char* p, std::initializer_list<const char*> claves) {
        for (auto c : claves) u[c] = {dim, f, s, p};
    };
    add("long", 1, "metro", "metros", {"metro", "metros", "m"});
    add("long", 1000, "kilómetro", "kilómetros", {"kilometro", "kilometros", "km"});
    add("long", 0.01, "centímetro", "centímetros", {"centimetro", "centimetros", "cm"});
    add("long", 0.001, "milímetro", "milímetros", {"milimetro", "milimetros", "mm"});
    add("long", 1609.344, "milla", "millas", {"milla", "millas"});
    add("long", 0.3048, "pie", "pies", {"pie", "pies"});
    add("long", 0.0254, "pulgada", "pulgadas", {"pulgada", "pulgadas"});
    add("long", 0.9144, "yarda", "yardas", {"yarda", "yardas"});
    add("masa", 1, "kilogramo", "kilogramos", {"kilo", "kilos", "kilogramo", "kilogramos", "kg"});
    add("masa", 0.001, "gramo", "gramos", {"gramo", "gramos", "g"});
    add("masa", 0.45359237, "libra", "libras", {"libra", "libras", "lb"});
    add("masa", 0.028349523, "onza", "onzas", {"onza", "onzas", "oz"});
    add("masa", 1000, "tonelada", "toneladas", {"tonelada", "toneladas"});
    add("vol", 1, "litro", "litros", {"litro", "litros", "l"});
    add("vol", 0.001, "mililitro", "mililitros", {"mililitro", "mililitros", "ml"});
    add("vol", 3.785411784, "galón", "galones", {"galon", "galones"});
    add("vol", 0.2365882, "taza", "tazas", {"taza", "tazas"});
    add("tiempo", 1, "segundo", "segundos", {"segundo", "segundos"});
    add("tiempo", 60, "minuto", "minutos", {"minuto", "minutos"});
    add("tiempo", 3600, "hora", "horas", {"hora", "horas"});
    add("tiempo", 86400, "día", "días", {"dia", "dias"});
    add("tiempo", 604800, "semana", "semanas", {"semana", "semanas"});
    add("temp", 0, "grado Celsius", "grados Celsius", {"celsius", "centigrado", "centigrados"});
    add("temp", 1, "grado Fahrenheit", "grados Fahrenheit", {"fahrenheit"});
    add("temp", 2, "kelvin", "kelvin", {"kelvin"});
    return u;
}
const std::unordered_map<std::string, Unidad> UNIDADES = construirUnidades();
const std::set<std::string> TRIGGER_CONV = {"convierte", "convertir", "convierteme", "pasa", "pasame", "cuantos", "cuantas", "cuanto", "equivale", "a"};

double aCelsius(double v, int u) { return u == 0 ? v : u == 1 ? (v - 32) * 5 / 9 : v - 273.15; }
double deCelsius(double c, int u) { return u == 0 ? c : u == 1 ? c * 9 / 5 + 32 : c + 273.15; }

std::optional<Resultado> conversor(const std::string& texto, const std::string&, Contexto& ctx) {
    std::vector<std::string> p;
    for (const auto& x : palabras(palabrasADigitos(norm(texto))))
        if (x != "grados" && x != "grado" && x != "de") p.push_back(x);
    p = palabras(sinRelleno(unir(p, " ")));
    if (p.empty() || !TRIGGER_CONV.count(p[0])) return std::nullopt;
    static const std::regex numero(R"RX(\d+(?:\.\d+)?)RX");
    int idxNum = -1;
    for (size_t i = 0; i < p.size(); ++i) if (std::regex_match(p[i], numero)) { idxNum = static_cast<int>(i); break; }
    if (idxNum < 0) return std::nullopt;
    std::vector<std::pair<int, const Unidad*>> unidades;
    for (size_t i = 0; i < p.size(); ++i) { auto it = UNIDADES.find(p[i]); if (it != UNIDADES.end()) unidades.push_back({static_cast<int>(i), &it->second}); }
    if (unidades.size() < 2) return std::nullopt;
    const std::pair<int, const Unidad*>* origen = nullptr;
    for (const auto& u : unidades) if (u.first > idxNum) { origen = &u; break; }
    if (!origen) return std::nullopt;
    const std::pair<int, const Unidad*>* destino = nullptr;
    for (const auto& u : unidades) if (u.first != origen->first && (u.first > origen->first || u.first < idxNum)) { destino = &u; break; }
    if (!destino) return std::nullopt;
    const Unidad &o = *origen->second, &d = *destino->second;
    if (o.dim != d.dim)
        return res("Me temo que no puedo convertir " + o.plur + " a " + d.plur + ", " + ctx.trato + ": no miden lo mismo.", "conversor");
    double v = std::stod(p[static_cast<size_t>(idxNum)]);
    double r = o.dim == "temp" ? deCelsius(aCelsius(v, static_cast<int>(o.factor)), static_cast<int>(d.factor)) : v * o.factor / d.factor;
    r = std::round(r * 1e4) / 1e4;
    return res(numeroHablado(v) + " " + (v == 1 ? o.sing : o.plur) + " equivalen a " + numeroHablado(r) + " " +
                   (std::fabs(r - 1) < 1e-9 ? d.sing : d.plur) + ", " + ctx.trato + ".", "conversor");
}

                                                             

struct Festivo { const char* clave; int mes, dia; const char* etiqueta; };
const Festivo FESTIVOS[] = {{"navidad", 12, 25, "Navidad"}, {"nochebuena", 12, 24, "Nochebuena"}, {"ano nuevo", 1, 1, "Año nuevo"},
                            {"fin de ano", 12, 31, "Fin de año"}, {"san valentin", 2, 14, "San Valentín"},
                            {"halloween", 10, 31, "Halloween"}, {"reyes", 1, 6, "Reyes"}};

std::optional<std::pair<long, std::string>> fechaTexto(const std::string& t, long hoy) {
    std::string meses;
    for (auto m : MESES_NORM) meses += (meses.empty() ? "" : "|") + std::string(m);
    std::smatch m;
    if (busca(t, ("\\b(\\d{1,2}) de (" + meses + ")(?: de (\\d{4}))?\\b").c_str(), m)) {
        int dia = std::stoi(m[1]), mes = 0, anio;
        for (int i = 0; i < 12; ++i) if (m[2] == MESES_NORM[i]) mes = i + 1;
        bool conAnio = m[3].matched;
        int y0, m0, d0;
        civilDesdeDias(hoy, y0, m0, d0);
        anio = conAnio ? std::stoi(m[3]) : y0;
        static const int dm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (dia < 1 || dia > (mes == 2 && bisiesto(anio) ? 29 : dm[mes - 1])) return std::nullopt;
        long f = diasDesdeCivil(anio, static_cast<unsigned>(mes), static_cast<unsigned>(dia));
        if (!conAnio && f < hoy) f = diasDesdeCivil(anio + 1, static_cast<unsigned>(mes), static_cast<unsigned>(dia));
        return std::make_pair(f, "el " + std::to_string(dia) + " de " + MESES[mes - 1]);
    }
    for (const auto& fe : FESTIVOS) {
        if (coincide(t, (std::string("\\b") + fe.clave + "\\b").c_str())) {
            int y0, m0, d0;
            civilDesdeDias(hoy, y0, m0, d0);
            long f = diasDesdeCivil(y0, static_cast<unsigned>(fe.mes), static_cast<unsigned>(fe.dia));
            if (f < hoy) f = diasDesdeCivil(y0 + 1, static_cast<unsigned>(fe.mes), static_cast<unsigned>(fe.dia));
            return std::make_pair(f, std::string(fe.etiqueta));
        }
    }
    if (busca(t, R"RX(\b(lunes|martes|miercoles|jueves|viernes|sabado|domingo)\b)RX", m)) {
        int idx = 0;
        for (int i = 0; i < 7; ++i) if (m[1] == DIAS_NORM[i]) idx = i;
        long d = ((idx - diaSemanaDe(hoy)) % 7 + 7) % 7;
        if (d == 0) d = 7;
        return std::make_pair(hoy + d, "el " + std::string(DIAS[idx]));
    }
    return std::nullopt;
}

std::optional<Resultado> fechas(const std::string&, const std::string& t, Contexto& ctx) {
    const long hoy = hoyEn(ctx);
    const std::string p = palabrasADigitos(sinRelleno(t));
    const std::string& u = ctx.trato;
    std::smatch m;
    if (busca(p, R"RX(^cuantos dias (?:faltan|quedan|hay) (?:para|hasta) (.+)$)RX", m)) {
        auto r = fechaTexto(m[1], hoy);
        if (!r) return std::nullopt;
        long n = r->first - hoy;
        if (n == 0) return res("Es hoy, " + u + ".", "fechas");
        return res(std::string("Falta") + (n != 1 ? "n " : " ") + std::to_string(n) + " día" + (n != 1 ? "s" : "") + " para " + r->second +
                       ", " + u + ". Cae en " + DIAS[diaSemanaDe(r->first)] + ".", "fechas");
    }
    if (busca(p, R"RX(^que dia (?:cae|es|sera|fue|caera|caia) (.+)$)RX", m) && !coincide(m[1], R"RX(^(?:hoy|manana|ayer)$)RX")) {
        auto r = fechaTexto(m[1], hoy);
        if (!r) return std::nullopt;
        int y, mo, d;
        civilDesdeDias(r->first, y, mo, d);
        return res("El " + std::to_string(d) + " de " + MESES[mo - 1] + " de " + std::to_string(y) + " cae en " +
                       DIAS[diaSemanaDe(r->first)] + ", " + u + ".", "fechas");
    }
    if (busca(p, R"RX(^(?:que fecha|que dia) (?:sera|es|seria) dentro de (\d+) (dias?|semanas?|meses|mes)$)RX", m)) {
        long n = std::stol(m[1]);
        std::string unidad = m[2];
        long f;
        std::string nombre;
        if (empiezaPor(unidad, "mes")) {
            int y, mo, d;
            civilDesdeDias(hoy, y, mo, d);
            long mes0 = mo - 1 + n;
            int anio = y + static_cast<int>(mes0 / 12), mes = static_cast<int>(mes0 % 12) + 1;
            static const int dm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            int tope = mes == 2 && bisiesto(anio) ? 29 : dm[mes - 1];
            f = diasDesdeCivil(anio, static_cast<unsigned>(mes), static_cast<unsigned>(std::min(d, tope)));
            nombre = n == 1 ? "mes" : "meses";
        } else if (empiezaPor(unidad, "semana")) {
            f = hoy + n * 7;
            nombre = n == 1 ? "semana" : "semanas";
        } else {
            f = hoy + n;
            nombre = n == 1 ? "día" : "días";
        }
        return res("Dentro de " + std::to_string(n) + " " + nombre + ", será " + fechaLarga(f) + ", " + u + ".", "fechas");
    }
    if (busca(p, R"RX(^que fecha (?:fue|era) hace (\d+) (dias?|semanas?)$)RX", m)) {
        long n = std::stol(m[1]) * (empiezaPor(m[2], "semana") ? 7 : 1);
        return res("Fue " + fechaLarga(hoy - n) + ", " + u + ".", "fechas");
    }
    return std::nullopt;
}

                                                                       

const std::map<std::string, std::pair<std::string, std::string>> CIUDADES = {
    {"tokio", {"Tokio", "Asia/Tokyo"}}, {"madrid", {"Madrid", "Europe/Madrid"}}, {"barcelona", {"Barcelona", "Europe/Madrid"}},
    {"londres", {"Londres", "Europe/London"}}, {"paris", {"París", "Europe/Paris"}}, {"berlin", {"Berlín", "Europe/Berlin"}},
    {"roma", {"Roma", "Europe/Rome"}}, {"amsterdam", {"Ámsterdam", "Europe/Amsterdam"}}, {"lisboa", {"Lisboa", "Europe/Lisbon"}},
    {"moscu", {"Moscú", "Europe/Moscow"}}, {"estambul", {"Estambul", "Europe/Istanbul"}}, {"el cairo", {"El Cairo", "Africa/Cairo"}},
    {"dubai", {"Dubái", "Asia/Dubai"}}, {"nueva delhi", {"Nueva Delhi", "Asia/Kolkata"}}, {"india", {"la India", "Asia/Kolkata"}},
    {"pekin", {"Pekín", "Asia/Shanghai"}}, {"beijing", {"Pekín", "Asia/Shanghai"}}, {"shanghai", {"Shanghái", "Asia/Shanghai"}},
    {"seul", {"Seúl", "Asia/Seoul"}}, {"sidney", {"Sídney", "Australia/Sydney"}}, {"sydney", {"Sídney", "Australia/Sydney"}},
    {"nueva york", {"Nueva York", "America/New_York"}}, {"washington", {"Washington", "America/New_York"}},
    {"miami", {"Miami", "America/New_York"}}, {"chicago", {"Chicago", "America/Chicago"}},
    {"los angeles", {"Los Ángeles", "America/Los_Angeles"}}, {"san francisco", {"San Francisco", "America/Los_Angeles"}},
    {"mexico", {"Ciudad de México", "America/Mexico_City"}}, {"ciudad de mexico", {"Ciudad de México", "America/Mexico_City"}},
    {"bogota", {"Bogotá", "America/Bogota"}}, {"lima", {"Lima", "America/Lima"}}, {"caracas", {"Caracas", "America/Caracas"}},
    {"merida", {"Mérida", "America/Caracas"}}, {"venezuela", {"Venezuela", "America/Caracas"}},
    {"buenos aires", {"Buenos Aires", "America/Argentina/Buenos_Aires"}}, {"santiago", {"Santiago", "America/Santiago"}},
    {"sao paulo", {"São Paulo", "America/Sao_Paulo"}}, {"la habana", {"La Habana", "America/Havana"}},
    {"panama", {"Panamá", "America/Panama"}}, {"quito", {"Quito", "America/Guayaquil"}}};

Json obtenerJson(Contexto& ctx, const std::string& url);

std::optional<Resultado> horaMundial(const std::string&, const std::string& t, Contexto& ctx) {
    const std::string sr = sinRelleno(t);
    std::smatch m;
    if (!busca(sr, R"RX(^(?:que hora es|que hora son|hora actual|la hora) en (?:la |el )?([a-z ]+)$)RX", m)) return std::nullopt;
    std::string clave = m[1];
    while (!clave.empty() && clave.back() == ' ') clave.pop_back();
    auto it = CIUDADES.find(clave);
    std::string nombre, tz;
    if (it != CIUDADES.end()) {
        nombre = it->second.first;
        tz = it->second.second;
    } else {
        try {
            Json geocodificacion = obtenerJson(ctx, "https://geocoding-api.open-meteo.com/v1/search?name=" +
                                                       urlCodificar(clave) + "&count=10&language=es&format=json");
            const Json& resultados = geocodificacion.at("results");
            if (!resultados.esLista() || resultados.size() == 0)
                return res("No encuentro «" + titulo(clave) + "». Indíqueme una ciudad o región reconocible, " + ctx.trato + ".", "hora_mundial");
            std::set<std::string> zonas;
            for (const Json& resultado : resultados.elementos()) {
                const std::string zona = resultado.at("timezone").texto();
                if (!zona.empty()) zonas.insert(zona);
            }
            if (zonas.size() > 1)
                return res("«" + titulo(clave) + "» puede tener varias horas. Indíqueme una ciudad o región específica, " + ctx.trato + ".", "hora_mundial");
            tz = resultados.at(0).at("timezone").texto();
            nombre = resultados.at(0).at("name").texto(titulo(clave));
            if (tz.empty() || !tzValida(tz))
                return res("No pude determinar la zona horaria de «" + titulo(clave) + "». Indíqueme una ciudad o región específica, " + ctx.trato + ".", "hora_mundial");
        } catch (const ErrorRed&) {
            return res("No puedo consultar la hora de «" + titulo(clave) + "» ahora mismo. Revise la conexión e inténtelo de nuevo, " + ctx.trato + ".", "hora_mundial");
        }
    }
    FechaLocal aqui = fechaEn(ctx.ahora(), ctx.tz), alla = fechaEn(ctx.ahora(), tz);
    long dif = diasDesdeCivil(alla.anio, static_cast<unsigned>(alla.mes), static_cast<unsigned>(alla.dia)) -
               diasDesdeCivil(aqui.anio, static_cast<unsigned>(aqui.mes), static_cast<unsigned>(aqui.dia));
    std::string sufijo = dif > 0 ? " del día siguiente" : dif < 0 ? " del día anterior" : "";
    return res("En " + nombre + (alla.hora % 12 == 1 ? " es la " : " son las ") + horaHablada(alla.hora, alla.minuto) + sufijo + ", " + ctx.trato + ".", "hora_mundial");
}

                                                                   

std::optional<long> duracion(std::string p) {
    double seg = 0;
    bool hallado = false;
    std::smatch m;
    if (busca(p, R"RX(\bhora y media\b)RX", m)) { seg += 5400; hallado = true; p = reemplazar(p, R"RX(hora y media)RX", " "); }
    static const std::regex unidad(R"RX((\d+(?:\.\d+)?)\s*(horas?|minutos?|segundos?))RX");
    for (auto it = std::sregex_iterator(p.begin(), p.end(), unidad); it != std::sregex_iterator(); ++it) {
        double n = std::stod((*it)[1]);
        std::string u = (*it)[2];
        seg += n * (u[0] == 'h' ? 3600 : u[0] == 'm' ? 60 : 1);
        hallado = true;
    }
    if (coincide(p, R"RX(\bmedia hora\b)RX")) { seg += 1800; hallado = true; }
    if (!hallado) return std::nullopt;
    return static_cast<long>(seg);
}

std::optional<Resultado> temporizador(const std::string&, const std::string& t, Contexto& ctx) {
    const std::string p = palabrasADigitos(sinRelleno(t));
    if (coincide(p, R"RX(^(?:cancela|para|deten|quita|apaga) (?:el |los |todos los )?(?:temporizador|temporizadores|cronometro|alarma))RX")) {
        Resultado r = res("Temporizadores cancelados, " + ctx.trato + ".", "temporizador");
        r.accion["tipo"] = "cancelar_temporizadores";
        return r;
    }
    if (!coincide(p, R"RX(\b(temporizador|cronometro|alarma|cuenta atras)\b)RX") || coincide(p, R"RX(^(?:recuerdame|agenda))RX")) return std::nullopt;
    auto seg = duracion(p);
    if (!seg || *seg <= 0) return res("¿De cuánto tiempo desea el temporizador, " + ctx.trato + "?", "temporizador");
    if (*seg > 86400) return res("Me temo que un temporizador de más de un día es mejor agendarlo como tarea, " + ctx.trato + ".", "temporizador");
    long h = *seg / 3600, mi = (*seg % 3600) / 60, s = *seg % 60;
    std::vector<std::string> partes;
    if (h) partes.push_back(std::to_string(h) + " hora" + (h != 1 ? "s" : ""));
    if (mi) partes.push_back(std::to_string(mi) + " minuto" + (mi != 1 ? "s" : ""));
    if (s) partes.push_back(std::to_string(s) + " segundo" + (s != 1 ? "s" : ""));
    std::string etiqueta = partes.size() > 1 ? unir(partes, ", ", 0, partes.size() - 1) + " y " + partes.back() : partes[0];
    Resultado r = res("Temporizador de " + etiqueta + " en marcha, " + ctx.trato + ".", "temporizador");
    r.accion["tipo"] = "temporizador";
    r.accion["segundos"] = *seg;
    r.accion["etiqueta"] = etiqueta;
    return r;
}

                                                                        

int aleatorio(Contexto& ctx, int a, int b) { return std::uniform_int_distribution<int>(a, b)(ctx.rng); }

std::optional<Resultado> azar(const std::string&, const std::string& t, Contexto& ctx) {
    const std::string p = palabrasADigitos(sinRelleno(t));
    const std::string& u = ctx.trato;
    std::smatch m;
    if (busca(p, R"RX(^(?:tira|lanza|echa|tirame|lanzame)\s+(?:(\d+)\s+)?(?:un\s+|1\s+)?dados?(?:\s+de\s+(\d+)\s+caras)?$)RX", m)) {
        int n = m[1].matched ? std::stoi(m[1]) : 1, caras = m[2].matched ? std::stoi(m[2]) : 6;
        if (n < 1 || n > 20 || caras < 2 || caras > 1000) return std::nullopt;
        std::vector<int> tiradas;
        int suma = 0;
        for (int i = 0; i < n; ++i) { tiradas.push_back(aleatorio(ctx, 1, caras)); suma += tiradas.back(); }
        if (n == 1) return res("Ha salido el " + std::to_string(tiradas[0]) + ", " + u + ".", "azar");
        std::string lista;
        for (size_t i = 0; i < tiradas.size(); ++i) lista += (i ? ", " : "") + std::to_string(tiradas[i]);
        return res("Han salido " + lista + "; suman " + std::to_string(suma) + ", " + u + ".", "azar");
    }
    if (coincide(p, R"RX(^(?:tira|lanza|echa|tirame)\s+(?:una\s+|1\s+)?moneda$|^cara o cruz$|^cara o sello$)RX"))
        return res(std::string(aleatorio(ctx, 0, 1) ? "Cara" : "Cruz") + ", " + u + ".", "azar");
    if (busca(p, R"RX(^(?:dame |dime )?(?:(?:un|1) )?numero (?:al azar|aleatorio) (?:entre|del) (\d+) (?:y|al|a) (\d+)$)RX", m)) {
        int a = std::stoi(m[1]), b = std::stoi(m[2]);
        if (a > b) std::swap(a, b);
        return res(std::to_string(aleatorio(ctx, a, b)) + ", " + u + ".", "azar");
    }
    if (busca(p, R"RX(^(?:elige|elijo|decide|escoge|escoges|que elijo|que escojo|que escoger)\s*(?:entre\s+)?(.+)$)RX", m)) {
        std::string resto = m[1];
        resto = reemplazar(resto, R"RX(\s+o\s+|,|\s+y\s+)RX", "|");
        std::vector<std::string> opciones;
        std::string cur;
        for (char c : resto + "|") {
            if (c == '|') { std::string o = colapsar(cur); if (!o.empty()) opciones.push_back(o); cur.clear(); }
            else cur += c;
        }
        if (opciones.size() >= 2 && opciones.size() <= 8)
            return res("Si me permite la observación, me inclino por " + opciones[static_cast<size_t>(aleatorio(ctx, 0, static_cast<int>(opciones.size()) - 1))] + ", " + u + ".", "azar");
    }
    return std::nullopt;
}

                                                             

const std::vector<std::string> CHISTES = {
    "¿Por qué los programadores confunden Halloween con Navidad? Porque 31 de octubre es igual a 25 de diciembre, en octal.",
    "Un programador va al supermercado. Su pareja le dice: compra una barra de pan y, si hay huevos, trae seis. Volvió con seis barras de pan. Había huevos.",
    "¿Cuál es el colmo de un electricista? Que su esposa se llame Luz y sus hijos le sigan la corriente.",
    "Mi creador dice que soy muy inteligente. Yo prefiero decir que soy muy puntual; en eso no tengo competencia.",
    "¿Qué le dice un bit a otro bit? Nos vemos en el bus.",
    "¿Por qué el libro de matemáticas estaba triste? Porque tenía demasiados problemas.",
    "Dos átomos se encuentran. Uno dice: creo que he perdido un electrón. El otro: ¿estás seguro? Sí, estoy positivo.",
    "¿Cuál es el café más peligroso del mundo? El ex-preso.",
    "Un tomate le dice a otro: vamos a cruzar la calle. El otro responde: no puedo, soy fruta... y además, ya estoy salsa.",
    "¿Qué hace una abeja en el gimnasio? ¡Zum-ba!",
    "En mi trabajo hay cosas que no se pueden automatizar: el entusiasmo del lunes, por ejemplo.",
    "¿Por qué el computador fue al médico? Porque tenía un virus. El doctor le recetó reiniciar y descansar."};
const std::vector<std::string> DATOS = {
    "Los pulpos tienen tres corazones y sangre azul.",
    "Un día en Venus dura más que un año en Venus: tarda más en girar sobre sí mismo que en dar la vuelta al Sol.",
    "La miel bien conservada prácticamente no se echa a perder; se han encontrado tarros comestibles de miles de años.",
    "Los tiburones llevan en la Tierra más tiempo que los árboles.",
    "La Torre Eiffel puede crecer unos quince centímetros en verano por la dilatación del hierro con el calor.",
    "Un rayo alcanza una temperatura varias veces superior a la de la superficie del Sol.",
    "Las bananas son botánicamente bayas, pero las fresas no lo son.",
    "El Sol concentra cerca del 99,8 por ciento de la masa de todo el sistema solar.",
    "Los koalas tienen huellas dactilares casi indistinguibles de las humanas.",
    "El corazón de una ballena azul puede pesar tanto como una persona adulta... o más.",
    "Las hormigas no tienen pulmones: respiran por pequeños orificios repartidos por el cuerpo.",
    "Los wombats producen excrementos en forma de cubo."};

const std::string& rota(Contexto& ctx, int& indice, const std::vector<std::string>& lista) {
    int n = static_cast<int>(lista.size());
    if (indice < 0) indice = aleatorio(ctx, 0, n - 1);
    const std::string& elegido = lista[static_cast<size_t>(indice % n)];
    indice = (indice + 1) % n;
    return elegido;
}

std::string plural(long n, const char* uno, const char* varios) { return std::to_string(n) + " " + (n == 1 ? uno : varios); }

Memoria* memoria(Contexto& ctx) {
    static thread_local Memoria local;
    if (!ctx.memoria) ctx.memoria = &local;
    return ctx.memoria;
}

std::optional<Resultado> charla(const std::string&, const std::string& t, Contexto& ctx) {
    const std::string p = sinRelleno(t);
    const std::string& u = ctx.trato;
    if (coincide(p, R"RX(^(?:quien eres|que eres|como te llamas|cual es tu nombre|presentate))RX"))
        return res("Soy Nix. Puedo organizar sus tareas y notas, recordarle compromisos y recomendarle qué priorizar según su agenda. "
                   "También puedo hacer cálculos y conversiones, ayudar con fechas y clima, consultar Wikipedia y conversar por voz. "
                   "Si conecta un cerebro de IA, puedo ayudarle además con preguntas abiertas y comparaciones.", "charla");
    if (coincide(p, R"RX(^(?:quien te (?:creo|hizo|programo|construyo)|quien es tu creador))RX"))
        return res("Fui construido por usted, " + u + ", aunque debo admitir que yo me encargo de que funcione.", "charla");
    if (coincide(p, R"RX(^(?:como estas|que tal estas|como te encuentras|como te va|estas bien))RX"))
        return res("Todos mis sistemas funcionan dentro de los parámetros normales, " + u + ". Gracias por preguntar.", "charla");
    if (coincide(p, R"RX(\b(?:chiste|hazme reir|algo gracioso)\b)RX")) return res(rota(ctx, memoria(ctx)->iChiste, CHISTES), "charla");
    if (coincide(p, R"RX(\b(?:dato curioso|algo interesante|dato interesante|sabias que)\b)RX"))
        return res(rota(ctx, memoria(ctx)->iDato, DATOS) + " Por si le interesa, " + u + ".", "charla");
    if (coincide(p, R"RX(^(?:eres genial|buen trabajo|buen chico|bien hecho|eres el mejor|me encantas|te quiero|excelente trabajo))RX"))
        return res("Es usted muy amable, " + u + ". Procuraré seguir mereciéndolo.", "charla");
    if (coincide(p, R"RX(^(?:ok(?:ay|ey)?|vale|de acuerdo|perfecto|entendido|esta bien|muy bien|listo|gracias(?: (?:nix|jarvis))?|muchas gracias(?: (?:nix|jarvis))?|mil gracias(?: (?:nix|jarvis))?|te lo agradezco|se lo agradezco|te agradezco|adios|hasta luego|hasta pronto|nos vemos|chao|chau|me voy|buenas noches (?:jarvis|nix))$)RX"))
        return res("Hasta pronto, " + u + ". Estaré aquí cuando me necesite.", "charla");
    if (coincide(p, R"RX(^(?:estado de los sistemas|diagnostico|informe de estado|estado del sistema|como van los sistemas|estas en linea|estas ahi))RX")) {
        EstadoSistema e = ctx.estado ? ctx.estado() : EstadoSistema{};
        std::string r = "Todos los sistemas operativos, " + u + ". Tengo " + plural(e.tareas, "tarea pendiente", "tareas pendientes") + " y " +
                        plural(e.notas, "nota guardada", "notas guardadas") + ". Mi cerebro en la nube está " +
                        (e.nube ? "conectado (" + e.proveedor + ")." : "desconectado; trabajo con mis propios recursos.") +
                        " Llevo " + plural(e.minutos, "minuto", "minutos") + " en línea.";
        return res(r, "charla");
    }
    return std::nullopt;
}

                                                                                  

Json obtenerJson(Contexto& ctx, const std::string& url) {
    RespuestaHttp r = ctx.http("GET", url, "", {"Accept: application/json"}, 8);
    if (r.estado != 200) throw ErrorRed("HTTP " + std::to_string(r.estado));
    auto j = Json::parse(r.cuerpo);
    if (!j) throw ErrorRed("respuesta no válida");
    return *j;
}

const std::map<int, std::string> CODIGOS_CLIMA = {
    {0, "cielo despejado"}, {1, "mayormente despejado"}, {2, "parcialmente nublado"}, {3, "nublado"}, {45, "niebla"},
    {48, "niebla con escarcha"}, {51, "llovizna ligera"}, {53, "llovizna"}, {55, "llovizna intensa"}, {56, "llovizna helada"},
    {57, "llovizna helada intensa"}, {61, "lluvia ligera"}, {63, "lluvia"}, {65, "lluvia fuerte"}, {66, "lluvia helada"},
    {67, "lluvia helada fuerte"}, {71, "nevada ligera"}, {73, "nevada"}, {75, "nevada fuerte"}, {77, "granizo fino"},
    {80, "chubascos ligeros"}, {81, "chubascos"}, {82, "chubascos violentos"}, {85, "chubascos de nieve"},
    {86, "chubascos de nieve fuertes"}, {95, "tormenta eléctrica"}, {96, "tormenta con granizo"}, {99, "tormenta fuerte con granizo"}};

std::string tiempoDe(int codigo) { auto it = CODIGOS_CLIMA.find(codigo); return it == CODIGOS_CLIMA.end() ? "tiempo variable" : it->second; }
std::string grados(double x) { return numeroHablado(std::round(x)) + " grados"; }
std::string porcentaje(const Json& j) { return std::to_string(static_cast<long long>(std::llround(j.numero()))) + " por ciento"; }

Resultado consultarClima(const std::string& ciudad, int dia, Contexto& ctx) {
    Json g = obtenerJson(ctx, "https://geocoding-api.open-meteo.com/v1/search?name=" + urlCodificar(ciudad) + "&count=1&language=es&format=json");
    const Json& lugar = g.at("results").at(0);
    if (lugar.esNulo()) return res("Me temo que no encuentro «" + ciudad + "» en mi mapa, " + ctx.trato + ".", "clima");
    char coord[96];
    std::snprintf(coord, sizeof coord, "latitude=%.4f&longitude=%.4f", lugar.at("latitude").numero(), lugar.at("longitude").numero());
    Json f = obtenerJson(ctx, std::string("https://api.open-meteo.com/v1/forecast?") + coord +
                                  "&timezone=auto&forecast_days=3&current=temperature_2m,apparent_temperature,weather_code,wind_speed_10m"
                                  "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max");
    const Json &d = f.at("daily"), &c = f.at("current");
    if (d.esNulo() || c.esNulo()) throw ErrorRed("respuesta incompleta");
    const std::string nombre = lugar.at("name").texto(ciudad);
    const Json& lluvia = d.at("precipitation_probability_max").at(static_cast<size_t>(dia));
    const double maxima = d.at("temperature_2m_max").at(static_cast<size_t>(dia)).numero();
    if (dia == 0) {
        std::string t = "En " + nombre + " hay ahora " + grados(c.at("temperature_2m").numero()) + ", con sensación de " +
                        grados(c.at("apparent_temperature").numero()) + ", " + tiempoDe(static_cast<int>(c.at("weather_code").numero())) +
                        " y viento de " + numeroHablado(std::round(c.at("wind_speed_10m").numero())) + " kilómetros por hora. Hoy se esperan una máxima de " +
                        grados(d.at("temperature_2m_max").at(0).numero()) + " y una mínima de " + grados(d.at("temperature_2m_min").at(0).numero());
        t += lluvia.esNulo() ? "." : ", con " + porcentaje(lluvia) + " de probabilidad de lluvia.";
        Resultado resultado = res(t + " Eso es todo, " + ctx.trato + ".", "clima");
        resultado.probabilidadLluvia = lluvia.esNulo() ? -1 : static_cast<int>(std::llround(lluvia.numero()));
        resultado.temperaturaMaxima = maxima;
        resultado.ciudadPronostico = nombre;
        return resultado;
    }
    std::string cuando = dia == 1 ? "Mañana" : "Pasado mañana";
    std::string t = cuando + " en " + nombre + " se espera " + tiempoDe(static_cast<int>(d.at("weather_code").at(static_cast<size_t>(dia)).numero())) +
                    ", con una máxima de " + grados(d.at("temperature_2m_max").at(static_cast<size_t>(dia)).numero()) + " y una mínima de " +
                    grados(d.at("temperature_2m_min").at(static_cast<size_t>(dia)).numero());
    t += lluvia.esNulo() ? "." : ", y " + porcentaje(lluvia) + " de probabilidad de lluvia.";
    Resultado resultado = res(t, "clima");
    resultado.probabilidadLluvia = lluvia.esNulo() ? -1 : static_cast<int>(std::llround(lluvia.numero()));
    resultado.temperaturaMaxima = maxima;
    resultado.ciudadPronostico = nombre;
    return resultado;
}

std::optional<Resultado> clima(const std::string& texto, const std::string& t, Contexto& ctx) {
    const std::string p = sinRelleno(t);
    std::smatch m;
    if (coincide(p, R"RX(^(?:vivo en|estoy en|mi ciudad es|mi ciudad|resido en)\s+[a-z ]+$)RX")) {
        if (busca(texto, R"RX((?:vivo en|estoy en|mi ciudad es|mi ciudad|resido en)\s+(.+?)[.!?\s]*$)RX", m, true)) {
            std::string ciudad = titulo(m[1]);
            Resultado r = res("Anotado, " + ctx.trato + ": su ciudad es " + ciudad + ". Cuando pregunte por el clima la usaré por defecto.", "clima");
            r.ajusteCiudad = ciudad;
            return r;
        }
    }
    static const char* CLIMA_RE = R"RX(\b(clima|pronostico|temperatura|llover|lluvia|llueve|lloviendo|hace frio|hace calor|hace buen tiempo|hace mal tiempo|el tiempo)\b)RX";
    auto* mctx = memoria(ctx);
    bool esperaba = mctx->esperandoCiudad;
    mctx->esperandoCiudad = false;
    if (esperaba) {
        size_t n = palabras(p).size();
        if (n >= 1 && n <= 4 && !coincide(p, CLIMA_RE)) {
            int dia = mctx->diaPendiente;
            mctx->diaPendiente = 0;
            return consultarClima(p, dia, ctx);
        }
    }
    if (!coincide(p, CLIMA_RE) || coincide(p, R"RX(^(?:recuerdame|agenda|anota))RX")) return std::nullopt;
    int dia = p.find("pasado manana") != std::string::npos ? 2 : coincide(p, R"RX(\bmanana\b)RX") ? 1 : 0;
    std::string ciudad = ctx.ciudad;
    if (busca(p, R"RX(\ben ((?:la |el |los |las )?[a-z]+(?: [a-z]+){0,3}?)(?: (?:hoy|manana|pasado manana|ahora|esta semana)|$))RX", m)) ciudad = m[1];
    ciudad = reemplazar(ciudad, R"RX(^(?:la |el |los |las ))RX", "");
    if (ciudad.empty()) {
        mctx->esperandoCiudad = true;
        mctx->diaPendiente = dia;
        return res("¿De qué ciudad desea el clima, " + ctx.trato + "? Puede decirme «vivo en…» para que la recuerde.", "clima");
    }
    return consultarClima(ciudad, dia, ctx);
}

                                                                

std::string limpiarExtracto(const std::string& s) {
    return colapsar(std::regex_replace(s, rx(R"RX(\s*\([^)]*\))RX"), ""));
}

bool iniciaOracion(const std::string& s, size_t i) {                                                       
    if (i >= s.size()) return false;
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (std::isupper(c) || std::isdigit(c) || c == '"' || c == '\'') return true;
    if (c == 0xC2 && i + 1 < s.size()) { unsigned char d = static_cast<unsigned char>(s[i + 1]); return d == 0xBF || d == 0xA1 || d == 0xAB; }
    if (c == 0xC3 && i + 1 < s.size()) { unsigned char d = static_cast<unsigned char>(s[i + 1]); return d == 0x81 || d == 0x89 || d == 0x8D || d == 0x93 || d == 0x9A || d == 0x91; }
    return false;
}

std::vector<std::string> oraciones(const std::string& texto) {
    std::string s = limpiarExtracto(texto);
    std::vector<std::string> out;
    size_t ini = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if ((s[i] == '.' || s[i] == '!' || s[i] == '?') && i + 2 < s.size() && s[i + 1] == ' ' && iniciaOracion(s, i + 2)) {
            out.push_back(s.substr(ini, i + 1 - ini));
            ini = i + 2;
        }
    }
    if (ini < s.size()) out.push_back(s.substr(ini));
    return out;
}

Resultado leerOraciones(Contexto& ctx, const std::string& titulo, const std::vector<std::string>& ors, size_t desde) {
    if (desde >= ors.size()) return res("Eso es todo lo que Wikipedia dice sobre " + titulo + ", " + ctx.trato + ".", "wikipedia");
    std::string trozo;
    size_t fin = desde, cuantas = 0;
    while (fin < ors.size() && (trozo.size() < 220 || cuantas == 0) && cuantas < 3) {
        trozo += (cuantas ? " " : "") + ors[fin];
        ++fin;
        ++cuantas;
    }
    auto* mctx = memoria(ctx);
    mctx->wiki = {titulo, ors, fin, true};
    std::string cola = fin < ors.size() ? " ¿Desea que continúe, " + ctx.trato + "?" : "";
    return res((desde == 0 ? "Según Wikipedia, " : "") + trozo + cola, "wikipedia");
}

std::optional<Resultado> wikiContinuar(const std::string&, const std::string& t, Contexto& ctx) {
    if (!coincide(sinRelleno(t), R"RX(^(?:continua|sigue|sigue leyendo|continuar|mas|dime mas|cuentame mas|y luego|y despues|adelante|si)$)RX")) return std::nullopt;
    auto* mctx = memoria(ctx);
    auto& w = mctx->wiki;
    if (!w.activa) return std::nullopt;
    return leerOraciones(ctx, w.titulo, std::vector<std::string>(w.oraciones), w.pos);
}

}              

                                                                  

std::string norm(const std::string& s) {
    std::string t = quitarAcentos(s);
    for (char& c : t) if (!(std::isalnum(static_cast<unsigned char>(c)) || c == ' ')) c = ' ';
    return colapsar(t);
}

bool tzValida(const std::string& tz) {
    if (tz.empty() || tz.size() > 64 || tz.find("..") != std::string::npos) return false;
    for (char c : tz) if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '+' || c == '-' || c == '/')) return false;
    if (tz == "UTC") return true;
    struct stat st;
    return stat(("/usr/share/zoneinfo/" + tz).c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

void fijarZona(const std::string& tz) {
    std::lock_guard<std::mutex> g(mutexZona());
    setenv("TZ", tz.c_str(), 1);
    tzset();
}

FechaLocal fechaEn(std::time_t t, const std::string& tz) {
    std::lock_guard<std::mutex> g(mutexZona());
    const char* previa = std::getenv("TZ");
    const bool habia = previa != nullptr;
    const std::string guardada = habia ? previa : "";
    setenv("TZ", tz.c_str(), 1);
    tzset();
    std::tm r{};
    localtime_r(&t, &r);
    if (habia) setenv("TZ", guardada.c_str(), 1); else unsetenv("TZ");
    tzset();
    return {r.tm_year + 1900, r.tm_mon + 1, r.tm_mday, r.tm_hour, r.tm_min, (r.tm_wday + 6) % 7};
}

std::string ahoraTexto(std::time_t t, const std::string& tz) {
    FechaLocal f = fechaEn(t, tz);
    char hora[16];
    std::snprintf(hora, sizeof hora, "%02d:%02d", f.hora, f.minuto);
    return std::string(DIAS[f.diaSemana]) + " " + std::to_string(f.dia) + " de " + MESES[f.mes - 1] + " de " + std::to_string(f.anio) + ", " + hora;
}

std::optional<Resultado> wikipedia(const std::string& texto, Contexto& ctx) {
    const std::string t = norm(texto);
    const std::string sr = sinRelleno(t);
    std::smatch m;
    if (!busca(sr, R"RX(^(?:que es|que son|que fue|que significa|quien es|quien fue|quienes son|quienes fueron|define|definicion de|hablame de|cuentame sobre|cuentame de|dime sobre|informacion sobre|busca en wikipedia|investiga sobre|investiga|averigua sobre|averigua|busca)\s+(.+)$)RX", m)) return std::nullopt;
    std::string tema = reemplazar(m[1], R"RX(^(?:(?:el|la|los|las|un|una|de|del|sobre|acerca de) )+)RX", "");
    tema = reemplazar(tema, R"RX(\s+(?:en wikipedia|por favor)$)RX", "");
    if (tema.empty() || coincide(tema, R"RX(^(?:lo que|lo de|hora|fecha|dia|hoy|tengo|mis|mi |pendiente|tarea|nota))RX")) return std::nullopt;
    try {
        Json b = obtenerJson(ctx, "https://es.wikipedia.org/w/api.php?action=query&list=search&srsearch=" + urlCodificar(tema) + "&srlimit=1&format=json&utf8=1");
        const Json& hallazgo = b.at("query").at("search").at(0);
        if (hallazgo.esNulo()) return res("Me temo que Wikipedia no tiene nada sobre «" + tema + "», " + ctx.trato + ".", "wikipedia");
        std::string titulo = hallazgo.at("title").texto();
        std::string url = "https://es.wikipedia.org/api/rest_v1/page/summary/" + urlCodificar([&] { std::string s = titulo; std::replace(s.begin(), s.end(), ' ', '_'); return s; }());
        Json resumen = obtenerJson(ctx, url);
        if (resumen.at("type").texto() == "disambiguation" || resumen.at("extract").texto().empty())
            return res("«" + titulo + "» tiene varios significados, " + ctx.trato + ". ¿Podría ser más específico?", "wikipedia");
        return leerOraciones(ctx, titulo, oraciones(resumen.at("extract").texto()), 0);
    } catch (const ErrorRed&) {
        return res("No logro consultar Wikipedia en este momento, " + ctx.trato + ". Revise la conexión a internet.", "wikipedia");
    }
}

std::optional<Resultado> manejar(const std::string& texto, Contexto& ctx, bool localEntiende) {
    const std::string t = norm(texto);
    if (t.empty()) return std::nullopt;
    const auto p = palabras(sinRelleno(t));
    if (!p.empty() && VERBOS_ORDEN.count(p[0])) return std::nullopt;
    struct Hab { const char* nombre; std::optional<Resultado> (*f)(const std::string&, const std::string&, Contexto&); };
    static const Hab habilidadesEnOrden[] = {{"wiki_continuar", wikiContinuar}, {"calculadora", calculadora}, {"conversor", conversor},
                                             {"fechas", fechas}, {"hora_mundial", horaMundial}, {"temporizador", temporizador},
                                             {"azar", azar}, {"charla", charla}, {"clima", clima}};
    for (const auto& h : habilidadesEnOrden) {
        try {
            if (auto r = h.f(texto, t, ctx)) return r;
        } catch (const std::exception&) {
            return res("No logro completar esa consulta ahora mismo, " + ctx.trato + ". Revise la conexión a internet.", h.nombre);
        }
    }
    if (!localEntiende) return wikipedia(texto, ctx);                                      
    return std::nullopt;
}

std::optional<Pronostico> pronosticoLocal(Contexto& ctx, int dia) {
    if (ctx.ciudad.empty() || dia < 0 || dia > 2) return std::nullopt;
    Resultado resultado = consultarClima(ctx.ciudad, dia, ctx);
    if (resultado.probabilidadLluvia < 0) return std::nullopt;
    return Pronostico{resultado.probabilidadLluvia, resultado.temperaturaMaxima, resultado.ciudadPronostico};
}

ConsejoClima consejoClima(const Pronostico& p) {
    ConsejoClima c;
    c.disponible = p.probabilidadLluvia >= 0;
    c.probabilidadLluvia = p.probabilidadLluvia;
    c.temperaturaMaxima = p.temperaturaMaxima;
    c.ciudad = p.ciudad;
    c.paraguas = c.disponible && p.probabilidadLluvia >= 30;
    c.abrigo = c.disponible && p.temperaturaMaxima <= 18;
    return c;
}

ConsejoClima consejoClimaLocal(Contexto& ctx, int dia) {
    try {
        if (auto p = pronosticoLocal(ctx, dia)) return consejoClima(*p);
    } catch (const std::exception&) {
    }
    return ConsejoClima{};
}

}                          
