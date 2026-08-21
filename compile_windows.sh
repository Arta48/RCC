#!/bin/bash
set -e

if [[ "$(uname)" == "Darwin" ]]; then
    echo "This script cannot be run on macOS!"
    exit 1
fi

# 1. Очистка старой сборки
rm -rf build-windows
mkdir -p build-windows

# 2. Конфигурация и сборка через CMake
if [[ "$(uname)" == *"MINGW"* ]] || [[ "$(uname)" == *"MSYS"* ]]; then # Github Actions
    cmake -S . -B build-windows -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    cmake --build build-windows -j$(nproc 2>/dev/null || echo 2)
else # Linux
    x86_64-w64-mingw32-cmake -B build-windows -DCMAKE_BUILD_TYPE=Release
    cmake --build build-windows -j$(nproc 2>/dev/null || echo 2)
fi

echo "
Done!
"
