-- TerrainInit.lua
-- Initializes the terrain system with test configuration

print("Initializing terrain system...")

-- Initialize camera for terrain viewing
local game = Game.GetInstance()
local camera = game.camera
-- Start over the Himalayas (tile 32-33, row 7) - the brightest/highest area of the heightmap
-- World coords: tile 32 * 512 = 16384, tile 7 * 512 = 3584
camera.transform.Pos = vec3(16640.0, 400.0, 3840.0)
camera:SetYawPitch(0.0, -30.0)  -- Look north toward mountains
camera.moveSpeed = 15.0  -- Reduced for precise navigation

-- Configure terrain for global heightmap (21600x10800 pixels)
local terrainConfig = {
    -- Map dimensions (number of tiles, not texels)
    mapWidth = 43,   -- 43 tiles * 512 = 22016 texels wide
    mapHeight = 22,  -- 22 tiles * 512 = 11264 texels tall

    -- Tile settings
    tileSize = 512,
    heightScale = 500.0,  -- Increased for more dramatic terrain

    -- World settings
    wrapHorizontal = true,

    -- Cache sizes
    cpuCacheSize = 64,
    gpuCacheSize = 32,

    -- Tile data paths (relative to build directory)
    heightTilesPath = "../res/terrain/height/",
    biomeTilesPath = "../res/terrain/biome/",

    -- Fog settings (increased range for better visibility)
    fogStart = 500.0,
    fogEnd = 2000.0,

    -- Clipmap ring configuration
    -- Each ring has resolution (grid size) and texelSize (world units per texel)
    rings = {
        { resolution = 128, texelSize = 1.0 },   -- Ring 0: Highest detail, 1m per texel
        { resolution = 128, texelSize = 2.0 },   -- Ring 1: 2m per texel
        { resolution = 128, texelSize = 4.0 },   -- Ring 2: 4m per texel
        { resolution = 128, texelSize = 8.0 },   -- Ring 3: 8m per texel
    }
}

-- Initialize terrain system
local success = Terrain.Initialize(terrainConfig)

if success then
    -- Enable wireframe by default for debugging
    Terrain.SetWireframe(true)

    print("Terrain system initialized successfully!")
    print("  Map size: " .. terrainConfig.mapWidth .. "x" .. terrainConfig.mapHeight)
    print("  Tile size: " .. terrainConfig.tileSize)
    print("  Rings: " .. #terrainConfig.rings)

    -- Print camera position for debugging
    local camPos = camera.transform.Pos
    print("  Camera position: (" .. camPos.x .. ", " .. camPos.y .. ", " .. camPos.z .. ")")
    print("  Camera front: (" .. camera.front.x .. ", " .. camera.front.y .. ", " .. camera.front.z .. ")")
else
    print("ERROR: Terrain initialization failed!")
end
