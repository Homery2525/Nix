#!/usr/bin/env bash
# Configura, compila y ejecuta todas las pruebas. Uso: bash probar.sh [-DNIX_DESIGNAR_DIR=/ruta/a/DeSiGNAR]
set -euo pipefail
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build -j"$(nproc 2>/dev/null || echo 2)"
ctest --test-dir build --output-on-failure
echo "Listo. Ejecute: ./build/nix   (y abra http://localhost:8080)"
