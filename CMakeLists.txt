cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(cppengine)

file(GLOB_RECURSE SOURCES
"src/*.cpp"
"src/controllers/*.cpp"
"src/controllers/python/*.cpp"
"src/components/*.cpp"
"src/entities/*.cpp"
"src/systems/*.cpp"
"src/util/*.cpp"
"include/*.h"
"include/controllers/*.h"
"include/components/*.h"
"include/entities/*.h"
"include/systems/*.h"
"include/util/*.h"
"external/glad/src/*.c"
)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

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

if(SCRIPTING_LANG STREQUAL "LUA")
    message(STATUS "Configuring for Lua")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build as static library" FORCE)
    add_subdirectory(external/lua-cmake)
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
        external/lua-cmake/upstream
        external/sol2/include/sol
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE lua sol2)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_LUA_SCRIPTING)
elseif(SCRIPTING_LANG STREQUAL "PYTHON")
    find_package(pybind11 REQUIRED)
    find_package(PythonLibs REQUIRED)
    include_directories(${PYTHON_INCLUDE_DIRS} external/pybind11/include)
    set(Python_EXECUTABLE "${CONDA_PREFIX}/bin/python")
    set(CONDA_PREFIX $ENV{CONDA_PREFIX})
    if (CONDA_PREFIX)
        set(Python_EXECUTABLE "${CONDA_PREFIX}/bin/python")
        include_directories("${CONDA_PREFIX}/include/python3.12")
    else()
        message(FATAL_ERROR "CONDA_PREFIX env var not set.")
    endif()
    pybind11_add_module(app_module src/controllers/python/app_bindings.cpp)
    target_include_directories(app_module PRIVATE external/glm)
    target_link_libraries(app_module PRIVATE glfw ${OPENGL_LIBRARIES})
    target_compile_definitions(app_module PRIVATE USE_PYTHON_SCRIPTING)

    pybind11_add_module(camera src/controllers/python/camera_bindings.cpp)
    target_include_directories(camera PRIVATE external/glm)
    target_link_libraries(camera PRIVATE glfw ${OPENGL_LIBRARIES})
    target_compile_definitions(camera PRIVATE USE_PYTHON_SCRIPTING)

    pybind11_add_module(enums src/controllers/python/enum_bindings.cpp)
    target_include_directories(enums PRIVATE external/glm)
    target_link_libraries(enums PRIVATE glfw ${OPENGL_LIBRARIES})
    target_compile_definitions(enums PRIVATE USE_PYTHON_SCRIPTING)

    pybind11_add_module(glm src/controllers/python/glm_bindings.cpp)
    target_include_directories(glm PRIVATE external/glm)
    target_link_libraries(glm PRIVATE glfw ${OPENGL_LIBRARIES})
    target_compile_definitions(glm PRIVATE USE_PYTHON_SCRIPTING)

    pybind11_add_module(tetris src/controllers/python/tetris_bindings.cpp)
    target_include_directories(tetris PRIVATE external/glm)
    target_link_libraries(tetris PRIVATE glfw ${OPENGL_LIBRARIES})
    target_compile_definitions(tetris PRIVATE USE_PYTHON_SCRIPTING)

    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${PYTHON_LIBRARIES})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_PYTHON_SCRIPTING)
endif()

include_directories(
    include
    external/headers
    external/glad/include
    external/yaml-cpp/include
    ${GLFW_DIR}
    ${IMGUI_DIR}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE external/glm)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw imgui yaml-cpp::yaml-cpp ${OPENGL_LIBRARIES})
