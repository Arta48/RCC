#!/bin/bash
set -e

if [[ "$(uname)" != "Linux" ]]; then
    echo "This script can only be run on Linux!"
    exit 1
fi

# 1. Определение путей Android SDK и NDK
if [ -z "$ANDROID_SDK_ROOT" ] && [ -n "$ANDROID_HOME" ]; then
    export ANDROID_SDK_ROOT="$ANDROID_HOME"
fi

if [ -z "$ANDROID_NDK_ROOT" ]; then
    if [ -n "$ANDROID_NDK_LATEST_HOME" ]; then
        export ANDROID_NDK_ROOT="$ANDROID_NDK_LATEST_HOME"
    else
        NDK_DIR=$(find "$ANDROID_SDK_ROOT/ndk" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | sort -V | tail -n 1)
        export ANDROID_NDK_ROOT="$NDK_DIR"
    fi
fi

# 2. Поиск Qt для Android
if [ -z "$QT_ANDROID_DIR" ]; then
    if [ -n "$RUNNER_WORKSPACE" ]; then
        QT_ANDROID_DIR=$(find "$RUNNER_WORKSPACE/Qt" -maxdepth 3 -type d -name "android_arm64_v8a" | head -n 1)
    else
        QT_ANDROID_DIR=$(find "$HOME/Qt" -maxdepth 3 -type d -name "android_arm64_v8a" 2>/dev/null | sort -V | tail -n 1)
    fi
fi

# 3. Очистка старой сборки
rm -rf build-android RCC.apk
mkdir -p build-android assets/android/res/drawable
cp assets/icon.png assets/android/res/drawable/icon.png

# 4. Конфигурация и компиляция APK
"$QT_ANDROID_DIR/bin/qt-cmake" -B build-android -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQT_ANDROID_ABIS="arm64-v8a" \
  -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
  -DANDROID_NDK_ROOT="$ANDROID_NDK_ROOT"

cmake --build build-android

# 5. Автоматическая подпись APK
keytool -genkey -v -keystore release.keystore -alias rcc_key -keyalg RSA -keysize 2048 -validity 10000 \
  -storepass android -keypass android -dname "CN=RoyalCardClub,O=Arta48,C=RU" 2>/dev/null || true

RAW_APK=$(find build-android -name "*.apk" | head -n 1)
APKSIGNER=$(find "$ANDROID_SDK_ROOT/build-tools" -name "apksigner" | sort -V | tail -n 1)

"$APKSIGNER" sign --ks release.keystore --ks-pass pass:android --ks-key-alias rcc_key --key-pass pass:android --out RCC.apk "$RAW_APK"

echo "
Done!
"
