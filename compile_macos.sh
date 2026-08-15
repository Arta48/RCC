#!/bin/bash

if [[ "$(uname)" != "Darwin" ]]; then
    echo "This script can only be run on macOS!"
    exit 1
fi

# 1. Попытка добавить пути к Qt6 из Homebrew в PATH
if command -v brew &> /dev/null; then
    BREW_PREFIX=$(brew --prefix)
    export PATH="$BREW_PREFIX/opt/qt/bin:$BREW_PREFIX/opt/qt6/bin:$PATH"
fi

# 2. Очистка старой сборки
rm -rf build-macos
mkdir -p build-macos

# 3. Конфигурация и компиляция
echo "Configuring and compiling project..."
cmake -B build-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos -j$(sysctl -n hw.ncpu)

# 4. Линковка внешних библиотек Qt и упаковка
if command -v macdeployqt &> /dev/null; then
    echo "Packaging standalone APP bundle..."
    # -no-codesign отключает встроенную проверку подписей, которая ломается на библиотеках Homebrew.
    # Перенаправляем stderr в stdout и скрываем ложные предупреждения rpath с помощью grep.
    macdeployqt "build-macos/Royal Card Club.app" -no-codesign 2>&1 | grep -v -E "Cannot resolve rpath|using QList|using QSet"
    
    echo "Applying local (ad-hoc) code signature to all components..."
    # Глубоко и принудительно подписываем все исполняемые файлы и dylib внутри бандла
    codesign --force --deep --sign - "build-macos/Royal Card Club.app"
    
    echo "Creating DMG installer..."
    if command -v create-dmg &> /dev/null; then
        # 4.1. Создаем временную папку для структуры будущего диска
        mkdir -p build-macos/dmg_root
    
        # 2. Копируем приложение во временную папку
        cp -R "build-macos/Royal Card Club.app" build-macos/dmg_root/

        # 4.3. Сборка DMG через create-dmg
        create-dmg \
          --volname "RCC Installer" \
          --window-pos 200 120 \
          --window-size 600 450 \
          --icon-size 100 \
          --icon "Royal Card Club.app" 150 150 \
          --hide-extension "Royal Card Club.app" \
          --app-drop-link 450 150 \
          "build-macos/RCC.dmg" \
          build-macos/dmg_root/ > /dev/null 2>&1
    
        # 4.4. Очищаем временную папку сборки
        rm -rf build-macos/dmg_root

        echo "Done! Standalone app was compiled, signed, and successfully packaged into the DMG installer."
        echo "The DMG file is located in: build-macos/RCC.dmg"
    else
        echo "Warning: create-dmg not found. Creating DMG skipped."
        echo "The standalone APP bundle is located in: build-macos/Royal Card Club.app"
    fi
else
    echo "Warning: macdeployqt not found. Deployment skipped."
    echo "The 'dependent' APP bundle is located in 'build-macos/Royal Card Club.app'."
fi
