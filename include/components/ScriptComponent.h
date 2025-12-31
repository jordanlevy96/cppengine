#pragma once

#include <string>
#include <sol/sol.hpp>
#include <pybind11/pybind11.h>
#include "util/Logger.h"

namespace py = pybind11;

enum class ScriptType
{
    Lua,
    Python
};

struct ScriptComponent
{
    std::string Name;
    ScriptType Type;

    // Script objects - only one will be valid depending on Type
    sol::table LuaClass;
    py::object PythonClass;

    // Lua constructor
    ScriptComponent(std::string name, sol::table luaClass)
        : Name(name), Type(ScriptType::Lua), LuaClass(luaClass)
    {
        sol::function ready = luaClass["ready"];
        try
        {
            ready(luaClass);
        }
        catch (const sol::error &e)
        {
            LOG_ERROR("Error calling Lua Ready: {}", e.what());
        }
    }

    // Python constructor
    ScriptComponent(std::string name, py::object pythonClass)
        : Name(name), Type(ScriptType::Python), PythonClass(pythonClass)
    {
    }

    ~ScriptComponent()
    {
        if (Type == ScriptType::Lua)
        {
            LuaClass.abandon();
        }
    }
};
