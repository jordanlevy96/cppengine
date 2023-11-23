#include "systems/RenderSystem.h"
#include "controllers/Registry.h"

#include <glad/glad.h>

static Registry *registry = &Registry::GetInstance();

void RenderSystem::RenderEntity(unsigned int id, Camera *cam)
{
    RenderComponent *rc;

    if (registry->RenderComponents.find(id) != registry->RenderComponents.end())
    {
        rc = registry->RenderComponents[id];
    }
    else if (registry->LightingComponents.find(id) != registry->LightingComponents.end())
    {
        rc = registry->LightingComponents[id];
    }
    else if (registry->EmitterComponents.find(id) != registry->EmitterComponents.end())
    {
        rc = registry->EmitterComponents[id];
    }

    Transform *t = registry->TransformComponents[id];

    rc->shader->Use();

    // Send Textures to GPU

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

    rc->AddUniform("view", View, UniformTypeMap::mat4);
    rc->AddUniform("projection", cam->Projection, UniformTypeMap::mat4);
    rc->SetUniforms();

    glBindVertexArray(rc->mesh->VAO);
    glDrawElements(GL_TRIANGLES, rc->mesh->indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // unbind VAO
    glUseProgram(0);      // unbind shader
}