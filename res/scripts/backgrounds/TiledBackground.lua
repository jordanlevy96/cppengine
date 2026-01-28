-- TiledBackground.lua
--
-- Configures the procedural tiled background renderer (C++) for Tetris.
-- This is intentionally lightweight: heavy lifting lives in C++.

TiledBackground = {
    -- YAML-configurable (strings supported by C++ Configure parser)
    enabled = true,
    planeY = 0.0,
    farDistance = 500.0,
    widthMultiplier = 1.2,

	    tileResolution = 256,
	    cacheSlots = 128,
	    maxUploadsPerFrame = 8,
	    tileWorldSize = 64.0,
	    lodCount = 4,
	    lodScale = 2.0,
	    lodSplitFactor = 6.0,

    gridSpacing = 4.0,
    majorEvery = 8,
    lineWidth = 0.08,

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
	            lineWidth = self.lineWidth
	        })

        BackgroundTiles.Enable(self.enabled)
    end,

    process = function(self, _deltaMs)
        -- Future: hook time-based scrolling or palette changes here if desired.
    end
}
