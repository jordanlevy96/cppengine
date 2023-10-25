cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)
project(MyGameEngine)

# Handle Dependencies

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

# elseif (WIN32) 
    # add_subdirectory(/glad)
else() 
    add_subdirectory(external/glad)
    find_package(glfw3 REQUIRED)
endif()

# Build Instructions
file(GLOB_RECURSE SOURCES "src/*.cpp" "external/glad/src/*.c")
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

include_directories(include)
include_directories(external/glad/include)
target_include_directories(MyGameEngine PRIVATE external/glm)

target_link_libraries(MyGameEngine PRIVATE glad dl glfw)
target_link_libraries(MyGameEngine PRIVATE ${OPENGL_LIBRARIES})
