#include <Camera.h>

#include <iostream>

glm::mat4 Camera::process(double frametime)
{
    return glm::mat4(0.0f);
}

void Camera::SetPerspective(float degrees, float width, float height)
{
    Projection = glm::perspective(glm::radians(degrees), width / height, 0.1f, 100.0f); // TODO: settings for near and far
}

void Camera::RenderAll(std::vector<GameObject *> objects)
{
    for (GameObject *obj : objects)
    {
        Render(obj);
    }
}

void Camera::Render(GameObject *obj)
{
    obj->shader->Use();

    GLenum gl_textures[] = {GL_TEXTURE0,
                            GL_TEXTURE1};
    for (int i = 0; i < obj->textures.size(); i++)
    {
        glActiveTexture(gl_textures[i]);
        glBindTexture(GL_TEXTURE_2D, obj->textures[i]);
    }

    obj->shader->setInt("texture1", 0);
    obj->shader->setInt("texture2", 1);

    obj->shader->setMat4("model", obj->transform);
    obj->shader->setMat4("view", View);
    obj->shader->setMat4("projection", Projection);

    glBindVertexArray(obj->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glUseProgram(0);
}
