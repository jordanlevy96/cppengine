#pragma once

#include <string>

#ifdef USE_LUA_SCRIPTING

#include <sol/sol.hpp>

struct ScriptComponent
{
    std::string Name;
    sol::table ScriptClass;
    ScriptComponent(std::string name, sol::table luaClass) : Name(name), ScriptClass(luaClass)
    {
        sol::function ready = luaClass["ready"];
        try
        {
            ready(luaClass);
        }
        catch (const sol::error &e)
        {
            std::cerr << "Error calling Ready: " << e.what() << std::endl;
        }
    };
    ~ScriptComponent()
    {
        ScriptClass.abandon();
    }
};

#endif

#ifdef USE_PYTHON_SCRIPTING

#include <pybind11/pybind11.h>

namespace py = pybind11;

struct ScriptComponent
{
    std::string Name;
    py::object ScriptClass;
    ScriptComponent(std::string name, py::object pythonClass) : Name(name), ScriptClass(pythonClass){};

    ~ScriptComponent()
    {
    }
};

#endif