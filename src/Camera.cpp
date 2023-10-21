#include <Camera.h>

Camera::Camera()
{
    w = a = s = d = 0;
    pos = glm::vec3(0, 0, -50);
    rot = glm::vec3(0, 0, 0);
}

glm::mat4 Camera::process(double frametime)
{
    double ftime = frametime;
    float speed = 0;
    if (w == 1)
    {
        speed = 10 * ftime;
    }
    else if (s == 1)
    {
        speed = -10 * ftime;
    }
    float yangle = 0;
    if (a == 1)
        yangle = -1 * ftime;
    else if (d == 1)
        yangle = 1 * ftime;
    rot.y += yangle;
    glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
    glm::vec4 dir = glm::vec4(0, 0, speed, 1);
    dir = dir * R;
    pos += glm::vec3(dir.x, dir.y, dir.z);
    glm::mat4 T = glm::translate(glm::mat4(1), pos);
    return R * T;
}
