#!/bin/bash
# Cross-platform export script for Imhotep engine games
#
# Usage:
#   ./scripts/export.sh --game vaporqube              # Build for current OS
#   ./scripts/export.sh --game vaporqube --skip-python # Without Python
#
# Output:
#   macOS  → dist/<AppName>-macos.dmg
#   Linux  → dist/<AppName>-linux.tar.gz
#   Windows → dist/<AppName>-windows.zip

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# ----- Parse arguments -----
GAME=""
SKIP_PYTHON=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --game)     GAME="$2"; shift 2 ;;
        --skip-python) SKIP_PYTHON=true; shift ;;
        *)          echo "Unknown option: $1"; exit 1 ;;
    esac
done

if [ -z "$GAME" ]; then
    echo "Usage: $0 --game <game-name> [--skip-python]"
    exit 1
fi

# ----- Validate game directory -----
GAME_DIR="$PROJECT_ROOT/res/games/$GAME"
SETTINGS_FILE="$GAME_DIR/conf/settings.yaml"

if [ ! -f "$SETTINGS_FILE" ]; then
    echo "Error: Game config not found: $SETTINGS_FILE"
    exit 1
fi

# ----- Read appName from settings.yaml -----
APP_NAME=$(grep 'appName:' "$SETTINGS_FILE" | head -1 | sed 's/.*appName:[[:space:]]*//')
if [ -z "$APP_NAME" ]; then
    echo "Error: Could not read appName from $SETTINGS_FILE"
    exit 1
fi

echo "=== Imhotep Export Script ==="
echo "Game:     $GAME"
echo "App Name: $APP_NAME"
echo "Python:   $([ "$SKIP_PYTHON" = true ] && echo 'disabled' || echo 'enabled')"

# ----- Detect OS -----
case "$(uname -s)" in
    Darwin*)  OS="macos" ;;
    Linux*)   OS="linux" ;;
    MINGW*|MSYS*|CYGWIN*) OS="windows" ;;
    *)        echo "Error: Unsupported OS: $(uname -s)"; exit 1 ;;
esac
echo "Platform: $OS"
echo ""

BUILD_DIR="$PROJECT_ROOT/build-export"
DIST_DIR="$PROJECT_ROOT/dist"

mkdir -p "$BUILD_DIR"
mkdir -p "$DIST_DIR"

# ----- CMake configure flags -----
CMAKE_FLAGS=(
    -DCMAKE_BUILD_TYPE=Release
    -DIMHOTEP_GAME_NAME="$APP_NAME"
    -DIMHOTEP_GAME_CONFIG="games/$GAME/conf/settings.yaml"
)

if [ "$SKIP_PYTHON" = true ]; then
    CMAKE_FLAGS+=(-DIMHOTEP_ENABLE_PYTHON=OFF)
fi

if [ "$OS" = "macos" ]; then
    CMAKE_FLAGS+=(-DIMHOTEP_BUILD_BUNDLE=ON)

    # Bundled Python for macOS (if not skipped)
    if [ "$SKIP_PYTHON" = false ]; then
        PYTHON_BUNDLE="$BUILD_DIR/python-bundle"
        PYTHON_URL="https://github.com/indygreg/python-build-standalone/releases/download/20241016/cpython-3.13.0+20241016-aarch64-apple-darwin-install_only.tar.gz"

        if [ ! -f "$PYTHON_BUNDLE/bin/python3" ]; then
            echo "=== Downloading Bundled Python ==="
            mkdir -p "$PYTHON_BUNDLE"
            curl -L "$PYTHON_URL" | tar xz -C "$PYTHON_BUNDLE" --strip-components=1
        fi

        CMAKE_FLAGS+=(
            -DIMHOTEP_BUNDLE_PYTHON=ON
            -DIMHOTEP_PYTHON_BUNDLE_PATH="$PYTHON_BUNDLE"
            -DPython3_EXECUTABLE="$PYTHON_BUNDLE/bin/python3"
            -DPython3_LIBRARY="$PYTHON_BUNDLE/lib/libpython3.13.dylib"
            -DPython3_INCLUDE_DIR="$PYTHON_BUNDLE/include/python3.13"
        )
    fi
fi

# ----- Build -----
echo "=== Configuring ==="
cmake -B "$BUILD_DIR" -S "$PROJECT_ROOT" "${CMAKE_FLAGS[@]}"

echo ""
echo "=== Building ==="
cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"

echo ""
echo "=== Installing ==="
INSTALL_DIR="$BUILD_DIR/install"
rm -rf "$INSTALL_DIR"
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

# ----- Post-process per platform -----
echo ""
echo "=== Packaging ($OS) ==="

if [ "$OS" = "macos" ]; then
    APP_BUNDLE="$INSTALL_DIR/${APP_NAME}.app"

    # Fix library IDs and references
    if [ -f "$APP_BUNDLE/Contents/Frameworks/libcore.dylib" ]; then
        install_name_tool -id "@rpath/libcore.dylib" \
            "$APP_BUNDLE/Contents/Frameworks/libcore.dylib"
    fi

    # Fix executable references
    if [ -f "$APP_BUNDLE/Contents/MacOS/$APP_NAME" ]; then
        install_name_tool -change "libcore.dylib" "@rpath/libcore.dylib" \
            "$APP_BUNDLE/Contents/MacOS/$APP_NAME" 2>/dev/null || true
    fi

    # Fix engine module references (if Python enabled)
    for so in "$APP_BUNDLE"/Contents/Frameworks/engine.*.so; do
        [ -f "$so" ] && install_name_tool -change "libcore.dylib" "@rpath/libcore.dylib" \
            "$so" 2>/dev/null || true
    done

    # Create DMG
    OUTPUT="$DIST_DIR/${APP_NAME}-macos.dmg"
    rm -f "$OUTPUT"
    hdiutil create -volname "$APP_NAME" -srcfolder "$APP_BUNDLE" \
        -ov -format UDZO "$OUTPUT"

    echo ""
    echo "=== Done ==="
    echo "Output: $OUTPUT"

elif [ "$OS" = "linux" ]; then
    OUTPUT="$DIST_DIR/${APP_NAME}-linux.tar.gz"
    tar -czf "$OUTPUT" -C "$INSTALL_DIR" "$APP_NAME"

    echo ""
    echo "=== Done ==="
    echo "Output: $OUTPUT"

elif [ "$OS" = "windows" ]; then
    OUTPUT="$DIST_DIR/${APP_NAME}-windows.zip"
    (cd "$INSTALL_DIR" && zip -r "$OUTPUT" "$APP_NAME")

    echo ""
    echo "=== Done ==="
    echo "Output: $OUTPUT"
fi
