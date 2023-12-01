#pragma once

#include <glm/glm.hpp>

#include <iostream>

#define DEBUG_PRINT std::cout << "!!! " << __FILE__ << ":" << __LINE__ << " !!!" << std::endl;
#define PRINT(x) std::cout << x << std::endl;

void PrintVec3(const glm::vec3 &vec)
{
    std::cout << vec.x << " " << vec.y << " " << vec.z << std::endl;
}

void PrintMat4(const glm::mat4 &matrix)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}
