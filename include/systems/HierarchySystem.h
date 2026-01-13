/**
 * @file HierarchySystem.h
 * @brief ECS system for computing world-space transforms from hierarchy
 */

#pragma once

#include <glm/glm.hpp>

typedef size_t EntityID;

/**
 * @brief Static system for computing world-space transformation matrices
 *
 * Traverses the entity hierarchy and computes WorldTransform.matrix for each
 * entity using proper matrix multiplication: World = ParentWorld * Local.
 *
 * **System Execution Order:**
 * Must run AFTER systems that modify local transforms (ScriptSystem, TweenSystem)
 * and BEFORE systems that read world transforms (RenderSystem).
 *
 * **Traversal Strategy:**
 * Uses depth-first traversal from root entities (Parent == -1).
 * Parents are always processed before children, ensuring parent's WorldTransform
 * is computed before being used for child computation.
 *
 * @note All methods are static - this is a stateless system
 * @note Runs on main thread during game loop
 * @see WorldTransform for computed output
 * @see Transform for local input data
 */
class HierarchySystem
{
public:
    /**
     * @brief Update world transforms for all entities in hierarchy order
     *
     * Finds all root entities and recursively updates their subtrees.
     * After this call, all WorldTransform.matrix values are valid.
     *
     * @note Call once per frame, after TweenSystem, before RenderSystem
     */
    static void Update();

private:
    /**
     * @brief Recursively update entity and its children
     * @param entity Entity ID to update
     * @param parentWorld Parent's world matrix (identity for roots)
     *
     * Computes: WorldTransform = parentWorld * LocalMatrix
     * Then recursively processes all children with this entity's WorldTransform.
     */
    static void UpdateEntity(EntityID entity, const glm::mat4& parentWorld);
};
