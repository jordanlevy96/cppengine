#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    glm::vec3 pos, rot;
    int w, a, s, d;
    glm::mat4 process(double frametime);
};
