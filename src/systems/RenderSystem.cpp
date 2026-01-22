#include "systems/RenderSystem.h"
#include "controllers/Registry.h"
#include "components/WorldTransform.h"
#include "util/Logger.h"

#include <glad/glad.h>

static Registry *registry = &Registry::GetInstance();

template <>
void RenderSystem::RenderEntity<Lighting>(EntityID id, Camera *cam)
{
    Lighting lightComp = registry->GetComponent<Lighting>(id);
    RenderComponent &rc = registry->GetComponent<RenderComponent>(id);
    Transform &lightTrans = registry->GetComponent<Transform>(lightComp.LightID);
    WorldTransform &lightWorld = registry->GetComponent<WorldTransform>(lightComp.LightID);

    // Extract world position from world matrix
    glm::vec3 worldLightPos = glm::vec3(lightWorld.matrix[3]);

    rc.AddUniform("lightColor", glm::vec3(lightTrans.Color), UniformTypeMap::vec3);
    rc.AddUniform("lightPos", worldLightPos, UniformTypeMap::vec3);
    rc.AddUniform("viewPos", cam->transform.Pos, UniformTypeMap::vec3);
}

template <>
void RenderSystem::RenderEntity<RenderComponent>(EntityID id, Camera *cam)
{
    // Get world transform (computed by HierarchySystem)
    WorldTransform &wt = registry->GetComponent<WorldTransform>(id);
    Transform &t = registry->GetComponent<Transform>(id); // Only for Color
    RenderComponent rc = registry->GetComponent<RenderComponent>(id);

    // Color is NOT affected by hierarchy (as per design doc)
    rc.AddUniform("objectColor", t.Color, UniformTypeMap::vec4);
    rc.shader->Use();

    glm::vec3 cameraPos = cam->transform.Pos;
    glm::mat4 View = glm::lookAt(cameraPos, cameraPos + cam->front, cam->up);

    // Use pre-computed world matrix directly
    rc.AddUniform("model", wt.matrix, UniformTypeMap::mat4);
    rc.AddUniform("view", View, UniformTypeMap::mat4);
    rc.AddUniform("projection", cam->Projection, UniformTypeMap::mat4);

    rc.shader->SetUniforms(rc.uniforms);

    glBindVertexArray(rc.mesh->VAO);
    glDrawElements(GL_TRIANGLES, rc.mesh->indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void RenderSystem::Update(Camera *cam, float delta)
{
    // Log system info only once at startup
    static bool logged = false;
    if (!logged)
    {
        size_t lightingCount = registry->GetComponentSet<Lighting>().GetEntities().size();
        size_t renderCount = registry->GetComponentSet<RenderComponent>().GetEntities().size();
        LOG_INFO("RenderSystem initialized - {} Lighting entities, {} RenderComponent entities", lightingCount, renderCount);
        logged = true;
    }

    // Enable blending for transparency support
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Process lighting entities
    for (EntityID id : registry->GetComponentSet<Lighting>().GetEntities())
    {
        RenderEntity<Lighting>(id, cam);
    }

    // Process render entities
    const auto &renderEntities = registry->GetComponentSet<RenderComponent>().GetEntities();
    for (EntityID id : renderEntities)
    {
        RenderEntity<RenderComponent>(id, cam);
    }

    // Disable blending after rendering
    glDisable(GL_BLEND);
}
