cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
project(MyGameEngine)

find_package(OpenGL REQUIRED)

# TODO: fix FetchContent
# include(FetchContent)

# FetchContent_Declare(
#     GLFW
#     URL https://github.com/glfw/glfw/archive/master.zip
# )

# FetchContent_MakeAvailable(GLFW)

# FetchContent_Declare(
#     GLAD
#     URL https://github.com/Dav1dde/glad/archive/master.zip
# )

# FetchContent_MakeAvailable(GLAD)

# target_include_directories(MyGameEngine PRIVATE
#     ${glfw_SOURCE_DIR}/include
#     ${GLAD_SOURCE_DIR}/include
# )

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

add_subdirectory(lib/glad)

add_executable(MyGameEngine src/main.cpp src/GameObject.cpp src/GameManager.cpp)
include_directories(include)

target_link_libraries(MyGameEngine PRIVATE ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
target_link_libraries(MyGameEngine PRIVATE glad)
