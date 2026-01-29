-- TiledBackground.lua
--
-- Configures the procedural tiled background renderer (C++) for Tetris.
-- This is intentionally lightweight: heavy lifting lives in C++.

TiledBackground = {
    -- YAML-configurable (strings supported by C++ Configure parser)
    enabled = true,
    planeY = 0.0,
    farDistance = 150.0,
    widthMultiplier = 1.2,

    -- GPU tile cache:
    -- - tileResolution affects only the *base gradient* (grid lines are procedural in the shader).
    -- - cacheSlots controls how many tiles can be resident at once (more = less churn).
    tileResolution = 256,
    cacheSlots = 128,
    maxUploadsPerFrame = 8,

    -- LOD:
    -- - tileWorldSize is the coarsest tile size at lod=0 (world units per tile edge).
    -- - Each finer LOD subdivides by lodScale (tileSize_lod = tileWorldSize / lodScale^lod).
    -- - lodSplitFactor controls when a tile is refined: if a tile's center is closer than
    --   (tileSize * lodSplitFactor) along the camera forward axis, it can split.
    tileWorldSize = 1024,
    lodCount = 32,
    lodScale = 2.5,
    lodSplitFactor = 6.0,

    -- Grid (world units):
    -- - gridSpacing is distance between minor (cyan) lines.
    -- - majorEvery makes every Nth minor line a major (magenta) line.
    -- - minorLineWidth/majorLineWidth are HALF-widths (world units). Smaller = thinner/crisper.
    gridSpacing = 4.0,
    majorEvery = 4,
    minorLineWidth = 0.08,
    majorLineWidth = 0.24,

    ready = function(self)
        if not BackgroundTiles then
            log_error("[TiledBackground] BackgroundTiles API not found")
            return
        end

        BackgroundTiles.Configure({
            enabled = self.enabled,
            planeY = self.planeY,
            farDistance = self.farDistance,
            widthMultiplier = self.widthMultiplier,
            tileResolution = self.tileResolution,
            cacheSlots = self.cacheSlots,
            maxUploadsPerFrame = self.maxUploadsPerFrame,
            tileWorldSize = self.tileWorldSize,
            lodCount = self.lodCount,
            lodScale = self.lodScale,
            lodSplitFactor = self.lodSplitFactor,
            gridSpacing = self.gridSpacing,
            majorEvery = self.majorEvery,
            minorLineWidth = self.minorLineWidth,
            majorLineWidth = self.majorLineWidth
        })

        BackgroundTiles.Enable(self.enabled)
    end,

    process = function(self, _deltaMs)
        -- Future: hook time-based scrolling or palette changes here if desired.
    end
}
