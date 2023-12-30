#include "systems/ScriptSystem.h"

static ScriptManager &sm = ScriptManager::GetInstance();
static Registry &registry = Registry::GetInstance();

void ScriptSystem::Update(float delta)
{
    for (EntityID id : registry.GetComponentSet<ScriptComponent>().GetEntities())
    {
        ScriptComponent sc = registry.GetComponent<ScriptComponent>(id);
        PRINT("Running " << id << "'s update script");
        sc.ScriptClass["process"](sc.ScriptClass, delta);
    }
}
