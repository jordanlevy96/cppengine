/**
 * @file WorldTransform.h
 * @brief Computed world-space transformation matrix component
 */

#pragma once

#include <glm/glm.hpp>

/**
 * @brief Component storing the computed world-space transformation matrix
 *
 * This component holds the final world-space matrix after hierarchy traversal.
 * It is computed by HierarchySystem each frame and consumed by RenderSystem.
 *
 * **Separation of Concerns:**
 * - Transform: Local-space data (relative to parent)
 * - WorldTransform: World-space result (computed by HierarchySystem)
 *
 * **Matrix Computation:**
 * For root entities: WorldMatrix = LocalMatrix
 * For child entities: WorldMatrix = ParentWorldMatrix * LocalMatrix
 *
 * @note Automatically attached to all entities via RegisterEntity()
 * @note Do NOT modify directly - updated by HierarchySystem each frame
 * @see Transform for local-space data
 * @see HierarchySystem for computation logic
 */
struct WorldTransform
{
    glm::mat4 matrix = glm::mat4(1.0f);  ///< Computed world transformation matrix
};
