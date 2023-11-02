#include <GameObject3D.h>

GameObject3D::GameObject3D(const char *shaderSrc, const char *modelSrc) : GameObject(shaderSrc, modelSrc) {}

void GameObject3D::Rotate(float degrees, EulerAngles dir)
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
    }

    transform = glm::rotate(transform, glm::radians(degrees), rotationAngle);
}