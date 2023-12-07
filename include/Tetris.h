#pragma once

#include "components/RenderComponent.h"
#include "components/CompositeEntity.h"

#include "util/debug.h"

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

    enum Rotations
    {
        CW,
        CCW
    };

    static std::unordered_map<Rotations, float> RotationMap = {
        {Rotations::CW, 90.0f},
        {Rotations::CCW, -90.0f}};

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

    void RotateTetrimino(unsigned int id, Rotations rotation);
    void MoveTetrimino(unsigned int id, glm::vec2 dir);
    void TweenTetrimino(unsigned int id, glm::vec3 dir, float duration);

    void LoadTetriminos(const std::string &src);
}