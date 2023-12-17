#pragma once

#include <sol/sol.hpp>

#include <string>

struct ScriptComponent
{
    std::string Name;
    sol::table ScriptClass;
    ScriptComponent(std::string name, sol::table scriptClass) : Name(name), ScriptClass(scriptClass)
    {
        sol::function ready = scriptClass["ready"];
        try
        {
            ready(scriptClass);
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