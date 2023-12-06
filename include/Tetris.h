#pragma once

#include "components/RenderComponent.h"
#include "components/CompositeEntity.h"

#include <glm/glm.hpp>
#include <vector>

namespace Tetris
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

    struct Tetrimino
    {
        glm::mat4 shape;
        glm::vec3 color;
    };

    struct TetriminoComponent : CompositeEntity
    {
        glm::mat4 cubeIDMap;
    };

    extern std::unordered_map<char, Tetrimino> TetriminoMap;

    unsigned int CreateTetrimino(std::shared_ptr<RenderComponent> rc, const std::string &in);
    unsigned int RegisterTetrimino(std::shared_ptr<RenderComponent> rc, Tetrimino tetriminoData);
    glm::mat4 GetChildMap(unsigned int parentID);
    void LoadTetriminos(const std::string &src);
}