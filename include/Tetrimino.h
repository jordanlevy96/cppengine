#pragma once

#include "components/RenderComponent.h"
#include "components/CompositeEntity.h"

#include <glm/glm.hpp>
#include <vector>

namespace Tetrimino
{

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

    static std::unordered_map<std::string, TetriminoShape> TetriminoMap = {
        {"I", TetriminoShape::I},
        {"O", TetriminoShape::O},
        {"T", TetriminoShape::T},
        {"J", TetriminoShape::J},
        {"L", TetriminoShape::L},
        {"S", TetriminoShape::S},
        {"Z", TetriminoShape::Z},
    };

    extern std::shared_ptr<RenderComponent> cubeComp;

    unsigned int RegisterTetrimino(glm::mat4 matrix);
    std::unordered_map<TetriminoShape, glm::mat4> LoadTetriminos(const std::string &src);
    void Rotate(unsigned int id);
}