cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
project(MyGameEngine)

find_package(OpenGL REQUIRED)

if (APPLE)
    include(FetchContent)

    FetchContent_Declare(
            GLFW
            URL https://github.com/glfw/glfw/archive/master.zip
        )

    FetchContent_MakeAvailable(GLFW)

    FetchContent_Declare(
        GLAD
        URL https://github.com/Dav1dde/glad/archive/master.zip
    )

    FetchContent_MakeAvailable(GLAD)

else() # WSL
    add_subdirectory(lib/glad)
    find_package(glfw3 REQUIRED)
endif()

add_executable(MyGameEngine src/main.cpp src/GameObject.cpp src/GameManager.cpp)

include_directories(include)
target_link_libraries(MyGameEngine PRIVATE glad glfw)

target_link_libraries(MyGameEngine PRIVATE ${OPENGL_LIBRARIES})
