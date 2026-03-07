# Imhotep

**Version: 0.1.0**

An experimental C++ game engine exploring declarative UI systems for complex, data-driven games.

See [CHANGELOG.md](CHANGELOG.md) for version history.

## Content Policy and AI Use

I use AI tools like Codex and Claude Code to assist with technical design and development. I do not use AI generated content or assets; I try to attribute all assets and use permissive licenses like CC0.

## Development Setup

On Windows, compile using CMake, then open the resulting .sln with Visual Studio.

With WSL-based development, you need to run an X11 server like X410. Make sure the DISPLAY environment variable is set to 127.0.0.1:0.

On MacOS and Linux, install dependencies, then run the below:

```sh
mkdir build
cd build
cmake ..
make
./imhotep
```

### Build Options

| CMake Variable | Default | Description |
|---|---|---|
| `IMHOTEP_GAME_NAME` | `imhotep` | Output binary name (e.g., `VaporQube`) |
| `IMHOTEP_GAME_CONFIG` | `games/vaporqube/conf/settings.yaml` | Game config path relative to `res/` |
| `IMHOTEP_ENABLE_PYTHON` | `ON` | Enable Python scripting (requires pybind11) |
| `IMHOTEP_BUILD_TESTS` | `OFF` | Build test executables |
| `IMHOTEP_LOG_LEVEL` | `Info` | Build-time log verbosity |

Example with options:

```sh
cmake -DIMHOTEP_ENABLE_PYTHON=OFF -DIMHOTEP_BUILD_TESTS=ON ..
```

### Automated Testing

Run the automated test suite with CTest:

```sh
cd build
cmake -DIMHOTEP_BUILD_TESTS=ON ..
make -j8
ctest --output-on-failure
```

Current automated suite:
- `vaporqube.lua.behavior` - Validates deterministic Lua gameplay contracts (gravity curve, lifecycle/UI state transitions, piece preview layout, and key input routing).
- `engine.unit` - ExpressionCache unit tests.
- `engine.smoke` - Engine boots and renders 10 frames without crashing.
- `engine.click` - End-to-end UI click event integration test.

### External Dependencies

- C++ Compiler (G++)\*
- CMake\*
- FreeType\*
- Python 3.x\*
- OpenGL†
- GLFW† (Linux: may need `brew install glfw` or `apt-get install libglfw3-dev`)
- GLAD†
- Quill†
- yaml-cpp‡
- GLM‡
- litehtml‡
- Lua‡
  - lua-cmake‡
  - sol2‡
- pybind11‡ (for Python bindings)
- [python-build-standalone](https://github.com/indygreg/python-build-standalone)§ (portable Python for distribution)

\*<sub><sup>System dependency - required installation</sup></sub>\
†<sub><sup>Auto-downloaded via CMake FetchContent</sup></sub>\
‡<sub><sup>Git submodule - run `cd external && git submodule update --init --recursive` to initialize</sup></sub>\
§<sub><sup>Downloaded automatically by packaging script - not needed for development</sup></sub>

#### Installing FreeType

**macOS:**

```sh
brew install freetype
```

**Ubuntu/Debian:**

```sh
sudo apt-get install libfreetype6-dev
```

**Windows:**

- Download from https://www.freetype.org/ or use vcpkg
- Or build from source

### macOS Apple Silicon (ARM64)

CMake auto-detects Homebrew ARM64 Python on Apple Silicon. If it picks the wrong architecture, force it:

```sh
cmake \
  -DPython3_EXECUTABLE=/opt/homebrew/bin/python3.13 \
  -DPython3_LIBRARY=/opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/lib/libpython3.13.dylib \
  -DPython3_INCLUDE_DIR=/opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/include/python3.13 \
  ..
```

### Distribution Builds

Build a distributable package for the current platform:

```sh
./scripts/export.sh --game vaporqube
```

| Platform | Output |
|---|---|
| macOS | `dist/VaporQube-macos.dmg` |
| Linux | `dist/VaporQube-linux.tar.gz` |
| Windows | `dist/VaporQube-windows.zip` |

The script reads `appName` from the game's `settings.yaml` to name the output.

To build without Python scripting support:

```sh
./scripts/export.sh --game vaporqube --skip-python
```

On macOS with Python enabled, a portable Python runtime is downloaded automatically.

**CI**: Push a `v*` tag to trigger GitHub Actions builds for all 3 platforms. Artifacts are attached to the GitHub Release.
