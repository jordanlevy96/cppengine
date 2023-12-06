#include "systems/ScriptSystem.h"

static ScriptManager &sm = ScriptManager::GetInstance();
static Registry &registry = Registry::GetInstance();

void ScriptSystem::Update(float delta)
{
    for (auto entity : registry.entities)
    {
        unsigned int id = entity.first;
        std::shared_ptr<ScriptComponent> sc = registry.GetComponent<ScriptComponent>(id);
        if (sc)
        {
            sc->ScriptClass["process"](sc->ScriptClass, delta);
        }
    }
}
