#include <Camera.h>

#include <iostream>

void Camera::SetPerspective(float fovInDegrees, float width, float height)
{
    fov = fovInDegrees;
    Projection = glm::perspective(glm::radians(fov), width / height, 0.1f, 100.0f); // TODO: settings for near and far
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

    // Other Uniforms

    obj->shader->setMat4("model", obj->transform);

    // Camera Handling
    View = glm::lookAt(pos, pos + front, up);
    obj->shader->setMat4("view", View);
    obj->shader->setMat4("projection", Projection);

    // Render!

    glBindVertexArray(obj->mesh->VAO);
    glDrawElements(GL_TRIANGLES, obj->mesh->indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // unbind VAO
    glUseProgram(0);      // unbind shader
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

void Camera::RotateByMouse(double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}

void Camera::Move(CameraDirections dir, float deltaTime)
{
    float speed = 0.05f * deltaTime;
    switch (dir)
    {
    case (CameraDirections::FORWARD):
        pos += speed * front;
        break;
    case (CameraDirections::BACK):
        pos -= speed * front;
        break;
    case (CameraDirections::LEFT):
        pos -= glm::normalize(glm::cross(front, up)) * speed;
        break;
    case (CameraDirections::RIGHT):
        pos += glm::normalize(glm::cross(front, up)) * speed;
        break;
    default:
        std::cerr << "Invalid direction given to camera" << std::endl;
    }
}

void Camera::Translate(glm::vec3 translate)
{
    pos += translate;
    // View = glm::translate(View, translate);
}