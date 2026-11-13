NIX: 
Es un asistente personal en español para organizar la vida diaria:
puede gestionar tareas pendientes, guardar notas, administrar un closet de ropa y sugerir combinaciones según la ocasión.
Funciona como una app web/chat con interfaz sencilla y también expone una API para interactuar con sus funciones. 
La lista que usa Nix para tareas, notas y closet está construida sobre la biblioteca **DeSiGNAR** (`Designar::DLList`).

## Estructura
```
nix_fase3/
├── include/            ← todos los .hpp (núcleo y servidor)
├── src/                ← todos los .cpp
│   └── pruebas/        ← pruebas (también .cpp)
├── web/                ← interfaz gráfica (chat, pendientes, notas y closet)
├── CMakeLists.txt      ← compilación (CMake ≥ 3.20, C++17)
├── Dockerfile · docker-compose.yml · .dockerignore · .env.example
├── probar.sh           ← atajo: configura, compila y ejecuta las pruebas
```

## DeSiGNAR
[DeSiGNAR](https://r3mmurd.github.io/DeSiGNAR) es una biblioteca de estructuras de datos y algoritmos en C++17 (repositorio:
https://github.com/r3mmurd/DeSiGNAR). Nix la usa en un único punto, `include/Estructuras.hpp`: la clase `ListaEnlazada<T>` conserva
la interfaz de siempre (`push_back`, `insertarOrdenado`, `erase`, `comoVector`…) y puede guardar sus elementos en una
`Designar::DLList<T>` (`#include <list.hpp>`). El resto del programa conserva su comportamiento. Para compilar sin red,
esta fase incluye un `list.hpp` de respaldo. Para usar explícitamente DeSiGNAR, configure con
`-DNIX_DESIGNAR_DIR=/ruta/a/DeSiGNAR`; CMake prioriza entonces la cabecera de la biblioteca sobre el respaldo local.

## Requisitos
- **g++ ≥ 9** o Clang ≥ 14 (C++17), **CMake ≥ 3.20** y **git** (para descargar DeSiGNAR).
- **Docker** con Compose v2 (solo si prefiere no instalar nada más).

## Compilar y probar con CMake
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure      # ejecuta todas las pruebas
./build/nix                                     # arranca en http://localhost:8080
```
`bash probar.sh` hace los tres primeros pasos. Si tiene DeSiGNAR en su equipo, indíquelo con
`cmake -S . -B build -DNIX_DESIGNAR_DIR=/ruta/a/DeSiGNAR`. Ejecute `./build/nix` desde la carpeta del proyecto (así encuentra
`./web`) o defina `JARVIS_WEB` y `JARVIS_DATA`.

Pruebas incluidas (`src/pruebas/`): `tests_fase1`, `tests_fase2`, `tests`, `tests_closet`, `tests_api`.

## Compilar y ejecutar con Docker
```
cp .env.example .env          # opcional
docker compose up --build     # la primera vez compila, ejecuta todas las pruebas y arranca
```
Abra **http://localhost:8080**. Los datos viven en el volumen `nix_datos` (`/data`).
Solo imagen, sin Compose:
```
docker build -t nix .
docker run --rm -p 127.0.0.1:8080:8080 -v nix_datos:/data nix
```
El `Dockerfile` tiene dos etapas: la primera compila con CMake (descarga DeSiGNAR) y ejecuta `ctest`; **si una prueba falla, la
imagen no se construye**. La segunda contiene solo el programa y la carpeta `web/`, y corre como usuario sin privilegios.
Comandos útiles: `docker compose logs -f nix` · `docker compose down` · `docker compose cp nix:/data ./respaldo`.

## Variables de entorno
| Variable | Para qué sirve | Valor por defecto |
|---|---|---|
| `PORT` | Puerto del servidor | `8080` |
| `JARVIS_WEB` | Carpeta de la interfaz | `./web` |
| `JARVIS_DATA` | Carpeta de datos (`tareas.txt`, `notas.txt`, `closet.txt`) | `./datos` |
| `JARVIS_TRATO` | Cómo le llama Nix | `señor` |

## Uso
Escriba o diga órdenes en el chat; el panel lateral muestra **Pendientes**, **Notas** y **Closet**.
```
agendar reunión con el cliente mañana a las 4        anota que el parcial es el viernes
guarda mi camisa azul, es formal                     (si no dice el estilo, Nix pregunta «¿formal o informal?»)
mi closet · qué tengo de ropa informal · mis camisas
quita mi camisa azul del closet · bórrala · deshaz
vacía mi closet                                      (pide confirmación y se puede deshacer)
qué me pongo para una entrevista                     (arma el outfit con sus prendas y muestra la tarjeta)
```
**API** (JSON; los POST exigen la cabecera `X-Jarvis: 1`): `GET /salud`, `/api/tareas`, `/api/notas`, `/api/closet`;
`POST /api/mensaje`, `/api/tarea/completar`, `/api/tarea/eliminar`, `/api/nota/eliminar`, `/api/closet/agregar`,
`/api/closet/eliminar`, `/api/outfit`.
