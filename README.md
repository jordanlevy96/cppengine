# Jordan's cppengine

This is an experimental project to explore C++ and OpenGL.

TODO: come up with a name

## Development Setup

On Windows, compile using CMake, then open the resulting .sln with Visual Studio.

With WSL-based development, you need to run an X11 server like X410. Make sure the DISPLAY environment variable is set to 127.0.0.1:0.

On MacOS and Linux, install dependencies, then run the below:

```sh
mkdir build
cd build
cmake ..
make
./cppengine
```

### External Dependencies

- C++ Compiler (G++)\*
- CMake\*
- GLFW†
- GLAD†
- stb_image†
- tiny_obj_loader†
- yaml-cpp‡
- GLM‡
- Dear ImGUI‡
- Lua‡
  - lua-cmake‡
  - sol2‡

\*<sub><sup>Required installation</sup></sub>\
†<sub><sup>Included with CMake or in external</sup></sub>\
‡<sub><sup>Set up with a git submodule; run `cd external && git submodule update --init --recursive` to initialize.</sup></sub>
