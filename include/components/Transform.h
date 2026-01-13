/**
 * @file Transform.h
 * @brief Spatial transformation component for entity position, rotation, scale, and color
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

typedef size_t EntityID;

/**
 * @brief Euler angle axis constants for rotations
 *
 * Provides standard basis vectors for pitch/roll/yaw rotations.
 * Use with glm::angleAxis() for quaternion rotations.
 *
 * @code
 * glm::quat rotation = glm::angleAxis(glm::radians(45.0f), EulerAngles::Yaw);
 * @endcode
 */
namespace EulerAngles
{
    const glm::vec3 Pitch(0.0f, 0.0f, 1.0f); ///< Z-axis rotation (nodding)
    const glm::vec3 Roll(1.0f, 0.0f, 0.0f);  ///< X-axis rotation (barrel roll)
    const glm::vec3 Yaw(0.0f, 1.0f, 0.0f);   ///< Y-axis rotation (turning)
};

/**
 * @brief Core spatial transformation component for entities
 *
 * Represents an entity's position, orientation, scale, and base color in 3D space.
 * This is the fundamental component automatically attached to all entities.
 *
 * **Coordinate System:** Right-handed, Y-up
 * - X-axis: Right
 * - Y-axis: Up
 * - Z-axis: Forward (out of screen)
 *
 * **Default Values:**
 * - Position: (0, 0, 0) - World origin
 * - Scale: (1, 1, 1) - Original size
 * - Rotation: Identity quaternion (no rotation)
 * - Color: (1, 1, 1) - White (RGB normalized 0-1)
 *
 * **Usage:**
 * @code
 * Transform t;
 * t.Pos = glm::vec3(10.0f, 5.0f, 0.0f);
 * t.Scale = glm::vec3(2.0f);  // Uniform scale
 * t.Rotation = glm::angleAxis(glm::radians(90.0f), EulerAngles::Yaw);
 * t.Color = glm::vec3(1.0f, 0.0f, 0.0f);  // Red
 * @endcode
 *
 * @note All entities automatically receive a Transform on creation
 * @see HierarchyComponent for parent/child transformation hierarchies
 */
struct Transform
{
    glm::vec3 Pos = glm::vec3(0.0f);                        ///< Local position relative to parent (x, y, z)
    glm::vec3 Scale = glm::vec3(1.0f);                      ///< Local scale (x, y, z) - 1.0 = original size
    glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); ///< Orientation quaternion (w, x, y, z) - identity = no rotation
    glm::vec3 Color = glm::vec3(1.0f);                      ///< Base tint color (r, g, b) normalized [0-1] - white = no tint
};