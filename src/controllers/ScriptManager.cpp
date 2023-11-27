#include "controllers/App.h"
#include "controllers/Registry.h"
#include "controllers/ScriptManager.h"
#include "util/Uniform.h"

#include <iostream>

void ScriptManager::Run(const char *scriptSrc)
{
    lua.open_libraries(sol::lib::base, sol::lib::os, sol::lib::math);
    lua.script_file(scriptSrc);
}

void ScriptManager::Initialize()
{
    LuaBindings::RegisterEnums(lua);
    LuaBindings::RegisterTypes(lua);
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
    }

    void RegisterTypes(sol::state &lua)
    {
        lua.new_usertype<glm::vec3>("vec3",
                                    sol::call_constructor, sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>());

        lua.new_usertype<App>("App",
                              "GetInstance", &App::GetInstance,
                              "registry", &App::registry,
                              "cam", &App::cam);

        lua.new_usertype<Registry>("ObjectRegistry",
                                   "GetInstance", &Registry::GetInstance,
                                   "GetEntityByName", &Registry::GetEntityByName);
    }
}
