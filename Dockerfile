# ───────── Etapa 1: compilar y probar (todo el proyecto es C++17) ─────────
FROM debian:bookworm-slim AS compilar
RUN apt-get update && apt-get install -y --no-install-recommends g++ make cmake git ca-certificates tzdata libcurl4-openssl-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt ./
COPY include/ include/
COPY src/ src/
# DeSiGNAR se descarga de GitHub durante `cmake` (necesita internet al construir la imagen).
# Si una prueba falla, la imagen no se construye.
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc" \
 && cmake --build build -j"$(nproc)" \
 && ctest --test-dir build --output-on-failure

# ───────── Etapa 2: imagen final (un solo programa + la interfaz web) ─────────
FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends libcurl4 ca-certificates tzdata && rm -rf /var/lib/apt/lists/* \
 && useradd --create-home --uid 1000 nix && mkdir /data && chown nix /data
COPY --from=compilar /src/build/nix /usr/local/bin/nix
COPY web/ /app/web/
ENV JARVIS_DATA=/data JARVIS_WEB=/app/web PORT=8080
USER nix
VOLUME /data
EXPOSE 8080
HEALTHCHECK --interval=30s --timeout=5s CMD ["nix", "--salud"]
CMD ["nix"]
