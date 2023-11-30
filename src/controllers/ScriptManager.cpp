#include "controllers/App.h"
#include "controllers/Registry.h"
#include "controllers/ScriptManager.h"
#include "util/Uniform.h"

#include <iostream>

void ScriptManager::Run(const char *scriptSrc)
{
    lua.open_libraries(sol::lib::base, sol::lib::table, sol::lib::os, sol::lib::math);
    lua.script_file(scriptSrc);
}

void ScriptManager::Initialize()
{
    LuaBindings::RegisterEnums(lua);
    LuaBindings::RegisterTypes(lua);
    LuaBindings::RegisterFunctions(lua);

    Run("../res/scripts/init.lua");
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
        lua.new_enum("UniformTypeMap",
                     "BOOL", UniformTypeMap::b,
                     "INT", UniformTypeMap::i,
                     "FLOAT", UniformTypeMap::f,
                     "VEC2", UniformTypeMap::vec2,
                     "VEC3", UniformTypeMap::vec3,
                     "VEC4", UniformTypeMap::vec4,
                     "MAT2", UniformTypeMap::mat2,
                     "MAT3", UniformTypeMap::mat3,
                     "MAT4", UniformTypeMap::mat4);

        lua.new_enum("InputTypes",
                     "KEY", InputTypes::Key,
                     "CLICK", InputTypes::Click,
                     "CURSOR", InputTypes::Cursor,
                     "RESIZE", InputTypes::Resize,
                     "SCROLL", InputTypes::Scroll);
    }

    void RegisterTypes(sol::state &lua)
    {
        lua.new_usertype<glm::vec3>("vec3",
                                    sol::call_constructor, sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>());

        lua.new_usertype<App>("App",
                              "GetInstance", &App::GetInstance,
                              "registry", &App::registry,
                              "cam", &App::cam,
                              "CloseWindow", &App::CloseWindow);

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
        // lua.set_function("CloseWindow", &App::CloseWindow);
    }
}
