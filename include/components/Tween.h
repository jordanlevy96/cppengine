#pragma once

#include <glm/glm.hpp>

#include <functional>

enum TransitionType
{
    TRANS_LINEAR,
    TRANS_SINE
    // TODO: implement more:
    /*
        TRANS_QUINT: Interpolation with a quintic (to the power of 5) function.
        TRANS_QUART: Interpolation with a quartic (to the power of 4) function.
        TRANS_QUAD: Interpolation with a quadratic (to the power of 2) function.
        TRANS_EXPO: Interpolation with an exponential (to the power of x) function.
        TRANS_ELASTIC: Interpolation with elasticity, wiggling around the edges.
        TRANS_CUBIC: Interpolation with a cubic (to the power of 3) function.
        TRANS_CIRC: Interpolation with a function using square roots.
        TRANS_BOUNCE: Interpolation by bouncing at the end.
        TRANS_BACK: Interpolation backing out at ends.
        TRANS_SPRING: Interpolation like a spring towards the end​
​        https://docs.godotengine.org/en/stable/classes/class_tween.html
    */
};

struct Tween
{
    std::function<void(unsigned int, glm::vec3)> Func;
    glm::vec3 Start;
    glm::vec3 End;
    float Duration;
    TransitionType Type;
    float elapsed = 0.0f;
    bool isActive = false;

    Tween(std::function<void(unsigned int, glm::vec3)> func, const glm::vec3 &start, const glm::vec3 &end, float dur, TransitionType type)
        : Func(func), Start(start), End(end), Duration(dur), Type(type) {}
};