cmake_minimum_required(VERSION 3.20)
project(Nix VERSION 0.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)   # cliente HTTP del hito (clima, Wikipedia y cerebros)

# ───────── DeSiGNAR (https://github.com/r3mmurd/DeSiGNAR) ─────────
# Nix usa su lista doblemente enlazada (Designar::DLList) a través de include/Estructuras.hpp.
# Por defecto se descarga de GitHub al configurar; con -DNIX_DESIGNAR_DIR=/ruta se usa una copia local (sin red).
set(NIX_DESIGNAR_DIR "" CACHE PATH "Copia local de DeSiGNAR (vacío = descargarla de GitHub)")
set(DESIGNAR_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(DESIGNAR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
if(NIX_DESIGNAR_DIR)
  set(designar_raiz "${NIX_DESIGNAR_DIR}")
  if(EXISTS "${designar_raiz}/CMakeLists.txt")
    add_subdirectory("${designar_raiz}" designar EXCLUDE_FROM_ALL)
  endif()
else()
  include(FetchContent)
  FetchContent_Declare(designar
    GIT_REPOSITORY https://github.com/r3mmurd/DeSiGNAR.git
    GIT_TAG main
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(designar)
  set(designar_raiz "${designar_SOURCE_DIR}")
endif()
if(TARGET Designar::Designar)
  set(NIX_DESIGNAR_OBJETIVO Designar::Designar)
elseif(TARGET Designar)
  set(NIX_DESIGNAR_OBJETIVO Designar)
endif()

# ───────── Nix: biblioteca con todo el código, programa y pruebas ─────────
file(GLOB NIX_FUENTES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER NIX_FUENTES EXCLUDE REGEX ".*/main\\.cpp$")
add_library(nix_nucleo STATIC ${NIX_FUENTES})
target_include_directories(nix_nucleo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${designar_raiz}/include")
target_link_libraries(nix_nucleo PUBLIC Threads::Threads CURL::libcurl ${NIX_DESIGNAR_OBJETIVO})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(nix_nucleo PRIVATE -Wall -Wextra)
endif()

add_executable(nix src/main.cpp)
target_link_libraries(nix PRIVATE nix_nucleo)

enable_testing()
foreach(prueba IN ITEMS tests_fase1 tests_fase2 tests tests_closet tests_servidor tests_api)
  add_executable(${prueba} src/pruebas/${prueba}.cpp)
  target_link_libraries(${prueba} PRIVATE nix_nucleo)
  add_test(NAME ${prueba} COMMAND ${prueba})
endforeach()

install(TARGETS nix RUNTIME DESTINATION bin)
