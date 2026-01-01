/**
 * @file TweenSystem.h
 * @brief ECS system for animating entity properties over time
 */

#pragma once

#include "controllers/Registry.h"

/**
 * @brief Static system for interpolating entity properties using tweens
 *
 * Processes entities with Tween components, smoothly animating values
 * from start to end over a specified duration using various easing functions.
 *
 * **Supported Transition Types:**
 * - TRANS_LINEAR: Constant velocity interpolation
 * - TRANS_SINE: Smooth ease-in/ease-out using sine curve
 *
 * **Usage Pattern:**
 * 1. Attach Tween component to entity with callback function
 * 2. Set isActive = true to start animation
 * 3. TweenSystem calls Func(id, value) each frame with interpolated value
 * 4. Automatically deactivates when elapsed >= Duration
 *
 * **Common Use Cases:**
 * - Smooth position/scale/rotation changes
 * - Camera transitions
 * - UI element animations
 * - Color fading
 *
 * @note All methods are static - this is a stateless system
 * @note Tweens update on main thread during game loop
 */
class TweenSystem
{
public:
    /**
     * @brief Update all active tweens, advancing animations by delta time
     * @param delta Time since last frame in seconds
     * @note Only updates tweens where isActive == true
     */
    static void Update(float delta);

private:
    /**
     * @brief Interpolate a single tween and invoke its callback
     * @param id Entity ID being animated
     * @param delta Time since last frame in seconds
     * @note Advances elapsed time, calculates interpolated value, calls Func()
     * @note Deactivates tween when elapsed >= Duration
     */
    static void UpdateTween(EntityID id, float delta);
};