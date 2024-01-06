cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(cppengine)

# file(GLOB_RECURSE SOURCES
# "src/*.cpp"
# "src/controllers/*.cpp"
# "src/controllers/python/*.cpp"
# "src/components/*.cpp"
# "src/systems/*.cpp"
# "src/util/*.cpp"
# "include/*.h"
# "include/controllers/*.h"
# "include/components/*.h"
# "include/systems/*.h"
# "include/util/*.h"
# "external/glad/src/*.c"
# )

add_library(core_lib SHARED
    "src/Camera.cpp"
    "src/Tetris.cpp"
    "src/controllers/App.cpp"
    "src/controllers/Registry.cpp"
    "src/controllers/ResourceManager.cpp"
    "src/controllers/ScriptManager.cpp"
    "src/controllers/WindowManager.cpp"
    "src/main.cpp"
    "src/systems/RenderSystem.cpp"
    "src/systems/ScriptSystem.cpp"
    "src/systems/TweenSystem.cpp"
    "src/systems/UI.cpp"
    "src/util/Mesh.cpp"
    "src/util/Shader.cpp"
    "src/util/TransformUtils.cpp"
    "external/glad/src/glad.c"
)

set(SCRIPTING_LANG "PYTHON")
message(STATUS "SCRIPTING_LANG set to " ${SCRIPTING_LANG})

set(OpenGL_GL_PREFERENCE "GLVND") 
find_package(OpenGL REQUIRED)

add_subdirectory(external/yaml-cpp)
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
    find_package(glfw3 REQUIRED)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dl -ldl)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_demo.cpp # TODO: remove this
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
target_link_libraries(imgui PUBLIC glfw)

set(CORE_LIB_INCLUDE_DIRS
    include
    external/headers
    external/glm
    external/glad/include
    external/yaml-cpp/include
    ${GLFW_DIR}
    ${IMGUI_DIR}
)

set(CORE_LIB_LINKS
    glfw
    imgui
    yaml-cpp::yaml-cpp
    ${OPENGL_LIBRARIES}
)

target_include_directories(core_lib PUBLIC ${CORE_LIB_INCLUDE_DIRS})
target_link_libraries(core_lib PUBLIC ${CORE_LIB_LINKS})

if(SCRIPTING_LANG STREQUAL "LUA")
    message(STATUS "Configuring for Lua")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build as static library" FORCE)
    target_add_subdirectory(core_lib external/lua-cmake)
    target_include_directories(core_lib PUBLIC
        external/lua-cmake/upstream
        external/sol2/include/sol
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE lua sol2)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_LUA_SCRIPTING)
elseif(SCRIPTING_LANG STREQUAL "PYTHON")
    find_package(pybind11 REQUIRED)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS "Configuring for Python")
    message(STATUS ${PYTHON_INCLUDE_DIRS})
    target_include_directories(core_lib PUBLIC ${PYTHON_INCLUDE_DIRS} external/pybind11/include)
    set(Python_EXECUTABLE "${CONDA_PREFIX}/bin/python")
    set(CONDA_PREFIX $ENV{CONDA_PREFIX})
    if (CONDA_PREFIX)
        set(Python_EXECUTABLE "${CONDA_PREFIX}/bin/python")
        include_directories("${CONDA_PREFIX}/include/python3.12")
    else()
        message(FATAL_ERROR "CONDA_PREFIX env var not set.")
    endif()

    pybind11_add_module(app_module src/controllers/python/app_bindings.cpp)
    pybind11_add_module(camera src/controllers/python/camera_bindings.cpp)
    pybind11_add_module(enums src/controllers/python/enum_bindings.cpp)
    pybind11_add_module(glm src/controllers/python/glm_bindings.cpp)
    pybind11_add_module(tetris src/controllers/python/tetris_bindings.cpp)
    
    foreach(module app_module camera enums glm tetris)
        # target_include_directories(${module} PRIVATE ${CORE_LIB_INCLUDE_DIRS})
        target_link_libraries(${module} PUBLIC core_lib) # ${CORE_LIB_LINKS}
    endforeach()

    target_link_libraries(core_lib PUBLIC ${PYTHON_LIBRARIES})
    target_compile_definitions(core_lib PUBLIC USE_PYTHON_SCRIPTING)
endif()

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE core_lib)
