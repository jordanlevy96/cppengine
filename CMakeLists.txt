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

elseif(WIN32)
    add_subdirectory(external/glad)
    include(FetchContent)


	# Specify where to get GLFW and what version to use
	FetchContent_Declare(
		GLFW
		GIT_REPOSITORY https://github.com/glfw/glfw.git
		GIT_TAG 3.3.8 # Specify Release Version
	)
	# Get and build GLFW
	FetchContent_MakeAvailable(GLFW)
else() # Linux
    add_subdirectory(external/glad)
    find_package(glfw3 REQUIRED)
endif()

# Build Instructions
file(GLOB_RECURSE SOURCES "src/*.cpp" "external/glad/src/*.c" "include/*.h" "include/*.hpp")
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

include_directories(include)
include_directories(external/glad/include)
target_include_directories(MyGameEngine PRIVATE external/glm)

target_link_libraries(MyGameEngine PRIVATE glad glfw)
target_link_libraries(MyGameEngine PRIVATE ${OPENGL_LIBRARIES})
