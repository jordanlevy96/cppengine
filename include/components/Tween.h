/**
 * @file Tween.h
 * @brief Animation interpolation component for smooth value transitions
 */

#pragma once

#include <glm/glm.hpp>

#include <functional>

/**
 * @brief Easing function types for animation curves
 *
 * Defines how values interpolate between start and end.
 * More transition types planned (see TODO below).
 *
 * @note Based on Godot's Tween system
 * @see https://docs.godotengine.org/en/stable/classes/class_tween.html
 */
enum TransitionType
{
    TRANS_LINEAR,  ///< Constant speed interpolation (no easing)
    TRANS_SINE     ///< Sinusoidal easing (smooth acceleration/deceleration)
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
        TRANS_SPRING: Interpolation like a spring towards the end
        https://docs.godotengine.org/en/stable/classes/class_tween.html
    */
};

/**
 * @brief Component for animating vec3 values over time with easing
 *
 * Interpolates between Start and End values over Duration seconds using
 * a callback function. Managed by TweenSystem which updates elapsed time
 * and executes the callback each frame.
 *
 * **Common Use Cases:**
 * - Position animation (moving entities smoothly)
 * - Color transitions (fade in/out effects)
 * - Scale animations (grow/shrink effects)
 * - Camera movements
 *
 * **Usage Example:**
 * @code
 * // Animate entity position from (0,0,0) to (10,5,0) over 2 seconds
 * auto positionSetter = [](EntityID id, glm::vec3 pos) {
 *     auto& transform = Registry::GetInstance().GetComponent<Transform>(id);
 *     transform.Pos = pos;
 * };
 *
 * Tween moveTween(
 *     positionSetter,
 *     glm::vec3(0.0f, 0.0f, 0.0f),  // Start position
 *     glm::vec3(10.0f, 5.0f, 0.0f), // End position
 *     2.0f,                          // Duration in seconds
 *     TRANS_SINE                     // Smooth easing
 * );
 * moveTween.isActive = true;  // Start the animation
 * registry.RegisterComponent(entityId, moveTween);
 * @endcode
 *
 * **Lifecycle:**
 * 1. Create Tween with callback and parameters
 * 2. Set isActive = true to begin animation
 * 3. TweenSystem updates elapsed time each frame
 * 4. Callback is invoked with interpolated value
 * 5. When elapsed >= Duration, tween completes (isActive = false)
 *
 * @note TweenSystem must be running in the game loop
 * @see TweenSystem for update logic and easing implementations
 */
struct Tween
{
    std::function<void(unsigned int, glm::vec3)> Func;  ///< Callback invoked each frame with (entityId, interpolatedValue)
    glm::vec3 Start;                                     ///< Starting value (at elapsed = 0)
    glm::vec3 End;                                       ///< Target value (at elapsed = Duration)
    float Duration;                                      ///< Total animation time in seconds
    TransitionType Type;                                 ///< Easing curve type
    float elapsed = 0.0f;                                ///< Current time elapsed (updated by TweenSystem, default: 0)
    bool isActive = false;                               ///< Animation active flag (set to true to start, default: false)

    /**
     * @brief Construct animation tween
     * @param func Callback function(EntityID, glm::vec3) to apply interpolated value
     * @param start Initial value at t=0
     * @param end Target value at t=Duration
     * @param dur Animation duration in seconds
     * @param type Easing function type
     * @note Set isActive = true after construction to begin animation
     */
    Tween(std::function<void(unsigned int, glm::vec3)> func, const glm::vec3 &start, const glm::vec3 &end, float dur, TransitionType type)
        : Func(func), Start(start), End(end), Duration(dur), Type(type) {}
};