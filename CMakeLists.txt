cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(cppengine)

file(GLOB_RECURSE SOURCES
"src/*.cpp"
"include/*.h"
"include/*.hpp"
"external/glad/src/*.c"
)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

# Handle Dependencies

set(OpenGL_GL_PREFERENCE "GLVND") # get latest version
find_package(OpenGL REQUIRED)

add_subdirectory(external/lua-cmake)
add_subdirectory(external/sol2)

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
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dl -ldl)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

# Add ImGui source files to your project
add_library(imgui
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp # TODO: remove this
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    # Add other ImGui source files as needed
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
target_link_libraries(imgui PUBLIC glfw)

include_directories(
    include
    external/headers
    external/glad/include
    external/lua-cmake/upstream
    external/sol2/include
    )
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE external/glm)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glad glfw imgui lua sol2)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
