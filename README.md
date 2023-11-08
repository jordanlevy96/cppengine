# Game Engine

This is an experimental project to explore C++ and OpenGL.

## Development Setup

### External Dependencies

- C++ Compiler (G++)
- CMake
- GLFW
- GLAD
- stb_image
- tiny_obj_loader
- GLM\*

\*Set up with a git submodule; run `git submodule update --init --recursive` to initialize.

With WSL, you need to run an X11 server like X410. Make sure the DISPLAY environment variable is set to 127.0.0.1:0 for local development.

```sh
mkdir build
cd build
cmake ..
make
./MyGameEngine
```
