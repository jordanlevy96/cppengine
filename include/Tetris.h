#pragma once

#include "controllers/Registry.h"
#include "components/RenderComponent.h"

#include "util/debug.h"

#include <glm/glm.hpp>
#include <vector>

typedef size_t EntityID;

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

    // a wee bit hacky way to avoid registering an additional component for this matrix
    // of which there will only ever be one of at a time
    static glm::mat4 ActiveTetriminoChildMap = glm::mat4(-1.0f);

    extern std::unordered_map<char, Tetrimino> TetriminoMap;

    EntityID CreateTetrimino(RenderComponent rc, const std::string &in);
    EntityID RegisterTetrimino(RenderComponent rc, Tetrimino &tetriminoData);
    glm::mat4 GetTetriminoChildMap();

    void RotateTetrimino(EntityID id, Rotations rotation);
    void MoveTetrimino(EntityID id, glm::vec2 dir);
    void TweenTetrimino(EntityID id, glm::vec3 dir, float duration);

    void LoadTetriminos(const std::string &src);
}