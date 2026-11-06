#!/usr/bin/env bash
# Compila y ejecuta las pruebas de este hito. Uso: bash probar.sh
set -euo pipefail
cd "$(dirname "$0")"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
g++ -std=c++17 -O1 -Wall -Wextra -pthread -ICore Core/*.cpp Tools/tests_fase1.cpp -o "$T/t1" && "$T/t1"
g++ -std=c++17 -O1 -Wall -Wextra -pthread -ICore Core/*.cpp Tools/tests_fase2.cpp -o "$T/t2" && "$T/t2"
