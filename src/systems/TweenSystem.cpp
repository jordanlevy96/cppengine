#include "systems/TweenSystem.h"

static Registry *registry = &Registry::GetInstance();

void TweenSystem::Update(float delta)
{
    for (EntityID id : registry->GetComponentSet<Tween>())
    {
        Tween tween = registry->GetComponent<Tween>(id);
        if (tween.isActive)
        {
            UpdateTween(id, delta);
        }
    }
}

void TweenSystem::UpdateTween(EntityID id, float delta)
{
    Tween tween = registry->GetComponent<Tween>(id);
    tween.elapsed += delta;

    if (tween.elapsed >= tween.Duration)
    {
        tween.Func(id, tween.End);
        tween.isActive = false;
        return;
    }

    switch (tween.Type)
    {
    case (TransitionType::TRANS_LINEAR):
    {
        glm::vec3 newVal = tween.Start + (tween.End - tween.Start) * (tween.elapsed / tween.Duration);

        glm::vec3 lowerBound = glm::min(tween.Start, tween.End);
        glm::vec3 upperBound = glm::max(tween.Start, tween.End);
        glm::vec3 value = glm::clamp(newVal, lowerBound, upperBound);
        tween.Func(id, value);
        break;
    }
    case (TransitionType::TRANS_SINE):
    {
        glm::vec3 newVal = tween.Start + (tween.End - tween.Start) * (float)sin((tween.elapsed / tween.Duration) * (3.1415926535897 / 2));

        glm::vec3 lowerBound = glm::min(tween.Start, tween.End);
        glm::vec3 upperBound = glm::max(tween.Start, tween.End);
        glm::vec3 value = glm::clamp(newVal, lowerBound, upperBound);
        tween.Func(id, value);
        break;
    }
    }
}