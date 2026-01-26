/**
 * @file TileCache.cpp
 * @brief LRU caches for terrain tiles (both CPU and GPU)
 * @lines ~280
 *
 * Purpose: Provides efficient tile caching for terrain rendering
 *
 * Key functions:
 * - TileCoord::operator== / TileCoordHash - Coordinate comparison/hashing (line ~25)
 * - CPUTile::SetHeight/GetHeight - Tile data access (line ~40)
 * - CPUTileCache::Get/Put - CPU cache operations (line ~70)
 * - GPUTileCache::Initialize - Create GL texture arrays (line ~150)
 * - GPUTileCache::Upload - Transfer tile data to GPU (line ~220)
 *
 * Thread Safety:
 * - Not thread-safe by default
 * - GPU operations must be called from main thread with GL context
 */

#include "util/TileCache.h"
#include "util/Logger.h"

#include <algorithm>

// ============================================================================
// TileCoord Implementation
// ============================================================================

bool TileCoord::operator==(const TileCoord &other) const
{
    return x == other.x && y == other.y && layer == other.layer;
}

bool TileCoord::operator<(const TileCoord &other) const
{
    if (x != other.x)
        return x < other.x;
    if (y != other.y)
        return y < other.y;
    return layer < other.layer;
}

size_t TileCoordHash::operator()(const TileCoord &coord) const
{
    // Combine hash values using prime multipliers
    size_t h1 = std::hash<int>{}(coord.x);
    size_t h2 = std::hash<int>{}(coord.y);
    size_t h3 = std::hash<int>{}(coord.layer);
    return h1 ^ (h2 * 31) ^ (h3 * 127);
}

// ============================================================================
// CPUTile Implementation
// ============================================================================

void CPUTile::SetHeight(int localX, int localY, uint16_t value, int tileSize)
{
    if (localX < 0 || localX >= tileSize || localY < 0 || localY >= tileSize)
    {
        LOG_WARNING("[CPUTile] SetHeight out of bounds: ({}, {}) for tileSize {}",
                    localX, localY, tileSize);
        return;
    }

    int index = localY * tileSize + localX;
    if (index >= 0 && index < static_cast<int>(heightData.size()))
    {
        heightData[index] = value;
        dirty = true;
    }
}

uint16_t CPUTile::GetHeight(int localX, int localY, int tileSize) const
{
    if (localX < 0 || localX >= tileSize || localY < 0 || localY >= tileSize)
    {
        LOG_WARNING("[CPUTile] GetHeight out of bounds: ({}, {}) for tileSize {}",
                    localX, localY, tileSize);
        return 0;
    }

    int index = localY * tileSize + localX;
    if (index >= 0 && index < static_cast<int>(heightData.size()))
    {
        return heightData[index];
    }
    return 0;
}

// ============================================================================
// CPUTileCache Implementation
// ============================================================================

void CPUTileCache::Initialize(int maxTiles, int tileSize)
{
    m_maxTiles = maxTiles;
    m_tileSize = tileSize;
    m_currentFrame = 0;
    m_hits = 0;
    m_misses = 0;
    m_tiles.clear();
    m_lruOrder.clear();

    LOG_INFO("[CPUTileCache] Initialized with maxTiles={}, tileSize={}", maxTiles, tileSize);
}

void CPUTileCache::Shutdown()
{
    LOG_INFO("[CPUTileCache] Shutting down. Final stats: {} tiles, {} hits, {} misses",
             GetTileCount(), m_hits, m_misses);

    m_tiles.clear();
    m_lruOrder.clear();
}

CPUTile *CPUTileCache::Get(const TileCoord &coord)
{
    auto it = m_tiles.find(coord);
    if (it != m_tiles.end())
    {
        m_hits++;
        it->second->lastAccessFrame = m_currentFrame;
        UpdateLRU(coord);
        return it->second.get();
    }

    m_misses++;
    return nullptr;
}

void CPUTileCache::Put(const TileCoord &coord, std::unique_ptr<CPUTile> tile)
{
    // Evict if at capacity
    while (static_cast<int>(m_tiles.size()) >= m_maxTiles)
    {
        if (!EvictLRU())
        {
            LOG_ERROR("[CPUTileCache] Failed to evict LRU tile");
            break;
        }
    }

    // Remove existing entry if present
    auto existingIt = m_tiles.find(coord);
    if (existingIt != m_tiles.end())
    {
        m_lruOrder.remove(coord);
        m_tiles.erase(existingIt);
    }

    // Insert new tile
    tile->lastAccessFrame = m_currentFrame;
    tile->coord = coord;
    m_tiles[coord] = std::move(tile);
    m_lruOrder.push_back(coord);

    LOG_TRACE_L2("[CPUTileCache] Put tile ({}, {}, layer={}), cache size: {}",
                 coord.x, coord.y, coord.layer, m_tiles.size());
}

bool CPUTileCache::EvictLRU()
{
    if (m_lruOrder.empty())
    {
        return false;
    }

    TileCoord lruCoord = m_lruOrder.front();
    m_lruOrder.pop_front();

    auto it = m_tiles.find(lruCoord);
    if (it != m_tiles.end())
    {
        LOG_TRACE_L2("[CPUTileCache] Evicting LRU tile ({}, {}, layer={})",
                     lruCoord.x, lruCoord.y, lruCoord.layer);
        m_tiles.erase(it);
        return true;
    }

    return false;
}

int CPUTileCache::GetTileCount() const
{
    return static_cast<int>(m_tiles.size());
}

int CPUTileCache::GetCacheHits() const
{
    return m_hits;
}

int CPUTileCache::GetCacheMisses() const
{
    return m_misses;
}

void CPUTileCache::AdvanceFrame()
{
    m_currentFrame++;
}

void CPUTileCache::UpdateLRU(const TileCoord &coord)
{
    // Move coord to back of LRU list (most recently used)
    m_lruOrder.remove(coord);
    m_lruOrder.push_back(coord);
}

// ============================================================================
// GPUTileCache Implementation
// ============================================================================

void GPUTileCache::Initialize(int maxSlots,
                              int tileSize,
                              int mapWidthTiles,
                              int mapHeightTiles,
                              GLenum heightFormat,
                              GLenum biomeFormat)
{
    m_maxSlots = maxSlots;
    m_tileSize = tileSize;
    m_mapWidthTiles = mapWidthTiles;
    m_mapHeightTiles = mapHeightTiles;
    m_heightFormat = heightFormat;
    m_biomeFormat = biomeFormat;

    // Initialize free slots (all slots available)
    m_freeSlots.clear();
    m_freeSlots.reserve(maxSlots);
    for (int i = maxSlots - 1; i >= 0; i--)
    {
        m_freeSlots.push_back(i); // Push in reverse so slot 0 is allocated first
    }

    m_coordToSlot.clear();
    m_lruOrder.clear();

    // Create height texture array (GL_TEXTURE_2D_ARRAY)
    glGenTextures(1, &m_heightArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_heightArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, heightFormat, tileSize, tileSize, maxSlots, 0, GL_RED, GL_UNSIGNED_SHORT, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create biome texture array (GL_TEXTURE_2D_ARRAY)
    glGenTextures(1, &m_biomeArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_biomeArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, biomeFormat, tileSize, tileSize, maxSlots, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    // Create height tile -> slot lookup (integer texture, sized in tiles)
    glGenTextures(1, &m_heightSlotMap);
    glBindTexture(GL_TEXTURE_2D, m_heightSlotMap);

    int slotMapWidth = std::max(1, m_mapWidthTiles);
    int slotMapHeight = std::max(1, m_mapHeightTiles);
    std::vector<int16_t> init(static_cast<size_t>(slotMapWidth) * slotMapHeight, static_cast<int16_t>(-1));

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16I, slotMapWidth, slotMapHeight, 0, GL_RED_INTEGER, GL_SHORT, init.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    LOG_INFO("[GPUTileCache] Initialized with maxSlots={}, tileSize={}, heightFormat=0x{:X}, biomeFormat=0x{:X}",
             maxSlots, tileSize, heightFormat, biomeFormat);
}

void GPUTileCache::Shutdown()
{
    LOG_INFO("[GPUTileCache] Shutting down. Used slots: {}", GetUsedSlotCount());

    if (m_heightArray != 0)
    {
        glDeleteTextures(1, &m_heightArray);
        m_heightArray = 0;
    }

    if (m_biomeArray != 0)
    {
        glDeleteTextures(1, &m_biomeArray);
        m_biomeArray = 0;
    }

    if (m_heightSlotMap != 0)
    {
        glDeleteTextures(1, &m_heightSlotMap);
        m_heightSlotMap = 0;
    }

    m_freeSlots.clear();
    m_coordToSlot.clear();
    m_lruOrder.clear();
}

int GPUTileCache::GetSlot(const TileCoord &coord)
{
    auto it = m_coordToSlot.find(coord);
    if (it != m_coordToSlot.end())
    {
        UpdateLRU(coord);
        return it->second;
    }
    return -1;
}

int GPUTileCache::AllocateSlot(const TileCoord &coord)
{
    // Check if already allocated
    auto existingIt = m_coordToSlot.find(coord);
    if (existingIt != m_coordToSlot.end())
    {
        UpdateLRU(coord);
        return existingIt->second;
    }

    // Get a free slot (evict if necessary)
    int slot = -1;
    if (!m_freeSlots.empty())
    {
        slot = m_freeSlots.back();
        m_freeSlots.pop_back();
    }
    else
    {
        // Evict LRU
        slot = EvictLRU();
        if (slot < 0)
        {
            LOG_ERROR("[GPUTileCache] Failed to allocate slot - eviction failed");
            return -1;
        }
    }

    // Map coord to slot
    m_coordToSlot[coord] = slot;
    m_lruOrder.push_back(coord);
    UpdateHeightSlotMap(coord, slot);

    LOG_TRACE_L2("[GPUTileCache] Allocated slot {} for tile ({}, {}, layer={})",
                 slot, coord.x, coord.y, coord.layer);

    return slot;
}

void GPUTileCache::Upload(int slot, const CPUTile *tile)
{
    if (slot < 0 || slot >= m_maxSlots)
    {
        LOG_ERROR("[GPUTileCache] Upload: invalid slot {}", slot);
        return;
    }

    if (!tile)
    {
        LOG_ERROR("[GPUTileCache] Upload: null tile");
        return;
    }

    // Upload height data (R16F format)
    if (!tile->heightData.empty())
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_heightArray);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0, 0, slot,                // xoffset, yoffset, zoffset (layer)
                        m_tileSize, m_tileSize, 1, // width, height, depth
                        GL_RED, GL_UNSIGNED_SHORT, // format, type
                        tile->heightData.data());
    }

    // Upload biome data (RGBA8 format)
    if (!tile->biomeData.empty())
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_biomeArray);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0, 0, slot,                // xoffset, yoffset, zoffset (layer)
                        m_tileSize, m_tileSize, 1, // width, height, depth
                        GL_RGBA, GL_UNSIGNED_BYTE, // format, type
                        tile->biomeData.data());
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    LOG_TRACE_L2("[GPUTileCache] Uploaded tile ({}, {}, layer={}) to slot {}",
                 tile->coord.x, tile->coord.y, tile->coord.layer, slot);
}

int GPUTileCache::EvictLRU()
{
    if (m_lruOrder.empty())
    {
        return -1;
    }

    TileCoord lruCoord = m_lruOrder.front();
    m_lruOrder.pop_front();

    auto it = m_coordToSlot.find(lruCoord);
    if (it != m_coordToSlot.end())
    {
        int slot = it->second;
        UpdateHeightSlotMap(lruCoord, -1);
        m_coordToSlot.erase(it);

        LOG_TRACE_L2("[GPUTileCache] Evicted LRU tile ({}, {}, layer={}) from slot {}",
                     lruCoord.x, lruCoord.y, lruCoord.layer, slot);

        return slot;
    }

    return -1;
}

GLuint GPUTileCache::GetHeightTextureArray() const
{
    return m_heightArray;
}

GLuint GPUTileCache::GetBiomeTextureArray() const
{
    return m_biomeArray;
}

GLuint GPUTileCache::GetHeightSlotMapTexture() const
{
    return m_heightSlotMap;
}

int GPUTileCache::GetUsedSlotCount() const
{
    return static_cast<int>(m_coordToSlot.size());
}

void GPUTileCache::UpdateLRU(const TileCoord &coord)
{
    // Move coord to back of LRU list (most recently used)
    m_lruOrder.remove(coord);
    m_lruOrder.push_back(coord);
}

void GPUTileCache::UpdateHeightSlotMap(const TileCoord& coord, int slot)
{
    if (coord.layer != 0 || m_heightSlotMap == 0)
    {
        return;
    }

    int slotMapWidth = std::max(1, m_mapWidthTiles);
    int slotMapHeight = std::max(1, m_mapHeightTiles);

    if (coord.x < 0 || coord.x >= slotMapWidth || coord.y < 0 || coord.y >= slotMapHeight)
    {
        return;
    }

    int16_t value = static_cast<int16_t>(slot);
    glBindTexture(GL_TEXTURE_2D, m_heightSlotMap);
    glTexSubImage2D(GL_TEXTURE_2D, 0, coord.x, coord.y, 1, 1, GL_RED_INTEGER, GL_SHORT, &value);
    glBindTexture(GL_TEXTURE_2D, 0);
}
