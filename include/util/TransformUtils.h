/**
 * @file TransformUtils.h
 * @brief Transform manipulation utilities with hierarchy support
 */

#pragma once

#include "components/Transform.h"

#ifdef __linux__
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // For glm::toMat4

typedef size_t EntityID;

/**
 * @brief Utilities for transforming entities with automatic child propagation
 *
 * All transformation functions recursively apply to entity hierarchies.
 * Child transforms are modified to maintain relative positions/rotations.
 *
 * @note Functions require entities to have both Transform and HierarchyComponent
 * @note Uses Registry singleton internally to fetch components
 * @see HierarchyComponent for parent/child relationships
 *
 * Performance considerations:
 * - translate() and rotate() are recursive (O(n) for n children)
 * - Deep hierarchies may impact performance in tight loops
 *
 * Usage example:
 * @code
 * // Create parent-child relationship
 * EntityID parent = registry->CreateEntity();
 * EntityID child = registry->CreateEntity();
 * AddChild(parent, child);  // From SceneTraversal.h
 *
 * // Move parent (child moves with it)
 * TransformUtils::translate(parent, glm::vec3(5.0f, 0.0f, 0.0f));
 *
 * // Rotate parent around its position (child orbits)
 * TransformUtils::rotate(parent, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
 * @endcode
 */
namespace TransformUtils
{
    /**
     * @brief Convert Transform to 4x4 transformation matrix
     * @param transform Transform component with position, rotation, scale
     * @return Model matrix (translation * rotation * scale)
     * @note Matrix multiplication order: T * R * S (standard OpenGL convention)
     */
    glm::mat4 calculateMatrix(Transform transform);

    /**
     * @brief Translate entity and all children by offset
     * @param entity Entity ID to translate
     * @param translation Translation vector in world space
     * @note Recursively translates all children, maintaining relative positions
     * @note Modifies Transform.Pos for entity and all descendants
     */
    void translate(EntityID entity, const glm::vec3 &translation);

    /**
     * @brief Move entity to absolute position (translates by difference)
     * @param entity Entity ID to move
     * @param newPos Target position in world space
     * @note Internally calls translate() with (newPos - currentPos)
     * @note Children move with parent, maintaining relative offsets
     */
    void move_to(EntityID entity, const glm::vec3 &newPos);

    /**
     * @brief Rotate entity and children around entity's position
     * @param entity Entity ID to rotate
     * @param angle Rotation angle in degrees
     * @param axis Rotation axis (will be normalized)
     * @note Parent's position is pivot point - children orbit around it
     * @note Uses quaternion math to rotate child positions and orientations
     * @note Recursively applies to all descendants
     */
    void rotate(EntityID entity, float angle, const glm::vec3 &axis);
};