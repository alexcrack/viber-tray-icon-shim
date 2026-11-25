#!/bin/sh
# Build the LD_PRELOAD shim into ./build/
set -e
cd "$(dirname "$0")"
mkdir -p build
g++ -O2 -fPIC -shared -std=c++17 -Wl,--version-script=src/symbols.map \
    -o build/viber-tray-shim.so src/viber-tray-shim.cpp -ldl
echo "Built build/viber-tray-shim.so"
