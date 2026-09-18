#!/bin/bash
# build-mac.sh — Compila Tritos Compress en macOS
# Requiere: Xcode Command Line Tools, brew install zlib
set -e

echo "=== Tritos Compress v5 — Build macOS ==="

if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "ERROR: Este script es solo para macOS"
    exit 1
fi

if ! command -v gcc &>/dev/null && ! command -v cc &>/dev/null; then
    echo "Instalando Xcode Command Line Tools..."
    xcode-select --install 2>/dev/null || true
    echo "Reinstala este script despues de la instalacion"
    exit 1
fi

if ! brew list zlib &>/dev/null 2>&1; then
    echo "Instalando zlib via Homebrew..."
    brew install zlib
fi

ZLIB_PREFIX=$(brew --prefix zlib)
CC="${CC:-cc}"

echo "Compilando tritos_compress..."
$CC -o tritos_compress src/ternary_filecompress.c \
    -I"$ZLIB_PREFIX/include" \
    -L"$ZLIB_PREFIX/lib" \
    -lz -lm -O2 -Wall

echo "Creando .pkg..."
mkdir -p pkgroot/usr/local/bin
cp tritos_compress pkgroot/usr/local/bin/

cat > distribution.xml << 'XML'
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>Tritos Compress v5</title>
    <options customize="never" require-scripts="false" hostArchitectures="x86_64,arm64"/>
    <domains enable_localSystem="true"/>
    <choices-outline>
        <line choice="tritos"/>
    </choices-outline>
    <choice id="tritos" title="Tritos Compress">
        <pkg-ref id="com.ternario.tritos-compress"/>
    </choice>
    <pkg-ref id="com.ternario.tritos-compress" version="5.0">tritos-compress.pkg</pkg-ref>
</installer-gui-script>
XML

pkgbuild --root pkgroot \
    --identifier com.ternario.tritos-compress \
    --version 5.0 \
    --install-location / \
    tritos-compress.pkg 2>&1

productbuild --distribution distribution.xml \
    --package-path tritos-compress.pkg \
    TritosCompress-5.0-macOS.pkg 2>&1

rm -rf pkgroot tritos-compress.pkg distribution.xml

echo ""
echo "=== LISTO ==="
ls -lh TritosCompress-5.0-macOS.pkg tritos_compress
echo ""
echo "Uso: ./tritos_compress -c [-m mode] in out"
echo "     ./tritos_compress -d in.trc5 out"
