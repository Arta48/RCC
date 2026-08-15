#!/bin/bash

if [[ "$(uname)" != "Linux" ]]; then
    echo "This script can only be run on Linux!"
    exit 1
fi

# 1. Очистка старой сборки
rm -rf build-windows
mkdir -p build-windows

# 2. Конфигурация и сборка через CMake
x86_64-w64-mingw32-cmake -B build-windows
cmake --build build-windows -j$(nproc)

# 3. Уведомление
echo "
Done!
"
