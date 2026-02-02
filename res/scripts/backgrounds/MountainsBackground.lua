-- MountainsBackground.lua
--
-- Configures the procedural mountain background renderer (C++).
-- Mountains are rendered as a camera-oriented heightfield strip behind the ground grid.

MountainsBackground = {
    enabled = true,

    -- Placement (world units):
    -- startDistance should match TiledBackground.farDistance so the ground grid reaches the mountains.
    -- depth controls how much "mountain terrain" exists behind the horizon.
    startDistance = 150.0,
    depth = 260.0,
    widthMultiplier = 1.8,

    -- Height shaping (world units):
    baseY = 0.0,
    height = 26.0,
    noiseScale = 0.012,
    detail = 0.55,
    scrollSpeed = 0.0, -- 0 = static mountains

    -- Grid overlay (world units):
    gridSpacing = 6.0,
    majorEvery = 6,
    minorLineWidth = 0.06,
    majorLineWidth = 0.12,

    fillColor = { r = 0.04, g = 0.01, b = 0.06, a = 1.0 },
    minorLineColor = { r = 0.00, g = 0.65, b = 0.95, a = 1.0 },
    majorLineColor = { r = 1.00, g = 0.00, b = 0.80, a = 1.0 },

    ready = function(self)
        if not BackgroundMountains then
            log_error("[MountainsBackground] BackgroundMountains API not found")
            return
        end

        BackgroundMountains.Configure({
            enabled = self.enabled,
            startDistance = self.startDistance,
            depth = self.depth,
            widthMultiplier = self.widthMultiplier,
            baseY = self.baseY,
            height = self.height,
            noiseScale = self.noiseScale,
            detail = self.detail,
            scrollSpeed = self.scrollSpeed,
            gridSpacing = self.gridSpacing,
            majorEvery = self.majorEvery,
            minorLineWidth = self.minorLineWidth,
            majorLineWidth = self.majorLineWidth,
            fillColor = self.fillColor,
            minorLineColor = self.minorLineColor,
            majorLineColor = self.majorLineColor
        })

        BackgroundMountains.Enable(self.enabled)
    end,

    process = function(self, _deltaMs)
        -- Reserved for future animation hooks.
    end
}

