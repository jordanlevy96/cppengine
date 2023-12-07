#include "systems/TweenSystem.h"

static Registry *registry = &Registry::GetInstance();

void TweenSystem::Update(float delta)
{
    for (std::pair<unsigned int, std::shared_ptr<Tween>> pair : registry->GetComponentMap<Tween>())
    {
        if (pair.second->isActive)
        {
            PRINT("Updating " << pair.first)
            UpdateTween(pair, delta);
        }
    }
}

void TweenSystem::UpdateTween(std::pair<unsigned int, std::shared_ptr<Tween>> pair, float delta)
{
    PRINT("Tweening")
    unsigned int id = pair.first;
    std::shared_ptr<Tween> tween = pair.second;
    tween->elapsed += delta;

    if (tween->elapsed >= tween->Duration)
    {
        tween->Func(id, tween->End);
        tween->isActive = false;
        PRINT("Finished!")
        return;
    }

    switch (tween->Type)
    {
    case (TransitionType::TRANS_LINEAR):
    {
        glm::vec3 newVal = tween->Start + (tween->End - tween->Start) * (tween->elapsed / tween->Duration);

        glm::vec3 lowerBound = glm::min(tween->Start, tween->End);
        glm::vec3 upperBound = glm::max(tween->Start, tween->End);
        glm::vec3 value = glm::clamp(newVal, lowerBound, upperBound);
        tween->Func(id, value);
        break;
    }
    case (TransitionType::TRANS_SINE):
    {
        glm::vec3 newVal = tween->Start + (tween->End - tween->Start) * (float)sin((tween->elapsed / tween->Duration) * (3.1415926535897 / 2));

        glm::vec3 lowerBound = glm::min(tween->Start, tween->End);
        glm::vec3 upperBound = glm::max(tween->Start, tween->End);
        glm::vec3 value = glm::clamp(newVal, lowerBound, upperBound);
        tween->Func(id, value);
        break;
    }
    }

    PRINT("Tweened!")
}