#!/bin/bash

if [[ "$(uname)" != "Linux" ]]; then
    echo "This script can only be run on Linux!"
    exit 1
fi

# 1. Поиск Android SDK
if [ -z "$ANDROID_SDK_ROOT" ] && [ -z "$ANDROID_HOME" ]; then
    if [ -d "$HOME/Android/Sdk" ]; then
        export ANDROID_SDK_ROOT="$HOME/Android/Sdk"
    elif [ -d "$HOME/Library/Android/sdk" ]; then
        export ANDROID_SDK_ROOT="$HOME/Library/Android/sdk"
    else
        echo "Error: ANDROID_SDK_ROOT is not set!"
        exit 1
    fi
fi

# 2. Поиск Android NDK
if [ -z "$ANDROID_NDK_ROOT" ]; then
    NDK_DIR=$(find "$ANDROID_SDK_ROOT/ndk" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | sort -V | tail -n 1)
    if [ -n "$NDK_DIR" ]; then
        export ANDROID_NDK_ROOT="$NDK_DIR"
    else
        echo "Error: Android NDK not found in $ANDROID_SDK_ROOT/ndk!"
        exit 1
    fi
fi

# 3. Поиск Qt6 для Android (ARM64)
if [ -z "$QT_ANDROID_DIR" ]; then
    QT_ANDROID_DIR=$(find "$HOME/Qt" -maxdepth 3 -type d -name "android_arm64_v8a" 2>/dev/null | sort -V | tail -n 1)
    if [ -z "$QT_ANDROID_DIR" ]; then
        echo "Error: Qt for Android (android_arm64_v8a) not found in $HOME/Qt!"
        exit 1
    fi
fi

# 4. Очистка старой сборки
rm -rf build-android
mkdir -p build-android

# 5. Подготовка иконки
mkdir -p assets/android/res/drawable
cp assets/icon.png assets/android/res/drawable/icon.png

# 6. Конфигурация и компиляция APK
"$QT_ANDROID_DIR/bin/qt-cmake" -B build-android -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQT_ANDROID_ABIS="arm64-v8a" \
  -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
  -DANDROID_NDK_ROOT="$ANDROID_NDK_ROOT"

cmake --build build-android

# 7. Автоматическая подпись APK
keytool -genkey -v -keystore release.keystore -alias rcc_key -keyalg RSA -keysize 2048 -validity 10000 -storepass android -keypass android -dname "CN=RoyalCardClub,O=Arta48,C=RU" 2>/dev/null || true

RAW_APK=$(find build-android -name "*.apk" | head -n 1)
APKSIGNER=$(find "$ANDROID_SDK_ROOT/build-tools" -name "apksigner" | sort -V | tail -n 1)

"$APKSIGNER" sign --ks release.keystore --ks-pass pass:android --ks-key-alias rcc_key --key-pass pass:android --out RCC.apk "$RAW_APK"

# 8. Уведомление
echo "
Done!
"
