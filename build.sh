#!/usr/bin/env bash
set -eu
if (set -o pipefail) 2>/dev/null; then
    set -o pipefail
fi

# Please install the following packages: unzip (or python3), curl (or wget), cmake

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Configurations
BUILD_TYPE=Release
KERNEL_NAME=omega
CLEAN_BUILD=true

if [ -z "${PFACES_SDK_ROOT:-}" ]; then
    echo "Error: PFACES_SDK_ROOT is not set." >&2
    exit 1
fi

OWL_LIB_DIR="$SCRIPT_DIR/kernel-driver/lib/ltl2dpa/owl/build/native-library"
case "$(uname -s)" in
    Darwin) OWL_LIB_EXT="dylib" ;;
    Linux)  OWL_LIB_EXT="so" ;;
    *)      OWL_LIB_EXT="so" ;;
esac

# Install Owl native library if needed
if [ -f "$OWL_LIB_DIR/libowl.h" ] && [ -f "$OWL_LIB_DIR/libowl.$OWL_LIB_EXT" ]; then
    echo "OWL native library is found ($OWL_LIB_DIR/libowl.$OWL_LIB_EXT)."
else
    echo "OWL native library not found; installing via install-owl.sh..."
    bash "$SCRIPT_DIR/kernel-driver/lib/ltl2dpa/install-owl.sh"
fi

if [ ! -f "$OWL_LIB_DIR/libowl.h" ] || [ ! -f "$OWL_LIB_DIR/libowl.$OWL_LIB_EXT" ]; then
    echo "Error: libowl.h or libowl.$OWL_LIB_EXT missing after install-owl.sh." >&2
    exit 1
fi

# Remove old build binaries
if [ "$CLEAN_BUILD" = "true" ]; then
    rm -rf "$SCRIPT_DIR/kernel-pack/$KERNEL_NAME.driver"
    rm -rf "$SCRIPT_DIR/kernel-pack/libowl.$OWL_LIB_EXT"
    rm -rf "$SCRIPT_DIR/build"
fi

# Build C++ driver
cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$SCRIPT_DIR/build" --config "$BUILD_TYPE" --parallel

if [ ! -f "$SCRIPT_DIR/kernel-pack/$KERNEL_NAME.driver" ]; then
    echo "Error: kernel-pack/$KERNEL_NAME.driver was not built." >&2
    exit 1
fi
if [ ! -f "$SCRIPT_DIR/kernel-pack/libowl.$OWL_LIB_EXT" ]; then
    echo "Error: kernel-pack/libowl.$OWL_LIB_EXT was not colocated." >&2
    exit 1
fi

echo "Build complete: kernel-pack/$KERNEL_NAME.driver and kernel-pack/libowl.$OWL_LIB_EXT"
