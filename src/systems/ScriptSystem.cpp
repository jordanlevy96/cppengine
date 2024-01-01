#include "systems/ScriptSystem.h"

static ScriptManager &sm = ScriptManager::GetInstance();
static Registry &registry = Registry::GetInstance();

void ScriptSystem::Update(float delta)
{
    for (EntityID id : registry.GetComponentSet<ScriptComponent>().GetEntities())
    {
        const char *update = "update";
        ScriptComponent sc = registry.GetComponent<ScriptComponent>(id);

        // TODO: more robust handling; what if the entity has an instance of a class?
        py::object updateFunc = sc.ScriptClass.attr(sc.Name.c_str()).attr(update);
        updateFunc(sc.ScriptClass, delta);
    }
}
