#!/bin/bash
set -euo pipefail

CATCH2_VERSION="3.14.0"
CATCH2_URL="https://github.com/catchorg/Catch2/archive/refs/tags/v${CATCH2_VERSION}.tar.gz"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
EXTERNAL_DIR="$PROJECT_DIR/external"
BUILD_DIR="$EXTERNAL_DIR/Catch2/build"
INSTALL_DIR="$EXTERNAL_DIR/Catch2/install"

echo "==> Setting up Catch2 v${CATCH2_VERSION} for LaTeX-Fmt"

mkdir -p "$EXTERNAL_DIR"

if [ -d "$INSTALL_DIR" ]; then
    echo "Catch2 already installed at: $INSTALL_DIR"
    echo "To reinstall, remove it first:  rm -rf $EXTERNAL_DIR/Catch2"
    exit 0
fi

TARBALL="$EXTERNAL_DIR/catch2-${CATCH2_VERSION}.tar.gz"

if [ ! -f "$TARBALL" ]; then
    echo "==> Downloading Catch2 v${CATCH2_VERSION} ..."
    if command -v wget &>/dev/null; then
        wget -q --show-progress -O "$TARBALL" "$CATCH2_URL"
    elif command -v curl &>/dev/null; then
        curl -L -o "$TARBALL" "$CATCH2_URL"
    else
        echo "ERROR: wget or curl required to download Catch2"
        exit 1
    fi
fi

echo "==> Extracting ..."
rm -rf "$EXTERNAL_DIR/Catch2-src"
mkdir -p "$EXTERNAL_DIR/Catch2-src"
tar xzf "$TARBALL" -C "$EXTERNAL_DIR/Catch2-src" --strip-components=1

echo "==> Building Catch2 ..."
mkdir -p "$BUILD_DIR"
cmake -S "$EXTERNAL_DIR/Catch2-src" -B "$BUILD_DIR" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DCATCH_BUILD_EXAMPLES=OFF \
    -DCATCH_BUILD_EXTRA_TESTS=OFF

cmake --build "$BUILD_DIR" --target install -j"$(nproc 2>/dev/null || echo 4)"

rm -rf "$EXTERNAL_DIR/Catch2-src" "$TARBALL" "$BUILD_DIR"

echo ""
echo "==> Catch2 v${CATCH2_VERSION} installed to: $INSTALL_DIR"
echo "==> Now run:  mkdir -p build && cd build && cmake .. -DBUILD_TESTS=ON && make -j\$(nproc) && ctest"
