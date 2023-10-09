cmake_minimum_required(VERSION 3.12)
project(MyGameEngine)

# Find and link GLFW library
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

# Add your source files
add_executable(MyGameEngine src/main.cpp)

# Link GLFW library
target_link_libraries(MyGameEngine ${GLFW_LIBRARIES})