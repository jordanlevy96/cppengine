/**
 * @file TileCache.h
 * @brief LRU caches for terrain tiles (both CPU and GPU)
 * @lines ~200
 *
 * Quick-stats (Public API):
 * - TileCoord - Tile coordinate struct with hash support (line ~30)
 * - CPUTile - CPU-side tile data container (line ~55)
 * - CPUTileCache - LRU cache for CPU tile data (line ~100)
 * - GPUTileCache - LRU cache for GPU texture array slots (line ~150)
 *
 * Usage:
 * - CPU cache stores decoded tile data ready for upload
 * - GPU cache manages GL_TEXTURE_2D_ARRAY slots
 * - Both use LRU eviction when capacity is reached
 *
 * Thread Safety:
 * - Not thread-safe by default
 * - Caller must synchronize access if used across threads
 *
 * Implementation: See src/util/TileCache.cpp
 */

#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

/**
 * @brief Tile coordinate identifier
 *
 * Uniquely identifies a tile by its position and layer type.
 */
struct TileCoord
{
    int x;     ///< Tile X coordinate in grid
    int y;     ///< Tile Y coordinate in grid
    int layer; ///< Layer type: 0 = height, 1 = biome

    /**
     * @brief Equality comparison
     * @param other Tile coordinate to compare
     * @return True if coordinates match
     */
    bool operator==(const TileCoord &other) const;

    /**
     * @brief Less-than comparison for std::set
     * @param other Tile coordinate to compare
     * @return True if this coordinate is less than other
     */
    bool operator<(const TileCoord &other) const;
};

/**
 * @brief Hash functor for TileCoord
 *
 * Enables use of TileCoord as key in unordered_map.
 */
struct TileCoordHash
{
    /**
     * @brief Compute hash for tile coordinate
     * @param coord Tile coordinate
     * @return Hash value
     */
    size_t operator()(const TileCoord &coord) const;
};

/**
 * @brief CPU-side tile data container
 *
 * Stores decoded tile data ready for GPU upload.
 * Height data uses R16F format, biome data uses RGBA8.
 */
struct CPUTile
{
    TileCoord coord;                  ///< Tile coordinate identifier
    std::vector<uint16_t> heightData; ///< R16F decoded (tileSize * tileSize)
    std::vector<uint8_t> biomeData;   ///< RGBA8 (tileSize * tileSize * 4)
    uint64_t lastAccessFrame = 0;     ///< Frame number of last access
    bool dirty = false;               ///< True if modified since last upload

    /**
     * @brief Set height value at local tile position
     * @param localX X coordinate within tile (0 to tileSize-1)
     * @param localY Y coordinate within tile (0 to tileSize-1)
     * @param value Height value (R16F encoded)
     * @param tileSize Tile dimension (assumed square)
     * @note Marks tile as dirty
     */
    void SetHeight(int localX, int localY, uint16_t value, int tileSize);

    /**
     * @brief Get height value at local tile position
     * @param localX X coordinate within tile (0 to tileSize-1)
     * @param localY Y coordinate within tile (0 to tileSize-1)
     * @param tileSize Tile dimension (assumed square)
     * @return Height value (R16F encoded)
     */
    uint16_t GetHeight(int localX, int localY, int tileSize) const;
};

/**
 * @brief LRU cache for CPU tile data
 *
 * Stores decoded tile data in memory. Uses least-recently-used
 * eviction policy when maximum capacity is reached.
 *
 * Thread Safety: Not thread-safe. Caller must synchronize.
 */
class CPUTileCache
{
public:
    /**
     * @brief Initialize cache with capacity
     * @param maxTiles Maximum number of tiles to cache
     * @param tileSize Tile dimension in pixels (assumed square)
     */
    void Initialize(int maxTiles, int tileSize);

    /**
     * @brief Release all cached tiles
     */
    void Shutdown();

    /**
     * @brief Get tile from cache
     * @param coord Tile coordinate
     * @return Pointer to cached tile, or nullptr if not cached
     * @note Updates LRU order on hit
     */
    CPUTile *Get(const TileCoord &coord);

    /**
     * @brief Add tile to cache
     * @param coord Tile coordinate
     * @param tile Tile data (takes ownership)
     * @note May evict LRU tile if cache is full
     */
    void Put(const TileCoord &coord, std::unique_ptr<CPUTile> tile);

    /**
     * @brief Evict least recently used tile
     * @return True if a tile was evicted, false if cache empty
     */
    bool EvictLRU();

    /**
     * @brief Get current number of cached tiles
     * @return Tile count
     */
    int GetTileCount() const;

    /**
     * @brief Get cache hit count since initialization
     * @return Hit count
     */
    int GetCacheHits() const;

    /**
     * @brief Get cache miss count since initialization
     * @return Miss count
     */
    int GetCacheMisses() const;

    /**
     * @brief Get tile size
     * @return Tile dimension in pixels
     */
    int GetTileSize() const { return m_tileSize; }

    /**
     * @brief Advance frame counter
     * @note Call once per frame to update access timestamps
     */
    void AdvanceFrame();

private:
    std::unordered_map<TileCoord, std::unique_ptr<CPUTile>, TileCoordHash> m_tiles;
    std::list<TileCoord> m_lruOrder; ///< Front = LRU, Back = MRU
    int m_maxTiles = 0;              ///< Maximum cache capacity
    int m_tileSize = 0;              ///< Tile dimension in pixels
    uint64_t m_currentFrame = 0;     ///< Current frame number
    int m_hits = 0;                  ///< Cache hit counter
    int m_misses = 0;                ///< Cache miss counter

    /**
     * @brief Update LRU order for accessed tile
     * @param coord Tile coordinate that was accessed
     */
    void UpdateLRU(const TileCoord &coord);
};

/**
 * @brief LRU cache for GPU texture array slots
 *
 * Manages slots in GL_TEXTURE_2D_ARRAY textures for efficient
 * terrain tile storage. Uses least-recently-used eviction.
 *
 * Thread Safety: Not thread-safe. Must be called from main thread
 * with active OpenGL context.
 */
class GPUTileCache
{
public:
    /**
     * @brief Initialize GPU cache and create texture arrays
     * @param maxSlots Maximum number of tile slots
     * @param tileSize Tile dimension in pixels (assumed square)
     * @param mapWidthTiles Map width in tiles
     * @param mapHeightTiles Map height in tiles
     * @param heightFormat OpenGL format for height data (e.g., GL_R16F)
     * @param biomeFormat OpenGL format for biome data (e.g., GL_RGBA8)
     * @note Must be called from main thread with active GL context
     */
    void Initialize(int maxSlots, int tileSize, int mapWidthTiles, int mapHeightTiles, GLenum heightFormat, GLenum biomeFormat);

    /**
     * @brief Release GPU resources
     * @note Must be called from main thread with active GL context
     */
    void Shutdown();

    /**
     * @brief Get texture array slot for tile
     * @param coord Tile coordinate
     * @return Slot index (0 to maxSlots-1), or -1 if not cached
     * @note Updates LRU order on hit
     */
    int GetSlot(const TileCoord &coord);

    /**
     * @brief Allocate slot for tile
     * @param coord Tile coordinate
     * @return Slot index, evicting LRU if necessary
     */
    int AllocateSlot(const TileCoord &coord);

    /**
     * @brief Upload tile data to GPU slot
     * @param slot Slot index (from GetSlot or AllocateSlot)
     * @param tile CPU tile data to upload
     * @note Must be called from main thread with active GL context
     */
    void Upload(int slot, const CPUTile *tile);

    /**
     * @brief Evict least recently used slot
     * @return Evicted slot index, or -1 if cache empty
     */
    int EvictLRU();

    /**
     * @brief Get height texture array handle
     * @return OpenGL texture ID (GL_TEXTURE_2D_ARRAY)
     */
    GLuint GetHeightTextureArray() const;

    /**
     * @brief Get biome texture array handle
     * @return OpenGL texture ID (GL_TEXTURE_2D_ARRAY)
     */
    GLuint GetBiomeTextureArray() const;

    /**
     * @brief Get tile-to-slot lookup texture for height tiles
     * @return OpenGL texture ID (GL_TEXTURE_2D, integer texture storing slot indices)
     */
    GLuint GetHeightSlotMapTexture() const;

    /**
     * @brief Get current number of used slots
     * @return Used slot count
     */
    int GetUsedSlotCount() const;

private:
    GLuint m_heightArray = 0; ///< Height texture array (GL_TEXTURE_2D_ARRAY)
    GLuint m_biomeArray = 0;  ///< Biome texture array (GL_TEXTURE_2D_ARRAY)
    GLuint m_heightSlotMap = 0; ///< TileCoord->slot lookup (GL_TEXTURE_2D, R16I)

    std::vector<int> m_freeSlots; ///< Available slot indices
    std::unordered_map<TileCoord, int, TileCoordHash> m_coordToSlot;
    std::list<TileCoord> m_lruOrder; ///< Front = LRU, Back = MRU

    int m_maxSlots = 0;              ///< Maximum slot capacity
    int m_tileSize = 0;              ///< Tile dimension in pixels
    int m_mapWidthTiles = 0;         ///< Map width in tiles
    int m_mapHeightTiles = 0;        ///< Map height in tiles
    GLenum m_heightFormat = GL_R16F; ///< Height texture internal format
    GLenum m_biomeFormat = GL_RGBA8; ///< Biome texture internal format

    /**
     * @brief Update LRU order for accessed slot
     * @param coord Tile coordinate that was accessed
     */
    void UpdateLRU(const TileCoord &coord);

    /**
     * @brief Update height slot lookup texture for a tile
     * @param coord Tile coordinate (layer must be 0)
     * @param slot Slot index, or -1 to clear mapping
     */
    void UpdateHeightSlotMap(const TileCoord& coord, int slot);
};
