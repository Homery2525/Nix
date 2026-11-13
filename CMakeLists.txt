cmake_minimum_required(VERSION 3.20)
project(Nix VERSION 0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
# Este hito solo usa sockets: no necesita libcurl.

# ───────── DeSiGNAR (https://github.com/r3mmurd/DeSiGNAR) ─────────
# Nix usa su lista doblemente enlazada (Designar::DLList) a través de include/Estructuras.hpp.
# Cuando el proyecto se compila sin internet o sin la dependencia externa, usamos el fallback local
# incluido en include/list.hpp para que el código siga compilando.
set(NIX_DESIGNAR_DIR "" CACHE PATH "Copia local de DeSiGNAR (vacío = usar fallback local)")
set(DESIGNAR_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(DESIGNAR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NIX_DESIGNAR_OBJETIVO "")
if(NIX_DESIGNAR_DIR)
  set(designar_raiz "${NIX_DESIGNAR_DIR}")
  if(EXISTS "${designar_raiz}/CMakeLists.txt")
    add_subdirectory("${designar_raiz}" designar EXCLUDE_FROM_ALL)
    if(TARGET Designar::Designar)
      set(NIX_DESIGNAR_OBJETIVO Designar::Designar)
    elseif(TARGET Designar)
      set(NIX_DESIGNAR_OBJETIVO Designar)
    endif()
  endif()
elseif(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/list.hpp")
  include(FetchContent)
  FetchContent_Declare(designar
    GIT_REPOSITORY https://github.com/r3mmurd/DeSiGNAR.git
    GIT_TAG main
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(designar)
  set(designar_raiz "${designar_SOURCE_DIR}")
  if(TARGET Designar::Designar)
    set(NIX_DESIGNAR_OBJETIVO Designar::Designar)
  elseif(TARGET Designar)
    set(NIX_DESIGNAR_OBJETIVO Designar)
  endif()
endif()

if(NIX_DESIGNAR_DIR AND NOT NIX_DESIGNAR_OBJETIVO)
  message(FATAL_ERROR "NIX_DESIGNAR_DIR must point to a DeSiGNAR checkout that defines Designar::Designar or Designar")
endif()

# ───────── Nix: biblioteca con todo el código, programa y pruebas ─────────
file(GLOB NIX_FUENTES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER NIX_FUENTES EXCLUDE REGEX ".*/main\\.cpp$")
add_library(nix_nucleo STATIC ${NIX_FUENTES})
if(NIX_DESIGNAR_OBJETIVO)
  # Prioriza DeSiGNAR sobre el list.hpp local, que solo sirve como fallback offline.
  target_include_directories(nix_nucleo BEFORE PUBLIC "${designar_raiz}/include")
  message(STATUS "Using DeSiGNAR from ${designar_raiz}")
else()
  message(STATUS "Using bundled list.hpp fallback; set NIX_DESIGNAR_DIR to use DeSiGNAR")
endif()
target_include_directories(nix_nucleo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(nix_nucleo PUBLIC Threads::Threads ${NIX_DESIGNAR_OBJETIVO})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(nix_nucleo PRIVATE -Wall -Wextra)
endif()

add_executable(nix src/main.cpp)
target_link_libraries(nix PRIVATE nix_nucleo)

enable_testing()
foreach(prueba IN ITEMS tests_fase1 tests_fase2 tests tests_closet tests_api)
  add_executable(${prueba} src/pruebas/${prueba}.cpp)
  target_link_libraries(${prueba} PRIVATE nix_nucleo)
  add_test(NAME ${prueba} COMMAND ${prueba})
endforeach()

install(TARGETS nix RUNTIME DESTINATION bin)
