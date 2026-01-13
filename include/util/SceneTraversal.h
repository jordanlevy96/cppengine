/**
 * @file SceneTraversal.h
 * @brief Scene graph hierarchy traversal utilities
 */

#pragma once

#include "controllers/Registry.h"

/**
 * @brief Establish parent-child relationship between entities
 *
 * Modifies HierarchyComponent for both parent and child:
 * - Adds child to parent's Children vector
 * - Sets child's Parent to parent ID
 *
 * @param parent Entity ID to become parent
 * @param child Entity ID to become child
 * @note Both entities must have HierarchyComponent
 * @note Does not check for circular dependencies (caller must ensure valid tree)
 * @note Does not remove child from previous parent (single parent only)
 * @see TransformUtils for hierarchy-aware transformations
 *
 * Usage example:
 * @code
 * EntityID ship = registry->CreateEntity();
 * EntityID turret = registry->CreateEntity();
 * EntityID barrel = registry->CreateEntity();
 *
 * AddChild(ship, turret);    // Turret follows ship
 * AddChild(turret, barrel);  // Barrel follows turret (and ship)
 *
 * // Moving ship moves entire hierarchy
 * TransformUtils::translate(ship, glm::vec3(10, 0, 0));
 * @endcode
 */
void AddChild(EntityID parent, EntityID child);

/**
 * @brief Get parent entity ID of a child
 * @param child Entity ID to query
 * @return Parent entity ID from HierarchyComponent
 * @note Returns whatever is stored in HierarchyComponent.Parent
 * @note No validation - caller must ensure child has HierarchyComponent
 */
EntityID GetParent(EntityID child);
