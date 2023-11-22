#include "systems/RenderSystem.h"

#include <glad/glad.h>

// void RenderSystem::RenderEntity(unsigned int id, Camera cam)
// {
//     // RenderComponent rc =

//     rc.shader->Use();

//     // Send Textures to GPU

//     // GLenum gl_textures[] = {GL_TEXTURE0,
//     //                         GL_TEXTURE1};
//     // for (int i = 0; i < obj->model->textures.size(); i++)
//     // {
//     //     glActiveTexture(gl_textures[i]);
//     //     glBindTexture(GL_TEXTURE_2D, obj->model->textures[i]);
//     // }

//     // obj->shader->setInt("texture1", 0);
//     // obj->shader->setInt("texture2", 1);

//     glm::mat4 View = glm::mat4(1.0f);
//     View = glm::lookAt(cam.transform.pos, cam.transform.pos + cam.front, cam.up);
//     rc.shader->AddUniform("view", View, UniformTypeMap::mat4);
//     rc.shader->AddUniform("projection", cam.Projection, UniformTypeMap::mat4);

//     glBindVertexArray(rc.mesh->VAO);
//     glDrawElements(GL_TRIANGLES, rc.mesh->indices.size(), GL_UNSIGNED_INT, 0);

//     glBindVertexArray(0); // unbind VAO
//     glUseProgram(0);      // unbind shader
// }