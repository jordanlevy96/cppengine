# Build Instructions (ARM64 Mac)

## Problem
CMake finds x86_64 Python instead of ARM64 Python on Apple Silicon Macs.

## Solution
Force CMake to use ARM64 Python from Homebrew:

```bash
cd /Users/jordan/dev/cppengine/build
rm -rf *

cmake \
  -DPython3_EXECUTABLE=/opt/homebrew/bin/python3.13 \
  -DPython3_LIBRARY=/opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/lib/libpython3.13.dylib \
  -DPython3_INCLUDE_DIR=/opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/include/python3.13 \
  ..

make -j8
```

## Verification
Check that Python libraries show ARM64:
```bash
file /opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/Python
# Should show: Mach-O 64-bit dynamically linked shared library arm64
```

Last successful build: 2026-01-01
