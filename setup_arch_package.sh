#!/bin/bash
set -e

if [[ "$(uname)" != "Linux" ]] || ! command -v pacman > /dev/null; then
    echo "This script can only be run on Arch-based Linux!"
    exit 1
fi

# Запрос sudo в самом начале
sudo echo > /dev/null

# 1. Сборка бинарника, если еще не собран
if [[ ! -f build/RCC ]]; then
    bash compile.sh
fi

# 2. Подготовка каталога пакета
rm -rf rcc_pkg
mkdir -p rcc_pkg
cp build/RCC rcc_pkg/
cp assets/icon.png rcc_pkg/rcc.png
cd rcc_pkg

export PACKAGER="Arta <arta@gmail.com>"

# 3. Генерация PKGBUILD
cat > PKGBUILD << 'EOF'
# Maintainer: Arta <arta@gmail.com>
pkgname=rcc
pkgver=1.0.0
pkgrel=1
pkgdesc="Royal Card Club"
arch=("x86_64")
url="https://github.com/Arta48/RCC"
options=(!debug)
depends=(
    qt6-base
    qt6-multimedia
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
)

prepare() {
    cd "${srcdir}"
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        magick rcc.png -resize "${size}x${size}" -gravity center -background transparent -extent "${size}x${size}" "icon-${size}.png"
    done

    cat > "${pkgname}.desktop" << EOD
[Desktop Entry]
Type=Application
Categories=Game;CardGame;
Name=Royal Card Club
Name[ru]=Royal Card Club
Exec=RCC
Icon=rcc
Terminal=false
StartupWMClass=rcc
EOD
}

package() {
    cd "${srcdir}"
    install -Dm755 RCC "${pkgdir}/opt/rcc/RCC"
    install -d "${pkgdir}/usr/bin"
    ln -s /opt/rcc/RCC "${pkgdir}/usr/bin/RCC"

    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        install -Dm644 "icon-${size}.png" "${pkgdir}/usr/share/icons/hicolor/${size}x${size}/apps/rcc.png"
    done

    install -Dm644 "${pkgname}.desktop" "${pkgdir}/usr/share/applications/${pkgname}.desktop"
}
EOF

# 4. Сборка и установка в систему
makepkg -si --skipinteg --noconfirm

cd .. && rm -rf rcc_pkg

echo "
Done!
"
