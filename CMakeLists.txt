cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(imhotep VERSION 0.1.0)

# Generate version header from CMake version
configure_file(
    "${CMAKE_SOURCE_DIR}/include/util/Version.h.in"
    "${CMAKE_SOURCE_DIR}/include/util/Version.h"
    @ONLY
)

add_library(core SHARED
    "src/Camera.cpp"
    "src/controllers/EngineCore.cpp"
    "src/controllers/Game.cpp"
    "src/controllers/Registry.cpp"
    "src/controllers/ResourceManager.cpp"
    "src/controllers/ScriptManager.cpp"
    "src/controllers/WindowManager.cpp"
    "src/systems/HierarchySystem.cpp"
    "src/systems/RenderSystem.cpp"
    "src/systems/ScriptSystem.cpp"
    "src/systems/TweenSystem.cpp"
    "src/systems/UI.cpp"
    "src/systems/HTMLRenderer.cpp"
    "src/systems/HTMLRendererMT.cpp"
    "src/systems/ReactiveUI.cpp"
    "src/systems/LuaUIState.cpp"
    "src/systems/TemplateParser.cpp"
    "src/systems/SceneModule.cpp"
    "src/systems/SceneContext.cpp"
    "src/systems/SceneLoader.cpp"
    "src/util/ConfigLoader.cpp"
    "src/util/FrameTiming.cpp"
    "src/util/Logger.cpp"
    "src/util/Mesh.cpp"
    "src/util/SceneTraversal.cpp"
    "src/util/Shader.cpp"
    "src/util/TransformUtils.cpp"
    "external/glad/src/glad.c"
)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

add_subdirectory(external/yaml-cpp)
add_subdirectory(external/sol2)
add_subdirectory(external/litehtml)

# Quill logging library (all platforms)
include(FetchContent)
set(QUILL_DISABLE_NON_PREFIXED_MACROS ON CACHE BOOL "Disable Quill's non-prefixed LOG_* macros" FORCE)
FetchContent_Declare(
    quill
    GIT_REPOSITORY https://github.com/odygrd/quill.git
    GIT_TAG        v7.4.0
)
FetchContent_MakeAvailable(quill)

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
    include(FetchContent)

    FetchContent_Declare(
        GLAD
        URL https://github.com/Dav1dde/glad/archive/master.zip
    )

    FetchContent_MakeAvailable(GLAD)

	FetchContent_Declare(
		GLFW
		GIT_REPOSITORY https://github.com/glfw/glfw.git
		GIT_TAG 3.3.8 	
    )
	FetchContent_MakeAvailable(GLFW)

else() # Linux
    add_subdirectory(external/glad)
    find_package(glfw3 REQUIRED) # may need to be installed with brew
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dl -ldl)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
target_link_libraries(imgui PUBLIC glfw)

set(core_INCLUDE_DIRS
    include
    external/headers
    external/glm
    external/glad/include
    external/yaml-cpp/include
    external/litehtml/include
    ${GLFW_DIR}
    ${IMGUI_DIR}
    ${FREETYPE_INCLUDE_DIRS}
)

set(core_LINKS
    glfw
    imgui
    yaml-cpp::yaml-cpp
    litehtml
    quill::quill
    ${OPENGL_LIBRARIES}
    ${FREETYPE_LIBRARIES}
)

target_include_directories(core PUBLIC ${core_INCLUDE_DIRS})
target_link_libraries(core PUBLIC ${core_LINKS})

# ============================================================================
# Lua Scripting - Game logic and UI
# ============================================================================
message(STATUS "Configuring Lua scripting")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build as static library" FORCE)
add_subdirectory(external/lua-cmake)
target_include_directories(core PUBLIC
    external/lua-cmake
    external/lua-cmake/upstream
    external/sol2/include/sol
)
target_link_libraries(core PUBLIC lua sol2)
target_compile_definitions(core PUBLIC USE_LUA_SCRIPTING)

# ============================================================================
# Python Scripting - Data analytics, etc.
# ============================================================================
message(STATUS "Configuring Python scripting")

# Find Python - support both conda and system installations
set(CONDA_PREFIX $ENV{CONDA_PREFIX})
if (CONDA_PREFIX)
    message(STATUS "Using Conda environment: ${CONDA_PREFIX}")
    set(Python3_ROOT_DIR "${CONDA_PREFIX}")
    set(Python3_EXECUTABLE "${CONDA_PREFIX}/bin/python")
endif()

find_package(Python3 COMPONENTS Interpreter Development Development.Embed REQUIRED)

# Try to find pybind11 as a system package first, fall back to submodule
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found in system, using submodule")
    add_subdirectory(external/pybind11)
else()
    message(STATUS "Using system pybind11: ${pybind11_DIR}")
endif()

message(STATUS "Python executable: ${Python3_EXECUTABLE}")
message(STATUS "Python include dirs: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python3_LIBRARIES}")
message(STATUS "Python library dirs: ${Python3_LIBRARY_DIRS}")

# pybind11's add_subdirectory already sets up include paths
target_include_directories(core PUBLIC ${Python3_INCLUDE_DIRS})
target_link_libraries(core PUBLIC pybind11::embed ${Python3_LIBRARIES})
target_compile_definitions(core PUBLIC USE_PYTHON_SCRIPTING)

# Create minimal Python module for data export
pybind11_add_module(engine src/controllers/python/engine_bindings.cpp)
target_link_libraries(engine PUBLIC core)

add_executable(imhotep src/main.cpp)
target_link_libraries(imhotep PRIVATE core)

# Editor executable
add_executable(imhotep-editor
    "src/editor/main.cpp"
    "src/editor/Editor.cpp"
    "src/editor/SceneViewport.cpp"
)
target_link_libraries(imhotep-editor PRIVATE core)
target_include_directories(imhotep-editor PRIVATE ${core_INCLUDE_DIRS})
