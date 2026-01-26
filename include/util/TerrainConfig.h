/**
 * @file TerrainConfig.h
 * @brief Terrain system configuration structure
 * @lines ~80
 *
 * Quick-stats:
 * - RingConfig - Per-ring LOD settings (line ~25)
 * - TerrainConfig - Full terrain configuration (line ~35)
 *
 * Loaded from YAML scene files via SceneLoader.
 */

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

/**
 * @brief Configuration for a single clipmap ring
 *
 * Defined in ClipmapGeometry.h, forward declared here for convenience.
 */
// struct RingConfig already defined in ClipmapGeometry.h

/**
 * @brief Complete terrain system configuration
 *
 * Loaded from YAML scene configuration. Defines tile formats,
 * clipmap LOD settings, caching parameters, and rendering options.
 */
struct TerrainConfig
{
    // Tile settings
    int tileSize = 512;                 ///< Tile dimension in pixels
    GLenum heightFormat = GL_R16;       ///< Height texture format (normalized 0..1)
    GLenum biomeFormat = GL_RGBA8;      ///< Biome texture format
    std::string compression = "none";   ///< "lz4", "zstd", or "none"

    // Clipmap ring configuration
    struct Ring
    {
        int resolution = 128;   ///< Grid resolution
        float texelSize = 1.0f; ///< World units per texel
    };
    std::vector<Ring> rings = {
        {128, 1.0f},
        {128, 2.0f},
        {128, 4.0f},
        {128, 8.0f}
    };

    // World settings
    int mapWidth = 4096;            ///< Number of tiles horizontally
    int mapHeight = 2048;           ///< Number of tiles vertically
    float heightScale = 100.0f;     ///< Height multiplier
    bool wrapHorizontal = true;     ///< Enable horizontal wrap

    // Cache settings
    int cpuCacheSize = 256;         ///< Max CPU tiles
    int gpuCacheSize = 64;          ///< Max GPU texture slots

    // Streaming settings
    size_t maxUploadBytesPerFrame = 10 * 1024 * 1024;  ///< 10 MB default

    // Data paths (relative to build directory)
    std::string heightTilesPath = "../res/terrain/height/";
    std::string biomeTilesPath = "../res/terrain/biome/";

    // Rendering
    float fogStart = 500.0f;
    float fogEnd = 2000.0f;
    glm::vec3 fogColor = glm::vec3(0.7f, 0.8f, 0.9f);
    glm::vec3 lightDir = glm::vec3(-0.5f, -1.0f, -0.3f);

    // Debug
    bool showWireframe = false;
    bool showClipRings = false;

    /**
     * @brief Validate configuration
     * @return True if configuration is valid
     */
    bool IsValid() const
    {
        return tileSize > 0 && mapWidth > 0 && mapHeight > 0 &&
               !rings.empty() && cpuCacheSize > 0 && gpuCacheSize > 0;
    }
};
