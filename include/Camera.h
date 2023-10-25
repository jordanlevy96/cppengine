#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera(glm::vec3 start)
    {
        w = a = s = d = 0;
        pos = start;
        rot = glm::vec3(0, 0, 0);
        target = glm::vec3(0, 0, 0);
        dir = glm::normalize(pos - target);
    };
    glm::vec3 pos, rot, dir, target;
    int w, a, s, d;
    glm::mat4 process(double frametime);
    glm::mat4 V, M, P; // View, Model and Perspective matrix
};
