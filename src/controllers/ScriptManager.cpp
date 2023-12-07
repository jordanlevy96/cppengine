#include "Camera.h"
#include "controllers/App.h"
#include "controllers/Registry.h"
#include "controllers/ScriptManager.h"
#include "util/Uniform.h"

#include "Tetris.h"

#include <iostream>

void ScriptManager::Run(const std::string &scriptSrc)
{
    lua.script_file(scriptSrc);
}

void ScriptManager::Initialize()
{
    LuaBindings::RegisterEnums(lua);
    LuaBindings::RegisterTypes(lua);
    LuaBindings::RegisterFunctions(lua);
    lua.open_libraries(sol::lib::base, sol::lib::table, sol::lib::os, sol::lib::math);

    Run(App::GetInstance().conf.ResourcePath + "scripts/init.lua");
}

void ScriptManager::Shutdown()
{
    // Clear Lua references to C++ singletons
    lua["GameManager"] = sol::lua_nil;

    // Run garbage collector
    lua.collect_garbage();

    // The Lua state itself will be destroyed when the ScriptManager instance goes out of scope
}

void ScriptManager::ProcessInput()
{
    sol::function handleInputFunction = lua[HANDLE_INPUT_F];
    try
    {
        handleInputFunction();
    }
    catch (const sol::error &e)
    {
        std::cerr << "Error calling HandleInput: " << e.what() << std::endl;
    }
}

namespace LuaBindings
{
    void RegisterEnums(sol::state &lua)
    {
        lua.new_enum("InputTypes",
                     "KEY", InputTypes::Key,
                     "CLICK", InputTypes::Click,
                     "CURSOR", InputTypes::Cursor,
                     "RESIZE", InputTypes::Resize,
                     "SCROLL", InputTypes::Scroll);

        lua.new_enum("CameraDirections",
                     "FORWARD", CameraDirections::FORWARD,
                     "BACK", CameraDirections::BACK,
                     "LEFT", CameraDirections::LEFT,
                     "RIGHT", CameraDirections::RIGHT);

        lua.new_enum("Rotations",
                     "CW", Tetris::Rotations::CW,
                     "CCW", Tetris::Rotations::CCW);
    }

    void RegisterTypes(sol::state &lua)
    {
        lua.new_usertype<glm::vec2>("vec2",
                                    sol::call_constructor, sol::constructors<glm::vec2(float, float)>(),
                                    "x", &glm::vec2::x,
                                    "y", &glm::vec2::y);

        lua.new_usertype<glm::vec3>("vec3",
                                    sol::call_constructor, sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
                                    "x", &glm::vec3::x,
                                    "y", &glm::vec3::y,
                                    "z", &glm::vec3::z);

        lua.new_usertype<Transform>("Transform",
                                    "Pos", &Transform::Pos);

        lua.new_usertype<Camera>("Camera",
                                 "transform", &Camera::transform,
                                 "fov", &Camera::fov,
                                 "front", &Camera::front,
                                 // casting is necessary here because the function is overloaded, which Lua does not support
                                 "SetPerspective", std::function<void(Camera *, float)>(static_cast<void (Camera::*)(float)>(&Camera::SetPerspective)),
                                 "Move", &Camera::Move,
                                 "RotateByMouse", &Camera::RotateByMouse);

        lua.new_usertype<App>("App",
                              "GetInstance", &App::GetInstance,
                              "delta", &App::delta,
                              "registry", &App::registry,
                              "camera", &App::cam,
                              "conf", &App::conf,
                              "window", &App::windowManager);

        lua.new_usertype<Config>("Config",
                                 "resPath", &Config::ResourcePath);

        lua.new_usertype<WindowManager>("Window",
                                        "CloseWindow", &WindowManager::CloseWindow,
                                        "GetSize", &WindowManager::GetSize);

        lua.new_usertype<Registry>("Registry",
                                   "GetInstance", &Registry::GetInstance,
                                   "GetEntityByName", &Registry::GetEntityByName);

        lua.new_usertype<InputEvent>(
            "InputEvent",
            "type", &InputEvent::type,
            "input", &InputEvent::input);
    }

    void RegisterFunctions(sol::state &lua)
    {
        lua.set_function("CreateRenderComponent", &Registry::CreateRenderComponent);
        lua.set_function("AttachScript", &Registry::AttachScript);
        lua.set_function("CreateCube", &Registry::CreateCube);

        lua.set_function("CreateTetrimino", &Tetris::CreateTetrimino);
        lua.set_function("GetChildMap", &Tetris::GetChildMap);
        lua.set_function("RotateTetrimino", &Tetris::RotateTetrimino);
        lua.set_function("MoveTetrimino", &Tetris::MoveTetrimino);
        lua.set_function("TweenTetrimino", &Tetris::TweenTetrimino);
    }
}
