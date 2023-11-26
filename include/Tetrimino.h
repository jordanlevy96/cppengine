#pragma once

#include "components/RenderComponent.h"

#include <glm/glm.hpp>
#include <vector>

enum TetriminoShape
{
    I,
    O,
    T,
    J,
    L,
    S,
    Z
};

class Tetrimino
{
public:
    Tetrimino(glm::mat4 matrix, std::shared_ptr<RenderComponent> rc);
    static std::unordered_map<TetriminoShape, glm::mat4> LoadTetriminos(const std::string &src);

private:
    unsigned int cubes[4];
};
