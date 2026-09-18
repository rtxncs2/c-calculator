#!/usr/bin/env bash
set -e

PKG_NAME="c-calculator"
PKG_VERSION="1.0.0"
ARCH=$(dpkg --print-architecture 2>/dev/null || echo "amd64")
ROOT_DIR="${PKG_NAME}_${PKG_VERSION}_${ARCH}"

echo "==> Installing build dependencies..."
sudo apt update -y && sudo apt install -y gcc libgtk-3-dev pkg-config

echo "==> Compiling binary..."
gcc -O2 calculator.c -o "${PKG_NAME}" $(pkg-config --cflags --libs gtk+-3.0)

echo "==> Creating Debian package directory tree..."
rm -rf "${ROOT_DIR}"
mkdir -p "${ROOT_DIR}/DEBIAN"
mkdir -p "${ROOT_DIR}/usr/bin"
mkdir -p "${ROOT_DIR}/usr/share/applications"

# Copy the compiled binary
cp "${PKG_NAME}" "${ROOT_DIR}/usr/bin/${PKG_NAME}"
chmod 755 "${ROOT_DIR}/usr/bin/${PKG_NAME}"

# Create Debian control metadata
cat <<EOF > "${ROOT_DIR}/DEBIAN/control"
Package: ${PKG_NAME}
Version: ${PKG_VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Depends: libgtk-3-0
Maintainer: Developer <dev@example.com>
Description: Lightweight C and GTK-3 GUI Calculator
 A minimal, fast desktop calculator written in C using GTK 3.
EOF

# Create desktop launcher file
cat <<EOF > "${ROOT_DIR}/usr/share/applications/${PKG_NAME}.desktop"
[Desktop Entry]
Name=C Calculator
Comment=Simple C GTK Calculator
Exec=/usr/bin/${PKG_NAME}
Icon=accessories-calculator
Terminal=false
Type=Application
Categories=Utility;Calculator;
EOF

echo "==> Packaging into .deb file..."
dpkg-deb --build "${ROOT_DIR}"

echo "==> Cleaning up build files..."
rm -rf "${ROOT_DIR}" "${PKG_NAME}"

echo "==> Done! Created: ${ROOT_DIR}.deb"
