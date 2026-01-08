#include "systems/RenderSystem.h"
#include "controllers/Registry.h"
#include "util/TransformUtils.h"
#include "util/Logger.h"

#include <glad/glad.h>

static Registry *registry = &Registry::GetInstance();

template <>
void RenderSystem::RenderEntity<Lighting>(EntityID id, Camera *cam)
{
    Lighting lightComp = registry->GetComponent<Lighting>(id);
    RenderComponent &rc = registry->GetComponent<RenderComponent>(id);
    Transform lightTrans = registry->GetComponent<Transform>(lightComp.LightID);

    rc.AddUniform("lightColor", lightTrans.Color, UniformTypeMap::vec3);
    rc.AddUniform("lightPos", lightTrans.Pos, UniformTypeMap::vec3);
    rc.AddUniform("viewPos", cam->transform.Pos, UniformTypeMap::vec3);
}

template <>
void RenderSystem::RenderEntity<RenderComponent>(EntityID id, Camera *cam)
{
    Transform t = registry->GetComponent<Transform>(id);
    RenderComponent rc = registry->GetComponent<RenderComponent>(id);
    HierarchyComponent hc = registry->GetComponent<HierarchyComponent>(id);

    // Log first render only (one-time debug info)
    static bool logged = false;
    if (!logged)
    {
        std::string entityName = registry->GetEntityName(id);
        LOG_INFO("First render - entity {} at pos({}, {}, {}), scale({}, {}, {})",
                 entityName, t.Pos.x, t.Pos.y, t.Pos.z, t.Scale.x, t.Scale.y, t.Scale.z);
        if (rc.mesh)
        {
            LOG_INFO("  VAO: {}, mesh indices: {}", rc.mesh->VAO, rc.mesh->indices.size());
        }
        logged = true;
    }

    // Null check for shader and mesh
    if (!rc.shader)
    {
        LOG_ERROR("Entity {} has null shader, skipping render", registry->GetEntityName(id));
        return;
    }
    if (!rc.mesh)
    {
        LOG_ERROR("Entity {} has null mesh, skipping render", registry->GetEntityName(id));
        return;
    }

    if (hc.Parent < std::numeric_limits<size_t>::max())
    {
        Transform parentTransform = registry->GetComponent<Transform>(hc.Parent);
        t.Pos += parentTransform.Pos;
        t.Color *= parentTransform.Color;
        t.Scale *= parentTransform.Scale;
    }

    rc.AddUniform("objectColor", t.Color, UniformTypeMap::vec3);
    rc.shader->Use();

    // Camera null check (critical, but only once)
    if (!cam)
    {
        LOG_CRITICAL("Camera pointer is NULL! Skipping render.");
        LOG_CRITICAL("EXITING");
        std::exit(EXIT_FAILURE);
        return;
    }

    glm::vec3 cameraPos = cam->transform.Pos;

    glm::mat4 View = glm::mat4(1.0f);
    View = glm::lookAt(cameraPos, cameraPos + cam->front, cam->up);
    glm::mat4 Model = TransformUtils::calculateMatrix(t);

    rc.AddUniform("model", Model, UniformTypeMap::mat4);
    rc.AddUniform("view", View, UniformTypeMap::mat4);
    rc.AddUniform("projection", cam->Projection, UniformTypeMap::mat4);

    rc.shader->SetUniforms(rc.uniforms);

    glBindVertexArray(rc.mesh->VAO);

    glDrawElements(GL_TRIANGLES, rc.mesh->indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // unbind VAO
    glUseProgram(0);      // unbind shader
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
}
