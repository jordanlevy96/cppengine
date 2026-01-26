/**
 * @file ClipmapGeometry.h
 * @brief Static ring mesh generation for geometry clipmaps with skirts
 * @lines ~120
 *
 * Quick-stats (Public API):
 * - Initialize() - Pre-generate N ring meshes (line ~65)
 * - Shutdown() - Release OpenGL resources (line ~70)
 * - GetRing() - Access ring by index (line ~75)
 * - GetRingCount() - Number of rings (line ~80)
 * - GetRingOffset() - Snapped offset for camera position (line ~85)
 *
 * Vertex layout:
 * - location 0: vec2 gridPos (local grid coordinates)
 * - location 1: float ringIndex (for geomorph blending)
 *
 * Implementation: See src/util/ClipmapGeometry.cpp
 */

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

/**
 * @brief Configuration for a single clipmap ring
 */
struct RingConfig
{
    int resolution;     ///< Grid resolution (e.g., 128, 256, 512, 1024)
    float texelSize;    ///< World units per texel
};

/**
 * @brief OpenGL resources for a single clipmap ring mesh
 *
 * Each ring is a grid mesh with skirts at the edges to hide
 * gaps between LOD levels. Rings snap to texel boundaries
 * based on camera position.
 */
struct ClipmapRing
{
    GLuint VAO = 0;         ///< Vertex Array Object
    GLuint VBO = 0;         ///< Vertex Buffer Object
    GLuint EBO = 0;         ///< Element Buffer Object (indices)
    int resolution = 0;     ///< Grid resolution (e.g., 128, 256, 512, 1024)
    int vertexCount = 0;    ///< Total vertex count including skirts
    int indexCount = 0;     ///< Total index count for triangles
    float texelSize = 1.0f; ///< World units per texel
};

/**
 * @brief Geometry clipmap mesh generator
 *
 * Pre-generates static ring meshes for terrain rendering using
 * geometry clipmaps. Each ring has a fixed vertex grid with skirts
 * at the edges to prevent gaps between LOD levels.
 *
 * **Usage:**
 * @code
 * ClipmapGeometry clipmap;
 * std::vector<RingConfig> configs = {
 *     {128, 1.0f},   // Ring 0: 128x128 grid, 1m per texel
 *     {128, 2.0f},   // Ring 1: 128x128 grid, 2m per texel
 *     {128, 4.0f},   // Ring 2: 128x128 grid, 4m per texel
 * };
 * clipmap.Initialize(configs);
 *
 * // Each frame
 * for (int i = 0; i < clipmap.GetRingCount(); ++i) {
 *     glm::vec2 offset = clipmap.GetRingOffset(i, cameraPos);
 *     // Set uniforms and draw ring...
 * }
 * @endcode
 *
 * **Vertex Format:**
 * - vec2 gridPos (location 0): Local grid coordinates [0, resolution]
 * - float ringIndex (location 1): Ring index for geomorph blending
 *
 * @note Rings snap to texel boundaries to prevent popping during movement
 * @note Skirts extend downward at edges to hide LOD seams
 */
class ClipmapGeometry
{
public:
    ClipmapGeometry() = default;
    ~ClipmapGeometry() { Shutdown(); }

    /**
     * @brief Pre-generate ring meshes based on configurations
     * @param configs Vector of ring configurations (resolution + texel size)
     * @note Creates VAO/VBO/EBO for each ring
     * @note Must be called with active OpenGL context
     */
    void Initialize(const std::vector<RingConfig>& configs);

    /**
     * @brief Release all OpenGL resources
     * @note Safe to call multiple times
     */
    void Shutdown();

    /**
     * @brief Get ring mesh by index
     * @param index Ring index (0 = innermost/highest detail)
     * @return Reference to ClipmapRing with OpenGL handles
     */
    const ClipmapRing& GetRing(int index) const;

    /**
     * @brief Get total number of rings
     * @return Ring count
     */
    int GetRingCount() const { return static_cast<int>(m_rings.size()); }

    /**
     * @brief Calculate snapped offset for a ring given camera position
     * @param ringIndex Ring index to calculate offset for
     * @param cameraPos Camera world position
     * @return 2D offset snapped to texel boundaries
     * @note Snapping prevents vertex popping during camera movement
     */
    glm::vec2 GetRingOffset(int ringIndex, const glm::vec3& cameraPos) const;

private:
    std::vector<ClipmapRing> m_rings;   ///< Generated ring meshes

    /**
     * @brief Generate mesh geometry for a single ring
     * @param ring Ring to populate with geometry
     * @param resolution Grid resolution
     * @param texelSize World units per texel
     * @param ringIndex Index of this ring (for vertex attribute)
     */
    void GenerateRingMesh(ClipmapRing& ring, int resolution, float texelSize, int ringIndex);
};
