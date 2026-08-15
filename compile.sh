#!/bin/bash

if [[ "$(uname)" != "Linux" ]]; then
    echo "This script can only be run on Linux!"
    exit 1
fi

# 1. Очистка старой сборки
rm -rf build
mkdir -p build

# 2. Конфигурация и сборка через CMake
cmake -B build
cmake --build build -j$(nproc) # Сборка на всех ядрах процессора

# (Опционально) Установка
# sudo cmake --install build

# 3. Уведомление
echo "
Done!
"
