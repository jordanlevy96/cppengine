/**
 * @file DirtyRectQueue.h
 * @brief Queue and merge runtime height modifications for terrain
 * @lines ~100
 *
 * Quick-stats (Public API):
 * - PushDelta() - Queue additive height edit (line ~50)
 * - PushSet() - Queue absolute height edit (line ~55)
 * - Flush() - Apply and return modified tiles (line ~60)
 * - HasPending() - Check for queued edits (line ~70)
 *
 * Thread Safety:
 * - All methods are thread-safe via mutex
 * - Lua scripts may call from different contexts
 *
 * Implementation: See src/util/DirtyRectQueue.cpp
 */

#pragma once

#include "util/TileCache.h"

#include <mutex>
#include <set>
#include <vector>

/**
 * @brief A rectangular region with a height modification
 */
struct DirtyRect
{
    int x, y, w, h;     ///< World-space coordinates and dimensions
    enum class Op { Delta, Set } op;  ///< Operation type
    float value;        ///< Delta amount or absolute value

    /**
     * @brief Check if two rects overlap
     */
    bool Overlaps(const DirtyRect& other) const;

    /**
     * @brief Merge two rects into bounding box
     */
    static DirtyRect Merge(const DirtyRect& a, const DirtyRect& b);
};

/**
 * @brief Queue for runtime terrain height edits
 *
 * Collects height modifications from game logic (e.g., digging canals)
 * and applies them to CPU tiles in batch. Modified tiles are marked
 * for GPU re-upload.
 *
 * **Usage:**
 * @code
 * // From game logic or Lua
 * queue.PushDelta(100, 200, 50, 10, -5.0f);  // Dig a canal
 *
 * // Each frame in terrain update
 * auto modified = queue.Flush(cpuCache, tileSize, mapW, mapH);
 * for (auto& coord : modified) {
 *     // Re-upload to GPU
 * }
 * @endcode
 *
 * Thread Safety: All methods are mutex-protected.
 */
class DirtyRectQueue
{
public:
    /**
     * @brief Queue an additive height modification
     * @param x World X coordinate
     * @param y World Y coordinate
     * @param w Width in world units
     * @param h Height in world units
     * @param delta Height change amount
     */
    void PushDelta(int x, int y, int w, int h, float delta);

    /**
     * @brief Queue an absolute height set
     * @param x World X coordinate
     * @param y World Y coordinate
     * @param w Width in world units
     * @param h Height in world units
     * @param value Absolute height value
     */
    void PushSet(int x, int y, int w, int h, float value);

    /**
     * @brief Check if there are pending edits
     * @return True if queue is not empty
     */
    bool HasPending() const;

    /**
     * @brief Get number of pending edits
     * @return Queue size
     */
    int GetPendingCount() const;

    /**
     * @brief Process all pending edits
     * @param cache CPU tile cache to modify
     * @param tileSize Tile dimension in pixels
     * @param mapWidth Map width in world units
     * @param mapHeight Map height in world units
     * @return List of modified tile coordinates (need GPU re-upload)
     */
    std::vector<TileCoord> Flush(CPUTileCache& cache, int tileSize, int mapWidth, int mapHeight);

private:
    std::vector<DirtyRect> m_queue;
    mutable std::mutex m_mutex;

    /**
     * @brief Merge adjacent/overlapping rects with same operation
     */
    void MergeAdjacent();

    /**
     * @brief Apply a single rect to the cache
     */
    void ApplyRect(const DirtyRect& rect, CPUTileCache& cache,
                   int tileSize, int mapWidth, int mapHeight,
                   std::set<TileCoord>& modifiedTiles);
};

/**
 * @brief Convert packed height texel to normalized float
 * @param value Height value in [0, 65535]
 * @return Normalized value in [0.0, 1.0]
 */
float HeightU16ToFloat01(uint16_t value);

/**
 * @brief Convert normalized float to packed height texel
 * @param value Normalized value (will be clamped to [0.0, 1.0])
 * @return Height value in [0, 65535]
 */
uint16_t Float01ToHeightU16(float value);
