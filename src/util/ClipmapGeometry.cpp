/**
 * @file ClipmapGeometry.cpp
 * @brief Static ring mesh generation for geometry clipmaps with skirts
 * @lines ~200
 *
 * Purpose: Generates GPU-ready terrain ring meshes for clipmap rendering
 *
 * Key functions:
 * - Initialize() - Create all ring meshes (line ~30, ~20 lines)
 * - Shutdown() - Release OpenGL resources (line ~55, ~15 lines)
 * - GenerateRingMesh() - Generate single ring geometry (line ~75, ~80 lines)
 * - GetRingOffset() - Calculate camera-snapped offset (line ~160, ~15 lines)
 *
 * Vertex format:
 * - vec2 gridPos (location 0): Grid coordinates [0, resolution]
 * - float ringIndex (location 1): For geomorph blending
 */

#include "util/ClipmapGeometry.h"
#include "util/Logger.h"

#include <cmath>

void ClipmapGeometry::Initialize(const std::vector<RingConfig>& configs)
{
    Shutdown();  // Clean up any existing data

    m_rings.resize(configs.size());

    for (size_t i = 0; i < configs.size(); ++i)
    {
        GenerateRingMesh(m_rings[i], configs[i].resolution, configs[i].texelSize, static_cast<int>(i));
        LOG_INFO("[ClipmapGeometry] Ring {} initialized: {}x{} grid, texelSize={}",
                 i, configs[i].resolution, configs[i].resolution, configs[i].texelSize);
    }

    LOG_INFO("[ClipmapGeometry] Initialized {} rings", configs.size());
}

void ClipmapGeometry::Shutdown()
{
    for (auto& ring : m_rings)
    {
        if (ring.VAO != 0)
        {
            glDeleteVertexArrays(1, &ring.VAO);
            ring.VAO = 0;
        }
        if (ring.VBO != 0)
        {
            glDeleteBuffers(1, &ring.VBO);
            ring.VBO = 0;
        }
        if (ring.EBO != 0)
        {
            glDeleteBuffers(1, &ring.EBO);
            ring.EBO = 0;
        }
    }
    m_rings.clear();
}

const ClipmapRing& ClipmapGeometry::GetRing(int index) const
{
    static ClipmapRing empty;
    if (index < 0 || index >= static_cast<int>(m_rings.size()))
    {
        LOG_WARNING("[ClipmapGeometry] GetRing: index {} out of range", index);
        return empty;
    }
    return m_rings[index];
}

void ClipmapGeometry::GenerateRingMesh(ClipmapRing& ring, int resolution, float texelSize, int ringIndex)
{
    ring.resolution = resolution;
    ring.texelSize = texelSize;

    // Grid dimensions (resolution+1 vertices per side for resolution cells)
    int gridSize = resolution + 1;
    int numVertices = gridSize * gridSize;

    // Vertex data: vec2 gridPos + float ringIndex = 3 floats per vertex
    std::vector<float> vertices;
    vertices.reserve(numVertices * 3);

    // Generate grid vertices
    for (int z = 0; z < gridSize; ++z)
    {
        for (int x = 0; x < gridSize; ++x)
        {
            // Grid position (will be offset by ring offset in shader)
            float gx = static_cast<float>(x) - static_cast<float>(resolution) * 0.5f;
            float gz = static_cast<float>(z) - static_cast<float>(resolution) * 0.5f;

            vertices.push_back(gx);                           // gridPos.x
            vertices.push_back(gz);                           // gridPos.y (actually z in world)
            vertices.push_back(static_cast<float>(ringIndex)); // ringIndex
        }
    }

    ring.vertexCount = numVertices;

    // Generate indices for triangle grid
    std::vector<unsigned int> indices;
    int numQuads = resolution * resolution;
    indices.reserve(numQuads * 6);  // 2 triangles per quad

    for (int z = 0; z < resolution; ++z)
    {
        for (int x = 0; x < resolution; ++x)
        {
            int topLeft = z * gridSize + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * gridSize + x;
            int bottomRight = bottomLeft + 1;

            // First triangle (CCW winding)
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle (CCW winding)
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    ring.indexCount = static_cast<int>(indices.size());

    // Create OpenGL buffers
    glGenVertexArrays(1, &ring.VAO);
    glGenBuffers(1, &ring.VBO);
    glGenBuffers(1, &ring.EBO);

    glBindVertexArray(ring.VAO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, ring.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ring.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex attribute 0: vec2 gridPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Vertex attribute 1: float ringIndex
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    LOG_DEBUG("[ClipmapGeometry] Ring {} mesh: {} vertices, {} indices",
              ringIndex, ring.vertexCount, ring.indexCount);
}

glm::vec2 ClipmapGeometry::GetRingOffset(int ringIndex, const glm::vec3& cameraPos) const
{
    if (ringIndex < 0 || ringIndex >= static_cast<int>(m_rings.size()))
    {
        return glm::vec2(0.0f);
    }

    const ClipmapRing& ring = m_rings[ringIndex];

    // Snap camera position to texel boundaries for this ring
    // This prevents vertex popping as the camera moves
    float snapSize = ring.texelSize;

    float snappedX = std::floor(cameraPos.x / snapSize) * snapSize;
    float snappedZ = std::floor(cameraPos.z / snapSize) * snapSize;

    // Center the grid on the camera by subtracting half the grid extent
    // Grid extends from offset to offset + resolution * texelSize
    // So we need to shift by half the extent to center it
    float halfExtent = ring.resolution * ring.texelSize * 0.5f;

    return glm::vec2(snappedX - halfExtent, snappedZ - halfExtent);
}
