# Hito 1

**Objetivo.** Tener los tipos básicos sobre los que se apoya todo lo demás, sin lógica de lenguaje todavía.

## Qué se añade en este hito
- Core/Estructuras.hpp — ListaEnlazada<T> propia (nodos, iteradores, copia/movimiento, inserción ordenada, borrado).
- Core/Modelos.hpp/.cpp — Tarea y Nota, orden de urgencia y serialización de una línea.
- Core/Texto.hpp — normalización UTF-8 a ASCII con mapa de posiciones y búsqueda de palabras completas.
- Tools/tests_fase1.cpp — pruebas de lo anterior.

## Cómo probarlo
```
bash probar.sh
```
Requiere: g++ con C++17.
