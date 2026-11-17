# Nix — Hito 4 de 6: Servidor completo, habilidades y clima

Asistente personal en español escrito en **C++17**. Este paquete es un hito acumulativo: contiene todo lo de los hitos anteriores
más lo que se describe en [FASE_4.md](FASE_4.md). La lista que usa Nix para tareas, notas y closet está construida sobre la
biblioteca **DeSiGNAR** (`Designar::DLList`).

## Estructura
```
nix_fase4/
├── include/            ← todos los .hpp (núcleo y servidor)
├── src/                ← todos los .cpp
│   └── pruebas/        ← pruebas (también .cpp)
├── web/                ← interfaz gráfica (chat, closet por categoría, temporizadores y clima)
├── CMakeLists.txt      ← compilación (CMake ≥ 3.20, C++17)
├── Dockerfile · docker-compose.yml · .dockerignore · .env.example
├── probar.sh           ← atajo: configura, compila y ejecuta las pruebas
└── FASE_4.md           ← qué incluye este hito
```

## DeSiGNAR
[DeSiGNAR](https://r3mmurd.github.io/DeSiGNAR) es una biblioteca de estructuras de datos y algoritmos en C++17 (repositorio:
https://github.com/r3mmurd/DeSiGNAR). Nix la usa en un único punto, `include/Estructuras.hpp`: la clase `ListaEnlazada<T>` conserva
la interfaz de siempre (`push_back`, `insertarOrdenado`, `erase`, `comoVector`…) pero guarda sus elementos en una
`Designar::DLList<T>` (`#include <list.hpp>`). El resto del programa no cambió, así que su comportamiento es el mismo.
`CMakeLists.txt` descarga DeSiGNAR de GitHub al configurar (necesita internet la primera vez).

## Requisitos
- **g++ ≥ 9** o Clang ≥ 14 (C++17), **CMake ≥ 3.20** y **git** (para descargar DeSiGNAR).
- **libcurl** con cabeceras (`sudo apt install libcurl4-openssl-dev` en Debian/Ubuntu).
- **Docker** con Compose v2 (solo si prefiere no instalar nada más).

## Compilar y probar con CMake
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure      # ejecuta todas las pruebas
./build/nix                                     # arranca en http://localhost:8080
```
`bash probar.sh` hace los tres primeros pasos. Si ya tiene DeSiGNAR en su equipo (o no hay internet), indíquelo:
`cmake -S . -B build -DNIX_DESIGNAR_DIR=/ruta/a/DeSiGNAR`. Ejecute `./build/nix` desde la carpeta del proyecto (así encuentra
`./web`) o defina `JARVIS_WEB` y `JARVIS_DATA`.

Pruebas incluidas (`src/pruebas/`): `tests_fase1`, `tests_fase2`, `tests`, `tests_closet`, `tests_servidor`, `tests_api`.

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
| `JARVIS_CIUDAD` | Ciudad para el clima | vacía |
| `TZ` | Zona horaria | `UTC` |

## Uso
Además de tareas, notas y closet, Nix responde **sin IA** a cálculo, conversiones, fechas, hora mundial, temporizadores,
clima y Wikipedia:
```
calcula 12 más 30 · cuánto es la raíz cuadrada de 144       pon un temporizador de 10 minutos
vivo en Mérida · qué clima hace hoy                         guarda mi camisa azul, es formal
```
El chip de clima de la interfaz usa `GET /api/clima` (caché de 10 minutos; sin ciudad o sin red no se inventa el clima).
Se suma a la API del hito 3. Defina la ciudad con `JARVIS_CIUDAD` o diciendo «vivo en…».
