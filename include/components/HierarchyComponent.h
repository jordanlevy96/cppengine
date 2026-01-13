/**
 * @file HierarchyComponent.h
 * @brief Parent-child entity relationship component for scene graphs
 */

#pragma once

#include <vector>

typedef size_t EntityID;

/**
 * @brief Component defining parent-child relationships between entities
 *
 * Establishes a scene graph hierarchy where children inherit transformations
 * from their parent. Used for complex objects made of multiple entities
 * (e.g., a car with wheels, a character with equipment).
 *
 * **Transformation Inheritance:**
 * - Child position is relative to parent position
 * - Child rotation is compounded with parent rotation
 * - Child scale is multiplied by parent scale
 * - When parent moves/rotates, children follow automatically
 *
 * **Scene Graph Example:**
 * @code
 * EntityID car = registry.RegisterEntity("car");
 * EntityID wheel1 = registry.RegisterEntity("wheel1", car);  // Parent = car
 * EntityID wheel2 = registry.RegisterEntity("wheel2", car);  // Parent = car
 *
 * // Car's HierarchyComponent.Children = {wheel1, wheel2}
 * // Wheel1's HierarchyComponent.Parent = car
 * @endcode
 *
 * **Usage Notes:**
 * - Parent ID of -1 (size_t max) indicates root entity (no parent)
 * - Children vector is automatically updated by Registry
 * - Destroying a parent does NOT auto-destroy children (manual cleanup needed)
 * - Transform calculations are done by RenderSystem during scene traversal
 *
 * **Common Patterns:**
 * @code
 * // Check if entity has parent
 * if (registry.HasComponent<HierarchyComponent>(entityId)) {
 *     auto& hierarchy = registry.GetComponent<HierarchyComponent>(entityId);
 *     if (hierarchy.Parent != static_cast<EntityID>(-1)) {
 *         // Has parent
 *     }
 * }
 *
 * // Iterate children
 * for (EntityID child : hierarchy.Children) {
 *     // Process child entity
 * }
 * @endcode
 *
 * @note Only entities that are part of a hierarchy need this component
 * @see Transform for spatial data
 * @see Registry::RegisterEntity() for parent parameter
 */
struct HierarchyComponent
{
    EntityID Parent;                 ///< Parent entity ID (-1 if root entity)
    std::vector<EntityID> Children;  ///< Child entity IDs (empty if leaf node)

    /**
     * @brief Construct hierarchy component with parent
     * @param parent Parent entity ID (use -1 for root entities)
     * @note Children vector starts empty and is populated by Registry
     */
    HierarchyComponent(EntityID parent) : Parent(parent) {}
};
