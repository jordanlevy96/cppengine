# Imhotep

**Version: 0.1.0**

An experimental C++ game engine exploring declarative UI systems for complex, data-driven games.

See [CHANGELOG.md](CHANGELOG.md) for version history.

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

\*<sub><sup>System dependency - required installation</sup></sub>\
†<sub><sup>Auto-downloaded via CMake FetchContent</sup></sub>\
‡<sub><sup>Git submodule - run `cd external && git submodule update --init --recursive` to initialize</sup></sub>

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
