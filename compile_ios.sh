#!/bin/bash
set -e

if [[ "$(uname)" != "Darwin" ]]; then
    echo "This script can only be run on macOS!"
    exit 1
fi

# 1. Поиск путей к Qt iOS и Qt Host
if [ -z "$QT_IOS_DIR" ]; then
    if [ -n "$RUNNER_WORKSPACE" ]; then
        QT_HOST_DIR=$(find "$RUNNER_WORKSPACE/Qt" -type d -name "macos" 2>/dev/null | head -n 1)
        QT_IOS_DIR=$(find "$RUNNER_WORKSPACE/Qt" -type d -name "ios" 2>/dev/null | head -n 1)
    else
        QT_HOST_DIR=$(find "$HOME/Qt" -maxdepth 3 -type d -name "macos" 2>/dev/null | sort -V | tail -n 1)
        QT_IOS_DIR=$(find "$HOME/Qt" -maxdepth 3 -type d -name "ios" 2>/dev/null | sort -V | tail -n 1)
    fi
fi

# 2. Очистка старой сборки
rm -rf build-ios
mkdir -p build-ios

# 3. Конфигурация через CMake для Xcode
cmake -B build-ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE="$QT_IOS_DIR/lib/cmake/Qt6/qt.toolchain.cmake" \
  -DQT_HOST_PATH="$QT_HOST_DIR" \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0

# 4. Компиляция проекта в Release
cd build-ios
xcodebuild -target RCC -configuration Release -sdk iphoneos \
  CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO

# 5. Упаковка в IPA и генерация стандартных иконок
mkdir -p ipa_root/Payload
cp -R Release-iphoneos/*.app ipa_root/Payload/

APP_DIR=$(find ipa_root/Payload -name "*.app" | head -n 1)
sips -z 180 180 ../assets/icon.png --out "$APP_DIR/AppIcon60x60@3x.png"
sips -z 120 120 ../assets/icon.png --out "$APP_DIR/AppIcon60x60@2x.png"
sips -z 152 152 ../assets/icon.png --out "$APP_DIR/AppIcon76x76@2x~ipad.png"

cd ipa_root
zip -qr ../../RCC.ipa Payload
cd ../..
rm -rf build-ios/ipa_root

echo "
Done!
"
