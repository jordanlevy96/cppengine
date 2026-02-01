-- SkyBackground.lua
--
-- Configures the fullscreen sky background renderer (C++).
-- This is intentionally lightweight: heavy lifting lives in C++/GLSL.

SkyBackground = {
    enabled = true,

    -- Colors are in linear RGB [0..1].
    topColor = { r = 0.12, g = 0.65, b = 0.90 },
    bottomColor = { r = 0.02, g = 0.02, b = 0.08 },

    -- Horizon:
    -- horizonY is vertical position [0..1] (0=bottom, 1=top).
    -- horizonGlow controls the blend half-width.
    horizonY = 0.40,
    horizonGlow = 0.0,
    horizonColor = { r = 0.95, g = 0.10, b = 0.80 },

    -- Sun:
    -- sunPos is UV [0..1] with (0,0) at bottom-left.
    -- sunRadius is UV-space radius; sunGlow expands the halo radius relative to sunRadius.
    sunPos = { x = 0.72, y = 0.62 },
    sunRadius = 0.06,
    sunGlow = 0.14,
    sunColor = { r = 1.00, g = 0.55, b = 0.15 },

    ready = function(self)
        if not BackgroundSky then
            log_error("[SkyBackground] BackgroundSky API not found")
            return
        end

        BackgroundSky.Configure({
            enabled = self.enabled,
            topColor = self.topColor,
            bottomColor = self.bottomColor,
            horizonY = self.horizonY,
            horizonGlow = self.horizonGlow,
            horizonColor = self.horizonColor,
            sunPos = self.sunPos,
            sunRadius = self.sunRadius,
            sunGlow = self.sunGlow,
            sunColor = self.sunColor
        })

        BackgroundSky.Enable(self.enabled)
    end,

    process = function(self, _deltaMs)
        -- Reserved for future animation hooks.
    end
}
