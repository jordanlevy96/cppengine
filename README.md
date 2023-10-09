# Game Engine

This is an experimental project to explore C++ and GLFW.

## Development Setup
With WSL, you need to run an X11 server like X410. Make sure the DISPLAY environment variable is set to 127.0.0.1:0 for local development.
I also had to install a C++ compiler (G++), CMake, and GLFW.
```sh
mkdir build
cd build
cmake ..
make
./MyGameEngine
```
