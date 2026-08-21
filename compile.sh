#!/bin/bash
set -e

if [[ "$(uname)" != "Linux" ]]; then
    echo "This script can only be run on Linux!"
    exit 1
fi

# 1. Очистка старой сборки
rm -rf build
mkdir -p build

# 2. Конфигурация и компиляция через CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc) # Сборка на всех ядрах процессора

echo "
Done!
"
