#!/bin/bash
# Creates distributable macOS app bundle with bundled Python
# Usage: ./scripts/package-macos.sh
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-release"
PYTHON_URL="https://github.com/indygreg/python-build-standalone/releases/download/20241016/cpython-3.13.0+20241016-aarch64-apple-darwin-install_only.tar.gz"

echo "=== Imhotep macOS Packaging Script ==="
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"

# Create build directory
mkdir -p "$BUILD_DIR"

echo ""
echo "=== Preparing Bundled Python ==="
mkdir -p "$BUILD_DIR/python-bundle"
if [ ! -f "$BUILD_DIR/python-bundle/bin/python3" ]; then
    echo "Downloading portable Python 3.13 for ARM64..."
    curl -L "$PYTHON_URL" | tar xz -C "$BUILD_DIR/python-bundle" --strip-components=1
    echo "Python downloaded and extracted."
else
    echo "Python bundle already exists, skipping download."
fi

echo ""
echo "=== Installing Python Packages ==="
"$BUILD_DIR/python-bundle/bin/pip3" install pandas matplotlib numpy --quiet
echo "Packages installed: pandas, matplotlib, numpy"

echo ""
echo "=== Building Imhotep ==="
cmake -B "$BUILD_DIR" -S "$PROJECT_ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DIMHOTEP_BUNDLE_PYTHON=ON \
    -DIMHOTEP_PYTHON_BUNDLE_PATH="$BUILD_DIR/python-bundle" \
    -DPython3_EXECUTABLE="$BUILD_DIR/python-bundle/bin/python3" \
    -DPython3_LIBRARY="$BUILD_DIR/python-bundle/lib/libpython3.13.dylib" \
    -DPython3_INCLUDE_DIR="$BUILD_DIR/python-bundle/include/python3.13"

cmake --build "$BUILD_DIR" -j8

echo ""
echo "=== Creating App Bundle ==="
cmake --install "$BUILD_DIR" --prefix "$BUILD_DIR/dist"

echo ""
echo "=== Fixing Library Paths ==="
APP="$BUILD_DIR/dist/imhotep.app"

# Fix core library ID and references
if [ -f "$APP/Contents/Frameworks/libcore.dylib" ]; then
    install_name_tool -id "@rpath/libcore.dylib" "$APP/Contents/Frameworks/libcore.dylib"
    echo "Fixed libcore.dylib ID"
fi

# Fix engine module references
if [ -f "$APP/Contents/Frameworks/engine.cpython-313-darwin.so" ]; then
    install_name_tool -change "libcore.dylib" "@rpath/libcore.dylib" \
        "$APP/Contents/Frameworks/engine.cpython-313-darwin.so" 2>/dev/null || true
    echo "Fixed engine module references"
fi

# Fix executable references to core
if [ -f "$APP/Contents/MacOS/imhotep" ]; then
    install_name_tool -change "libcore.dylib" "@rpath/libcore.dylib" \
        "$APP/Contents/MacOS/imhotep" 2>/dev/null || true
    echo "Fixed imhotep executable references"
fi

echo ""
echo "=== Build Complete ==="
echo "App bundle created at: $APP"
echo ""
echo "To test the bundle:"
echo "  open $APP"
echo ""
echo "To create a distributable DMG (optional):"
echo "  hdiutil create -volname 'Imhotep' -srcfolder '$APP' -ov -format UDZO '$BUILD_DIR/Imhotep.dmg'"
