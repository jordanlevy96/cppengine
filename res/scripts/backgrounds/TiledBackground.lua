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
    -- - heightResolution controls the resolution of the elevation layer (R16) used to form mountains.
    -- - cacheSlots controls how many tiles can be resident at once (more = less churn).
    tileResolution = 256,
    heightResolution = 64,
    cacheSlots = 128,
    maxUploadsPerFrame = 8,

    -- LOD:
    -- - tileWorldSize is the coarsest tile size at lod=0 (world units per tile edge).
    -- - Each finer LOD subdivides by lodScale (tileSize_lod = tileWorldSize / lodScale^lod).
    -- - NOTE: current implementation is a quadtree, so lodScale must be 2.0.
    -- - lodSplitFactor controls when a tile is refined: if a tile's center is closer than
    --   (tileSize * lodSplitFactor) along the camera forward axis, it can split.
    tileWorldSize = 1024,
    lodCount = 16,
    lodScale = 2.0,
    lodSplitFactor = 6.0,

    -- Tile mesh:
    -- - meshResolution is the number of segments per tile edge (higher = smoother mountain silhouettes).
    -- - Vertex count per instance is ~ (meshResolution^2 * 2 triangles).
    meshResolution = 48,

    -- Grid (world units):
    -- - gridSpacing is distance between minor (cyan) lines.
    -- - majorEvery makes every Nth minor line a major (magenta) line.
    -- - minorLineWidth/majorLineWidth are HALF-widths (world units). Smaller = thinner/crisper.
    gridSpacing = 4.0,
    majorEvery = 4,
    minorLineWidth = 0.08,
    majorLineWidth = 0.2,

    -- Horizon termination line:
    -- - Grid is rendered out to farDistance and ends on a magenta line.
    -- - horizonLinePixels is the approximate thickness in pixels (AA included).
    horizonLinePixels = 2.0,

    -- Mountains (distant terrain beyond the horizon):
    -- - mountainExtraDistance extends rendering beyond farDistance so displaced terrain is visible.
    -- - mountainHeight controls peak amplitude (world units above planeY).
    -- - mountainNoiseScale converts world units -> noise space (smaller = broader features, larger = busier noise).
    --   Useful ranges:
    --     0.003 - 0.006 : broad "range" shapes (more realistic from a distance)
    --     0.008 - 0.014 : busier ridges / more waves
    -- - mountainDetail (0..1) controls ruggedness:
    --     0.0 : smooth rolling hills
    --     0.5 : ridged peaks with some smoothing (good default)
    --     1.0 : sharp ridges / crags (can look noisy if noiseScale is too large)
    -- - mountainFadeDistance controls how quickly mountains rise after the horizon (world units).
    --   If you see a "wall" at the horizon, increase this; if mountains feel too flat, decrease it.
    -- Baseline tuning (good starting point):
    -- - Keep mountainExtraDistance reasonably large so you see an actual mountain surface, not just a thin horizon band.
    -- - Keep mountainFadeDistance <= mountainExtraDistance for a clean rise after the horizon.
    mountainExtraDistance = 650.0,
    mountainHeight = 60.0,
    mountainNoiseScale = 0.005,
    mountainDetail = 0.65,
    mountainFadeDistance = 70.0,

    -- Mountain composition (optional):
    -- Bias the generated height tiles so there are two dominant ranges (left/right) with a flatter center valley.
    -- This mask is applied in the CPU tile generator (world-X), so it's deterministic and stable per tile key.
    mountainSideStrength = 1.0,   -- 0 disables, 1 full effect
    mountainSideOffsetX = 80.0,   -- half-width of the center "valley" in world units (mountains rise beyond this |X|)
    mountainSideWidthX = 80.0,    -- softness around the valley boundary (world units)
    mountainSideBase = 0.18,      -- baseline amplitude multiplier at the valley center [0..1]

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
            heightResolution = self.heightResolution,
            cacheSlots = self.cacheSlots,
            maxUploadsPerFrame = self.maxUploadsPerFrame,
            tileWorldSize = self.tileWorldSize,
            lodCount = self.lodCount,
            lodScale = self.lodScale,
            lodSplitFactor = self.lodSplitFactor,

            meshResolution = self.meshResolution,

            gridSpacing = self.gridSpacing,
            majorEvery = self.majorEvery,
            minorLineWidth = self.minorLineWidth,
            majorLineWidth = self.majorLineWidth,

            horizonLinePixels = self.horizonLinePixels,

            mountainExtraDistance = self.mountainExtraDistance,
            mountainHeight = self.mountainHeight,
            mountainNoiseScale = self.mountainNoiseScale,
            mountainDetail = self.mountainDetail,
            mountainFadeDistance = self.mountainFadeDistance,

            mountainSideStrength = self.mountainSideStrength,
            mountainSideOffsetX = self.mountainSideOffsetX,
            mountainSideWidthX = self.mountainSideWidthX,
            mountainSideBase = self.mountainSideBase
        })

        BackgroundTiles.Enable(self.enabled)
    end,

    process = function(self, _deltaMs)
        -- Future: hook time-based scrolling or palette changes here if desired.
    end
}
