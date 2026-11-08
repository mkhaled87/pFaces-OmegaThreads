#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OS_RAW="$(uname -s)"
ARCH_RAW="$(uname -m)"

case "$OS_RAW" in
    Darwin)
        NIK_OS="macos"
        ;;
    Linux)
        NIK_OS="linux"
        ;;
    *)
        echo "Unsupported OS: $OS_RAW" >&2
        exit 1
        ;;
esac

case "$ARCH_RAW" in
    arm64|aarch64)
        NIK_ARCH="aarch64"
        ;;
    x86_64|amd64)
        NIK_ARCH="amd64"
        ;;
    *)
        echo "Unsupported architecture: $ARCH_RAW" >&2
        exit 1
        ;;
esac

NIK_TAG="22.3.5+1-11.0.22+12"
NIK_TAG_ENC="22.3.5%2B1-11.0.22%2B12"
NIK_FILE="bellsoft-liberica-vm-openjdk11.0.22+12-22.3.5+1-${NIK_OS}-${NIK_ARCH}.tar.gz"
NIK_URL="https://github.com/bell-sw/LibericaNIK/releases/download/${NIK_TAG_ENC}/bellsoft-liberica-vm-openjdk11.0.22%2B12-22.3.5%2B1-${NIK_OS}-${NIK_ARCH}.tar.gz"

INSTALL_PATH="$SCRIPT_DIR/GraalVM"
GVM_SETUP_FILE="$SCRIPT_DIR/gvm_name.txt"
mkdir -p "$INSTALL_PATH"

find_java_home() {
    local ni_bin
    ni_bin="$(find "$INSTALL_PATH" -maxdepth 5 -path "*/bin/native-image" ! -path "*/lib/svm/bin/native-image" 2>/dev/null | head -n 1 || true)"
    if [ -n "$ni_bin" ]; then
        dirname "$(dirname "$ni_bin")"
        return 0
    fi
    local java_bin
    java_bin="$(find "$INSTALL_PATH" -maxdepth 5 -path "*/bin/java" 2>/dev/null | head -n 1 || true)"
    if [ -n "$java_bin" ]; then
        dirname "$(dirname "$java_bin")"
        return 0
    fi
    return 1
}

JAVA_HOME_DIR="$(find_java_home || true)"
if [ -z "$JAVA_HOME_DIR" ] || [ ! -x "$JAVA_HOME_DIR/bin/java" ]; then
    echo "Installing BellSoft Liberica NIK ($NIK_TAG) for ${NIK_OS}-${NIK_ARCH}..."
    rm -rf "$INSTALL_PATH"/*
    TMP_ARCHIVE="$INSTALL_PATH/$NIK_FILE"
    if command -v curl >/dev/null 2>&1; then
        curl -fSL -o "$TMP_ARCHIVE" "$NIK_URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O "$TMP_ARCHIVE" "$NIK_URL"
    else
        echo "Neither curl nor wget is available to download Liberica NIK." >&2
        exit 1
    fi

    tar -xzf "$TMP_ARCHIVE" -C "$INSTALL_PATH"
    rm -f "$TMP_ARCHIVE"

    JAVA_HOME_DIR="$(find_java_home || true)"
    if [ -z "$JAVA_HOME_DIR" ] || [ ! -x "$JAVA_HOME_DIR/bin/java" ]; then
        echo "Failed to locate JAVA_HOME after extracting Liberica NIK." >&2
        exit 1
    fi
fi

# Store relative path under GraalVM/ in gvm_name.txt
REL_JAVA_HOME="${JAVA_HOME_DIR#"$INSTALL_PATH"/}"
printf '%s\n' "$REL_JAVA_HOME" > "$GVM_SETUP_FILE"

export JAVA_HOME="$JAVA_HOME_DIR"
export GRAAL_HOME="$JAVA_HOME_DIR"
export PATH="$JAVA_HOME/bin:$PATH"

echo "Using JAVA_HOME=$JAVA_HOME"
"$JAVA_HOME/bin/java" -version

if [ ! -e "$JAVA_HOME/bin/native-image" ]; then
    echo "Installing native-image component via gu..."
    "$JAVA_HOME/bin/gu" install native-image
fi
"$JAVA_HOME/bin/native-image" --version

echo "Extracting and building bundled Owl (release-20.06.00)..."
rm -rf "$SCRIPT_DIR/owl"
mkdir -p "$SCRIPT_DIR/owl"
cd "$SCRIPT_DIR/owl"

if command -v unzip >/dev/null 2>&1; then
    unzip -q ../owl-release-20.06.00.zip
else
    python3 -m zipfile -e ../owl-release-20.06.00.zip .
fi
chmod +x ./gradlew

./gradlew clean buildNativeLibrary -Pdisable-pandoc --no-daemon

if [ "$NIK_OS" = "macos" ]; then
    DYLIB_PATH="$SCRIPT_DIR/owl/build/native-library/libowl.dylib"
    if [ ! -f "$DYLIB_PATH" ]; then
        echo "Expected libowl.dylib was not produced at $DYLIB_PATH" >&2
        exit 1
    fi
    install_name_tool -id "@rpath/libowl.dylib" "$DYLIB_PATH"
    if command -v codesign >/dev/null 2>&1; then
        codesign -f -s - "$DYLIB_PATH"
    fi
else
    SO_PATH="$SCRIPT_DIR/owl/build/native-library/libowl.so"
    if [ ! -f "$SO_PATH" ]; then
        echo "Expected libowl.so was not produced at $SO_PATH" >&2
        exit 1
    fi
fi

if [ ! -f "$SCRIPT_DIR/owl/build/native-library/libowl.h" ]; then
    echo "Expected libowl.h was not produced." >&2
    exit 1
fi

echo "Owl native library build completed successfully."
