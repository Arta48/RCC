#!/bin/bash

if [[ "$(uname)" != "Darwin" ]]; then
    echo "This script can only be run on macOS!"
    exit 1
fi


# 1. Очистка старой сборки
rm -rf build-ios
mkdir -p build-ios

# 2. Автоматический поиск Qt для iOS (если не задана переменная QT_ROOT_DIR)
if [ -z "$CMAKE_PREFIX_PATH" ] && [ -z "$QT_ROOT_DIR" ]; then
    QT_IOS_DIR=$(find "$HOME/Qt" -maxdepth 3 -type d -name "ios" 2>/dev/null | sort -V | tail -n 1)
    if [ -n "$QT_IOS_DIR" ]; then
        echo "Found Qt iOS at: $QT_IOS_DIR"
        export CMAKE_PREFIX_PATH="$QT_IOS_DIR"
    fi
fi

# 3. Конфигурация через CMake для Xcode
cmake -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0

# 4. Компиляция под архитектуру реального iPhone (Release)
cd build-ios
xcodebuild -target RCC -configuration Release -sdk iphoneos \
  CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO

# 5. Упаковка в IPA
mkdir -p ipa_root/Payload
cp -R "Release-iphoneos/Royal Card Club.app" ipa_root/Payload/

# Автоматическая нарезка иконок под стандарты iOS
APP_DIR=$(find ipa_root/Payload -name "*.app" | head -n 1)
sips -z 180 180 ../assets/icon.png --out "$APP_DIR/AppIcon60x60@3x.png"
sips -z 120 120 ../assets/icon.png --out "$APP_DIR/AppIcon60x60@2x.png"
sips -z 152 152 ../assets/icon.png --out "$APP_DIR/AppIcon76x76@2x~ipad.png"

cd ipa_root
zip -qr ../RCC.ipa Payload
cd ..
rm -rf ipa_root

# 6. Уведомление
echo "
Done!
"
