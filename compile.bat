@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

set "MSYS2_DIR=C:\msys64"

REM 0. Подготовка среды (MSYS2)
if not exist "%MSYS2_DIR%\usr\bin\bash.exe" (
    winget --version >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        winget install MSYS2.MSYS2 --force --accept-source-agreements --accept-package-agreements --location %MSYS2_DIR%
    )
    if not exist "%MSYS2_DIR%\usr\bin\bash.exe" (
        curl -L -o msys2_installer.exe https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-x86_64-latest.exe
        start /wait "" msys2_installer.exe --mode unattended --confirm-command --accept-messages --root %MSYS2_DIR%
        del msys2_installer.exe
    )
)

set "LIST=mingw-w64-x86_64-qt6-static mingw-w64-x86_64-cmake mingw-w64-x86_64-toolchain mingw-w64-x86_64-pcre2 mingw-w64-x86_64-icu mingw-w64-x86_64-libpng mingw-w64-x86_64-libjpeg-turbo mingw-w64-x86_64-freetype mingw-w64-x86_64-harfbuzz mingw-w64-x86_64-brotli mingw-w64-x86_64-zstd mingw-w64-x86_64-double-conversion mingw-w64-x86_64-libwebp mingw-w64-x86_64-libtiff mingw-w64-x86_64-vulkan-headers mingw-w64-qt6-multimedia-static"
"%MSYS2_DIR%\usr\bin\bash.exe" -lc "pacman -S --noconfirm --needed !LIST!"

set "PATH=%MSYS2_DIR%\mingw64\bin;%MSYS2_DIR%\usr\bin;%PATH%"

REM 1. Очистка старой сборки
rmdir /s /q build-windows
mkdir build-windows

REM 2. Конфигурация и сборка через CMake
cmake -S . -B build-windows -G "MinGW Makefiles"
cmake --build build-windows -j %NUMBER_OF_PROCESSORS%

REM 3. Уведомление
echo.
echo Done^^!
echo.
echo P.S. For full cleaning delete %MSYS2_DIR% folder^^!
echo.
pause
