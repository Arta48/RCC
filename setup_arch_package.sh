#!/bin/bash

if [[ "$(uname)" != "Linux" ]] || ! command -v pacman > /dev/null; then
    echo "This script can only be run on Arch-based Linux!"
    exit 1
fi


# sudo at the beginning
sudo echo > /dev/null


if [[ ! -f build/RCC ]]; then
    sh compile.sh || exit 1
fi


mkdir rcc_pkg
cp build/RCC rcc_pkg
cp assets/icon.png rcc_pkg/rcc.png
cd rcc_pkg


# Info about Packager
export PACKAGER="Arta <arta@gmail.com>"


echo '# Maintainer: Arta <arta@gmail.com>
pkgname=rcc
pkgver=1.0.0
pkgrel=1
pkgdesc="Royal Card Club"
arch=("x86_64")
url="https://github.com/Arta48/RCC"
depends=(
    qt6-base
    gcc-libs
    glibc
    hicolor-icon-theme
)
makedepends=(
    imagemagick
)
optdepends=(
    "qt6-wayland: native Wayland support"
)
source=(
    "RCC"
    "rcc.png"
)
sha256sums=(
    "SKIP"
    "SKIP"
    "SKIP"
    "SKIP"
)

prepare() {
    cd "${srcdir}"

    # Generate icons of different sizes
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        magick rcc.png -resize "${size}x${size}" -gravity center -background transparent -extent "${size}x${size}" "icon-${size}.png"
    done

    # Создание desktop-файла
    cat > "${pkgname}.desktop" <<EOF
[Desktop Entry]
Type=Application
Categories=Development;Education;Emulator;
Name=Royal Card Club
Name[ru]=Royal Card Club
Exec=rcc
Icon=rcc
Terminal=false
StartupWMClass=rcc
EOF
}

package() {
    cd "${srcdir}"

    # Installing the binary and resources in /opt
    install -Dm755 RCC "${pkgdir}/opt/rcc/RCC"

    # Creating a symbolic link in /usr/bin
    install -d "${pkgdir}/usr/bin"
    ln -s /opt/rcc/RCC "${pkgdir}/usr/bin/rcc"

    # Installing Icons
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        install -Dm644 "icon-${size}.png" "${pkgdir}/usr/share/icons/hicolor/${size}x${size}/apps/rcc.png"
    done

    # Installing a desktop file
    install -Dm644 "${pkgname}.desktop" "${pkgdir}/usr/share/applications/${pkgname}.desktop"
}' > PKGBUILD


makepkg -si --skipinteg --noconfirm


cd .. && rm -rf rcc_pkg


# Status output
echo "

Done!"
