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

    // GLenum gl_textures[] = {GL_TEXTURE0,
    //                         GL_TEXTURE1};
    // for (int i = 0; i < obj->model->textures.size(); i++)
    // {
    //     glActiveTexture(gl_textures[i]);
    //     glBindTexture(GL_TEXTURE_2D, obj->model->textures[i]);
    // }

    // obj->shader->setInt("texture1", 0);
    // obj->shader->setInt("texture2", 1);

    obj->shader->setMat4("model", obj->transform);
    obj->shader->setMat4("view", View);
    obj->shader->setMat4("projection", Projection);

    glBindVertexArray(obj->model->VAO);
    glDrawArrays(GL_TRIANGLES, 0, obj->model->vertices.size());

    glUseProgram(0);
}

static glm::vec3 rotationAngle(EulerAngles dir)
{
    glm::vec3 rotationAngle;

    switch (dir)
    {
    case EulerAngles::ROLL:
        rotationAngle = glm::vec3(1.0f, 0.0f, 0.0f);
        break;

    case EulerAngles::YAW:
        rotationAngle = glm::vec3(0.0f, 1.0f, 0.0f);
        break;

    case EulerAngles::PITCH:
        rotationAngle = glm::vec3(0.0f, 0.0f, 1.0f);
        break;
    default:
        rotationAngle = glm::vec3(0.0f); // don't rotate if EulerAngle is invalid
    }

    return rotationAngle;
}

void Camera::Translate(glm::vec3 translate)
{
    View = glm::translate(View, translate);
}