#pragma once

#include <sol/sol.hpp>

#include <string>

struct ScriptComponent
{
    std::string Name;
    sol::table ScriptClass;
    ScriptComponent(std::string name, sol::table scriptClass) : Name(name), ScriptClass(scriptClass){};
};