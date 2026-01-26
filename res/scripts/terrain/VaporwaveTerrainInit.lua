-- VaporwaveTerrainInit.lua
-- Vaporwave aesthetic terrain background for Tetris

print("Initializing vaporwave terrain background...")

-- NOTE: Camera is controlled by Tetris - we don't touch it here.
-- The terrain will render centered around the Tetris camera position.
-- Since Tetris camera is at ~(9, 19, 23), terrain will render around that location.

-- Terrain configuration for vaporwave background
local terrainConfig = {
    -- Use full map (will center around camera)
    mapWidth = 43,
    mapHeight = 22,

    tileSize = 512,
    heightScale = 50.0,     -- Flatten mountains for background
    heightOffset = -500.0,  -- WAY down below everything

    wrapHorizontal = true,

    -- Smaller caches since it's just background
    cpuCacheSize = 32,
    gpuCacheSize = 16,

    -- Tile paths
    heightTilesPath = "../res/terrain/height/",
    biomeTilesPath = "../res/terrain/biome/",

    -- VAPORWAVE FOG: Reduced so colors show through
    fogStart = 1000.0,   -- Start way in the distance
    fogEnd = 5000.0,     -- Very far away
    fogColor = { r = 0.2, g = 0.1, b = 0.3 },  -- Dark purple fog (subtle)

    -- Clipmap rings (larger to cover visible area)
    rings = {
        { resolution = 128, texelSize = 4.0 },   -- Ring 0
        { resolution = 128, texelSize = 8.0 },   -- Ring 1
        { resolution = 128, texelSize = 16.0 },  -- Ring 2
        { resolution = 128, texelSize = 32.0 },  -- Ring 3 (wide coverage)
    }
}

-- Initialize terrain
local success = Terrain.Initialize(terrainConfig)

if success then
    -- Disable wireframe for filled vaporwave look
    Terrain.SetWireframe(false)

    print("Vaporwave terrain initialized!")
    print("  Style: Neon gradient fill")
    print("  Fog: Pink/purple haze")
    print("  Rendering as background layer behind Tetris grid")
else
    print("ERROR: Vaporwave terrain initialization failed!")
end
