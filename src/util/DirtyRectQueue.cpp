/**
 * @file DirtyRectQueue.cpp
 * @brief Queue and merge runtime height modifications for terrain
 * @lines ~180
 *
 * Purpose: Manages runtime terrain edits (e.g., digging canals)
 *
 * Key functions:
 * - PushDelta/PushSet() - Queue edits (line ~30, ~15 lines)
 * - Flush() - Apply all edits to tiles (line ~50, ~40 lines)
 * - ApplyRect() - Apply single edit to cache (line ~95, ~50 lines)
 * - HeightU16ToFloat01/Float01ToHeightU16() - R16 conversion (line ~150, ~20 lines)
 */

#include "util/DirtyRectQueue.h"
#include "util/Logger.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// ============================================================================
// DirtyRect Implementation
// ============================================================================

bool DirtyRect::Overlaps(const DirtyRect& other) const
{
    return !(x + w <= other.x || other.x + other.w <= x ||
             y + h <= other.y || other.y + other.h <= y);
}

DirtyRect DirtyRect::Merge(const DirtyRect& a, const DirtyRect& b)
{
    DirtyRect result;
    result.x = std::min(a.x, b.x);
    result.y = std::min(a.y, b.y);
    int maxX = std::max(a.x + a.w, b.x + b.w);
    int maxY = std::max(a.y + a.h, b.y + b.h);
    result.w = maxX - result.x;
    result.h = maxY - result.y;
    result.op = a.op;  // Assume same op when merging
    result.value = a.value;  // Use first value (simplified)
    return result;
}

// ============================================================================
// DirtyRectQueue Implementation
// ============================================================================

void DirtyRectQueue::PushDelta(int x, int y, int w, int h, float delta)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back({x, y, w, h, DirtyRect::Op::Delta, delta});
}

void DirtyRectQueue::PushSet(int x, int y, int w, int h, float value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back({x, y, w, h, DirtyRect::Op::Set, value});
}

bool DirtyRectQueue::HasPending() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_queue.empty();
}

int DirtyRectQueue::GetPendingCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_queue.size());
}

std::vector<TileCoord> DirtyRectQueue::Flush(CPUTileCache& cache, int tileSize,
                                              int mapWidth, int mapHeight)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_queue.empty())
    {
        return {};
    }

    // Optional: merge adjacent rects
    MergeAdjacent();

    std::set<TileCoord> modifiedTiles;
    int rectCount = static_cast<int>(m_queue.size());

    for (const auto& rect : m_queue)
    {
        ApplyRect(rect, cache, tileSize, mapWidth, mapHeight, modifiedTiles);
    }

    m_queue.clear();

    LOG_DEBUG("[DirtyRectQueue] Flushed {} rects, {} tiles modified",
              rectCount, modifiedTiles.size());

    return std::vector<TileCoord>(modifiedTiles.begin(), modifiedTiles.end());
}

void DirtyRectQueue::MergeAdjacent()
{
    // Simple merge: combine overlapping rects with same operation
    // For now, skip merging to keep implementation simple
    // Production code would implement proper rect coalescing
}

void DirtyRectQueue::ApplyRect(const DirtyRect& rect, CPUTileCache& cache,
                                int tileSize, int mapWidth, int mapHeight,
                                std::set<TileCoord>& modifiedTiles)
{
    for (int py = rect.y; py < rect.y + rect.h; py++)
    {
        for (int px = rect.x; px < rect.x + rect.w; px++)
        {
            // Handle wrap
            int wx = px % mapWidth;
            if (wx < 0) wx += mapWidth;
            int wy = std::clamp(py, 0, mapHeight - 1);

            // Compute tile coord
            TileCoord tileCoord;
            tileCoord.x = wx / tileSize;
            tileCoord.y = wy / tileSize;
            tileCoord.layer = 0;  // Height layer

            // Get tile (skip if not loaded)
            CPUTile* tile = cache.Get(tileCoord);
            if (!tile) continue;

            // Local position within tile
            int localX = wx % tileSize;
            int localY = wy % tileSize;

            // Apply modification
            if (rect.op == DirtyRect::Op::Delta)
            {
                uint16_t current = tile->GetHeight(localX, localY, tileSize);
                float newVal = HeightU16ToFloat01(current) + rect.value;
                tile->SetHeight(localX, localY, Float01ToHeightU16(newVal), tileSize);
            }
            else
            {
                tile->SetHeight(localX, localY, Float01ToHeightU16(rect.value), tileSize);
            }

            tile->dirty = true;
            modifiedTiles.insert(tileCoord);
        }
    }
}

// ============================================================================
// Height Conversion (R16 normalized)
// ============================================================================

float HeightU16ToFloat01(uint16_t value)
{
    return static_cast<float>(value) / 65535.0f;
}

uint16_t Float01ToHeightU16(float value)
{
    float clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<uint16_t>(std::lround(clamped * 65535.0f));
}
