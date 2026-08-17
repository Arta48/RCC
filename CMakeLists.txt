cmake_minimum_required(VERSION 3.16)

project(RCC)
set(EXE_NAME RCC)

# --- Настройки стандартов C++ и Qt ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Настройки для Windows ---
if(WIN32)
    set(QT_NO_PACKAGE_VERSION_CHECK TRUE)
    set(QT_NO_PACKAGE_VERSION_INCOMPATIBLE_WARNING TRUE)

    # --- Настройки для Windows из-под Linux ---
    if (CMAKE_CROSSCOMPILING)
        # Используем CACHE PATH, чтобы это можно было переопределить из консоли при желании
        set(STATIC_BASE "/usr/x86_64-w64-mingw32/static" CACHE PATH "Path to static base")

        # Сначала ищем в статической Qt, потом в обычном mingw
        set(CMAKE_FIND_ROOT_PATH "${STATIC_BASE};/usr/x86_64-w64-mingw32")
    # --- Настройки для Windows из-под Windows ---
    else()
        if(DEFINED ENV{MINGW_PREFIX})
            # Динамически считываем путь MSYS2 (работает и на GitHub Actions, и локально)
            set(STATIC_BASE "$ENV{MINGW_PREFIX}/qt6-static" CACHE PATH "Path to static base")
            # Сначала ищем в статической Qt, потом в обычном mingw
            set(CMAKE_FIND_ROOT_PATH "${STATIC_BASE};$ENV{MINGW_PREFIX}")
        elseif(EXISTS "C:/msys64")
            # Используем CACHE PATH, чтобы это можно было переопределить из консоли при желании
            set(STATIC_BASE "C:/msys64/mingw64/qt6-static" CACHE PATH "Path to static base")

            # Сначала ищем в статической Qt, потом в обычном mingw
            set(CMAKE_FIND_ROOT_PATH "${STATIC_BASE};C:/msys64/mingw64")
        endif()
    endif()

    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

    set(CMAKE_PREFIX_PATH "${STATIC_BASE}")
    set(Qt6_DIR "${STATIC_BASE}/lib/cmake/Qt6")

    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

    # Линковка системных библиотек компилятора
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

    # Принудительно подключаем Winsock2 и Crypt32 в самый конец линковки
    set(CMAKE_CXX_STANDARD_LIBRARIES "-lws2_32 -lcrypt32 ${CMAKE_CXX_STANDARD_LIBRARIES}")

    add_definitions(-DQT_STATIC)
endif()

# --- Поиск зависимостей ---
if(WIN32)
    # --- Настройки для Windows ---
    find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Network Multimedia)

    # --- Настройки для Windows из-под Linux ---
    if(CMAKE_CROSSCOMPILING)
        set(LIB_PATH "/usr/x86_64-w64-mingw32/lib")
    # --- Настройки для Windows из-под Windows ---
    else()
        if(DEFINED ENV{MINGW_PREFIX})
            set(LIB_PATH "$ENV{MINGW_PREFIX}/lib")
        elseif(EXISTS "C:/msys64")
            set(LIB_PATH "C:/msys64/mingw64/lib")
        endif()
    endif()

    # Принудительно только статические библиотеки
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

    # 1. Harfbuzz + Graphite2
    find_library(HARFBUZZ_STATIC NAMES harfbuzz PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(GRAPHITE2 NAMES graphite2 PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)

    if(TARGET harfbuzz::harfbuzz)
        set_target_properties(harfbuzz::harfbuzz PROPERTIES
            IMPORTED_LOCATION         "${HARFBUZZ_STATIC}"
            IMPORTED_IMPLIB           "${HARFBUZZ_STATIC}"
            IMPORTED_LOCATION_RELEASE "${HARFBUZZ_STATIC}"
            IMPORTED_IMPLIB_RELEASE   "${HARFBUZZ_STATIC}"
            INTERFACE_LINK_LIBRARIES  "${GRAPHITE2}"
        )
    endif()

    # 2. Freetype + Brotli + BZip2 + ZSTD
    find_library(BROTLIDEC NAMES brotlidec PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(BROTLI_COMMON NAMES brotlicommon PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(BZ2 NAMES bz2 PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(ZSTD_STATIC NAMES zstd PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)

    if(TARGET Freetype::Freetype)
        set_target_properties(Freetype::Freetype PROPERTIES INTERFACE_LINK_LIBRARIES "${HARFBUZZ_STATIC};${BROTLIDEC};${BROTLI_COMMON};${BZ2};${ZSTD_STATIC}")
    endif()

    # 3. WrapZSTD
    if(TARGET WrapZSTD)
        set_target_properties(WrapZSTD PROPERTIES INTERFACE_LINK_LIBRARIES "${ZSTD_STATIC}")
    endif()
else()
    # --- Настройки для Linux и macOS ---
    # --- Исправление бага Qt на macOS ---
    if(APPLE AND NOT IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT TARGET WrapOpenGL::WrapOpenGL)
        find_package(OpenGL REQUIRED)
        add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE OpenGL::GL)
    endif()

    find_package(Qt6 REQUIRED COMPONENTS Widgets Network Multimedia)
endif()

# --- Исходные файлы ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/*/*.cpp" "src/*/*.h")

# --- Ресурсы ---
set(RESOURCES assets/resources.qrc)
qt_add_resources(SOURCES ${RESOURCES})

# --- Сборка ---
if(WIN32)
    # --- Настройки для Windows ---
    set(WIN_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/assets/resource.rc")

    # ВАЖНО: Включаем ресурсы напрямую в список файлов
    qt_add_executable(${EXE_NAME}
        WIN32
        MANUAL_FINALIZATION
        ${SOURCES}
        ${WIN_RESOURCES}
    )

    # Линковка (порядок важен для статических библиотек!)
    target_link_libraries(${EXE_NAME} PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Network Qt6::Multimedia)

    # --- Настройки для Windows из-под Windows ---
    if (NOT CMAKE_CROSSCOMPILING)
        # Добавляем системные зависимости libtiff и harfbuzz
        set(CMAKE_CXX_STANDARD_LIBRARIES "-llzma -llerc -ldeflate -ljbig -ljpeg -lrpcrt4 ${CMAKE_CXX_STANDARD_LIBRARIES}")
    endif()

    # Финализация (обязательна для Qt6 на Windows)
    qt_finalize_executable(${EXE_NAME})

elseif(IOS)
    # --- Сборка под iOS ---
    set(IOS_ICON assets/icon.png)
    set(IOS_STORYBOARD assets/ios/LaunchScreen.storyboard)
    set_source_files_properties(${IOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${IOS_STORYBOARD} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    add_executable(${EXE_NAME} MACOSX_BUNDLE ${SOURCES} ${IOS_ICON} ${IOS_STORYBOARD})

    set_target_properties(${EXE_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/assets/ios/Info.plist.in
        MACOSX_BUNDLE_ICON_FILE icon.png
        MACOSX_BUNDLE_BUNDLE_NAME "Royal Card Club"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.arta48.rcc"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.arta48.rcc"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "16.0"
        QT_IOS_LAUNCH_SCREEN "${CMAKE_CURRENT_SOURCE_DIR}/assets/ios/LaunchScreen.storyboard"
    )

    # Исключаем FFmpeg и принудительно используем нативный медиа-плагин Apple AVFoundation
    qt_import_plugins(${EXE_NAME}
        INCLUDE Qt6::QDarwinMediaPlugin
        EXCLUDE Qt6::QFFmpegMediaPlugin
    )

    target_link_libraries(${EXE_NAME} PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::Multimedia
        "-framework AVFoundation"
        "-framework AudioToolbox"
    )
else()
    if(APPLE)
        # --- Настройки для macOS ---
        # На macOS собираем как Bundle для генерации .app директории
        set(MACOSX_ICON assets/macos/icon.icns)
        set(MACOSX_LOCALES
            assets/macos/locales/ru.lproj/InfoPlist.strings
            assets/macos/locales/en.lproj/InfoPlist.strings
        )
        # Размечаем ресурсы внутри бандла .app
        set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set_source_files_properties(assets/macos/locales/ru.lproj/InfoPlist.strings PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/ru.lproj")
        set_source_files_properties(assets/macos/locales/en.lproj/InfoPlist.strings PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/en.lproj")

        add_executable(${EXE_NAME} MACOSX_BUNDLE ${SOURCES} ${MACOSX_ICON} ${MACOSX_LOCALES})

        # Задаем метаданные бандла и подключаем кастомный Info.plist.in
        set_target_properties(${EXE_NAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/assets/macos/Info.plist.in
            MACOSX_BUNDLE_ICON_FILE icon.icns
            MACOSX_BUNDLE_BUNDLE_NAME "Royal Card Club"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.arta48.rcc"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
            MACOSX_BUNDLE_COPYRIGHT "Copyright © 2026 Arta48"
            OUTPUT_NAME "Royal Card Club" # Задает правильное имя папки .app
        )
    else()
        # --- Настройки для Linux ---
        # Обычная сборка для Linux
        add_executable(${EXE_NAME} ${SOURCES})
    endif()

    target_link_libraries(${EXE_NAME} PRIVATE Qt6::Widgets Qt6::Network Qt6::Multimedia)
endif()
