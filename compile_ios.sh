#!/bin/bash

if [[ "$(uname)" != "Darwin" ]]; then
    echo "This script can only be run on macOS!"
    exit 1
fi


# 1. Очистка старой сборки
rm -rf build-ios
mkdir -p build-ios

# 2. Конфигурация через CMake для Xcode
cmake -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

# 3. Компиляция под архитектуру реального iPhone (Release)
cd build-ios
xcodebuild -target RCC -configuration Release -sdk iphoneos \
  CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO

# 4. Упаковка в IPA
mkdir -p ipa_root/Payload
cp -R "Release-iphoneos/Royal Card Club.app" ipa_root/Payload/

cd ipa_root
zip -qr ../RCC.ipa Payload
cd ..
rm -rf ipa_root

# 5. Уведомление
echo "
Done!
"
