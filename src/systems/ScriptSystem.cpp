#include "systems/ScriptSystem.h"
#include "util/Logger.h"

static ScriptManager &sm = ScriptManager::GetInstance();
static Registry &registry = Registry::GetInstance();

void ScriptSystem::Update(float delta)
{
    for (EntityID id : registry.GetComponentSet<ScriptComponent>().GetEntities())
    {
        ScriptComponent &sc = registry.GetComponent<ScriptComponent>(id);

        if (sc.Type == ScriptType::Lua)
        {
            try
            {
                sol::function updateFunc = sc.LuaClass["process"];
                if (updateFunc.valid())
                {
                    updateFunc(sc.LuaClass, delta);
                }
            }
            catch (const sol::error &e)
            {
                LOG_ERROR("Lua script error in {}: {}", sc.Name, e.what());
            }
        }
#ifdef USE_PYTHON_SCRIPTING
        else if (sc.Type == ScriptType::Python)
        {
            // TODO: more robust handling; what if the entity has an instance of a class?
            py::object updateFunc = sc.PythonClass.attr(sc.Name.c_str()).attr("update");
            updateFunc(sc.PythonClass, delta);
        }
#endif
    }
}
