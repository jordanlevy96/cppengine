#include "systems/ScriptSystem.h"

static ScriptManager &sm = ScriptManager::GetInstance();
static Registry &registry = Registry::GetInstance();

void ScriptSystem::Update(float delta)
{
    for (EntityID id : registry.GetComponentSet<ScriptComponent>().GetEntities())
    {
        ScriptComponent &sc = registry.GetComponent<ScriptComponent>(id);

        if (sc.Type == ScriptType::Lua)
        {
            sol::function updateFunc = sc.LuaClass["process"];
            updateFunc(sc.LuaClass, delta);
        }
        else if (sc.Type == ScriptType::Python)
        {
            // TODO: more robust handling; what if the entity has an instance of a class?
            py::object updateFunc = sc.PythonClass.attr(sc.Name.c_str()).attr("update");
            updateFunc(sc.PythonClass, delta);
        }
    }
}
