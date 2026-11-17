#include "Http.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <mutex>
#include <thread>

                                                                        

namespace {

size_t escribir(char* datos, size_t tam, size_t n, void* usuario) {
    static_cast<std::string*>(usuario)->append(datos, tam * n);
    return tam * n;
}

}              

RespuestaHttp httpCurl(const std::string& metodo, const std::string& url, const std::string& cuerpo,
                       const std::vector<std::string>& cabeceras, int timeoutSeg) {
    static std::once_flag unaVez;
    std::call_once(unaVez, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    CURL* h = curl_easy_init();
    if (!h) throw ErrorRed("no se pudo iniciar libcurl");
    std::string respuesta;
    curl_slist* lista = nullptr;
    for (const auto& c : cabeceras) lista = curl_slist_append(lista, c.c_str());

    curl_easy_setopt(h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(h, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeg));
    curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(h, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(h, CURLOPT_USERAGENT, "Jarvis/1.0 (asistente personal local)");
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, &escribir);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, &respuesta);
    if (lista) curl_easy_setopt(h, CURLOPT_HTTPHEADER, lista);
    if (metodo == "POST") {
        curl_easy_setopt(h, CURLOPT_POST, 1L);
        curl_easy_setopt(h, CURLOPT_POSTFIELDS, cuerpo.c_str());
        curl_easy_setopt(h, CURLOPT_POSTFIELDSIZE, static_cast<long>(cuerpo.size()));
    }
    const auto rc = curl_easy_perform(h);
    long estado = 0;
    curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &estado);
    const std::string error = curl_easy_strerror(rc);
    curl_slist_free_all(lista);
    curl_easy_cleanup(h);
    if (rc != CURLE_OK) throw ErrorRed(error);
    return {estado, respuesta};
}

std::string urlCodificar(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += static_cast<char>(c);
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}
