#!/bin/bash
# Build script: compiles Main.cpp -> WebAssembly bang Emscripten
# Chay thu local: ./build.sh, sau do emrun dist/index.html

set -e

mkdir -p dist

emcc Main.cpp -o dist/index.html \
  -sUSE_SDL=2 \
  -sUSE_SDL_IMAGE=2 \
  -sUSE_SDL_TTF=2 \
  -sUSE_SDL_MIXER=2 \
  -sSDL2_IMAGE_FORMATS='["png"]' \
  --preload-file resources \
  -sALLOW_MEMORY_GROWTH=1 \
  -sNO_EXIT_RUNTIME=1 \
  -O2

echo ""
echo "Build xong! Cac file nam trong thu muc dist/"
echo "Chay thu bang lenh: emrun dist/index.html"