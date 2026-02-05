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

### Logging Verbosity

Configure the default log level at build time:

```sh
cmake -DIMHOTEP_LOG_LEVEL=Warning ..
```

Valid values: `TraceL3`, `TraceL2`, `TraceL1`, `Debug`, `Info`, `Warning`, `Error`, `Critical`, `Off`.

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
- Dear ImGui‡
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

### Distribution Build (macOS)

To create a distributable `.app` bundle with bundled Python:

```sh
chmod +x scripts/package-macos.sh
./scripts/package-macos.sh
open build-release/dist/Imhotep.app
```

This downloads a portable Python runtime, installs packages (pandas, matplotlib, numpy), and creates a self-contained app bundle.
