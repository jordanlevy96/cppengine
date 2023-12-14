#include "systems/RenderSystem.h"
#include "controllers/Registry.h"
#include "util/TransformUtils.h"

#include <glad/glad.h>

static Registry *registry = &Registry::GetInstance();

template <>
void RenderSystem::RenderEntity<Lighting>(EntityID id, Camera *cam)
{
    Lighting lightComp = registry->GetComponent<Lighting>(id);
    RenderComponent rc = registry->GetComponent<RenderComponent>(id);

    rc.AddUniform("lightColor", lightComp.LightTrans->Color, UniformTypeMap::vec3);
    rc.AddUniform("lightPos", lightComp.LightTrans->Pos, UniformTypeMap::vec3);
    rc.AddUniform("viewPos", cam->transform.Pos, UniformTypeMap::vec3);
}

template <>
void RenderSystem::RenderEntity<RenderComponent>(EntityID id, Camera *cam)
{
    Transform t = registry->GetComponent<Transform>(id);
    RenderComponent rc = registry->GetComponent<RenderComponent>(id);

    // Basic.shader
    rc.AddUniform("objectColor", t.Color, UniformTypeMap::vec3);

    rc.shader->Use();

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

    for (EntityID id : registry->GetComponentSet<Lighting>().GetEntities())
    {
        RenderEntity<Lighting>(id, cam);
    }

    for (EntityID id : registry->GetComponentSet<RenderComponent>().GetEntities())
    {
        RenderEntity<RenderComponent>(id, cam);
    }
}
