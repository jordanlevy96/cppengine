#include <GameObject3D.h>

GameObject3D::GameObject3D(float x, float y, float width, float height, float *vertices, int numVertices, unsigned int *indices, int numIndices, char *shaderSrc) : GameObject(x, y, width, height, vertices, numVertices, indices, numIndices, shaderSrc) {}

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