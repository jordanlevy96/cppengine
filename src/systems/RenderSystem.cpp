#include "systems/RenderSystem.h"
#include "controllers/Registry.h"
#include "util/TransformUtils.h"

#include <glad/glad.h>

static Registry *registry = &Registry::GetInstance();

void RenderSystem::Update(Camera *cam, float delta)
{
    for (std::pair<unsigned int, std::string> pair : registry->entities)
    {
        unsigned int id = pair.first;
        RenderSystem::RenderEntity(id, cam);
    }
}

void RenderSystem::RenderEntity(unsigned int id, Camera *cam)
{
    std::shared_ptr<Transform> t = registry->GetComponent<Transform>(id);
    std::shared_ptr<RenderComponent> rc = registry->GetComponent<RenderComponent>(id);

    if (!rc)
    {
        std::shared_ptr<Lighting> lightComp = registry->GetComponent<Lighting>(id);
        if (lightComp)
        {
            rc = lightComp->RC;

            rc->AddUniform("objectColor", *lightComp->ObjectColor, UniformTypeMap::vec3);
            rc->AddUniform("lightColor", lightComp->LightTrans->Color, UniformTypeMap::vec3);
            rc->AddUniform("lightPos", lightComp->LightTrans->Pos, UniformTypeMap::vec3);
            rc->AddUniform("viewPos", cam->transform.Pos, UniformTypeMap::vec3);
        }
        else
        {
            // No RenderComponent = nothing to render (children are rendered separately)
            return;
        }
    }

    rc->shader->Use();

    // TODO: set up texture/material component
    // GLenum gl_textures[] = {GL_TEXTURE0,
    //                         GL_TEXTURE1};
    // for (int i = 0; i < obj->model->textures.size(); i++)
    // {
    //     glActiveTexture(gl_textures[i]);
    //     glBindTexture(GL_TEXTURE_2D, obj->model->textures[i]);
    // }
    // obj->shader->setInt("texture1", 0);
    // obj->shader->setInt("texture2", 1);

    glm::vec3 cameraPos = cam->transform.Pos;
    glm::mat4 View = glm::mat4(1.0f);
    View = glm::lookAt(cameraPos, cameraPos + cam->front, cam->up);
    glm::mat4 Model = TransformUtils::calculateMatrix(t);

    rc->AddUniform("model", Model, UniformTypeMap::mat4);
    rc->AddUniform("view", View, UniformTypeMap::mat4);
    rc->AddUniform("projection", cam->Projection, UniformTypeMap::mat4);
    rc->shader->SetUniforms(rc->uniforms);

    glBindVertexArray(rc->mesh->VAO);
    glDrawElements(GL_TRIANGLES, rc->mesh->indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // unbind VAO
    glUseProgram(0);      // unbind shader
}
