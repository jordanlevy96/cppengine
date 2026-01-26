-- TerrainInput.lua
-- Input handling for terrain test scene

local TerrainInput = {}

local fastMultiplier = 3.0

-- Mouse state
local mouseCaptured = false

function HandleInput()
    local game = Game.GetInstance()
    local camera = game.camera
    local delta = game.delta

    while #EventQueue > 0 do
        local event = table.remove(EventQueue, 1)
        if event.type == InputTypes.KEY then
            local key = event.input
            local dt = delta

            -- Hold shift for faster movement
            if event.mods and (event.mods % 2 == 1) then  -- Shift modifier
                dt = dt * fastMultiplier
            end

            -- Camera movement (WASD)
            if key == "W" or key == "w" then
                camera:Move(CameraDirections.FORWARD, dt)
            elseif key == "S" or key == "s" then
                camera:Move(CameraDirections.BACK, dt)
            elseif key == "A" or key == "a" then
                camera:Move(CameraDirections.LEFT, dt)
            elseif key == "D" or key == "d" then
                camera:Move(CameraDirections.RIGHT, dt)
            end

            -- Terrain debug toggles
            if key == "T" or key == "t" then
                TerrainInput.wireframeEnabled = not TerrainInput.wireframeEnabled
                Terrain.SetWireframe(TerrainInput.wireframeEnabled)
            elseif key == "R" or key == "r" then
                TerrainInput.clipRingsEnabled = not TerrainInput.clipRingsEnabled
                Terrain.SetShowClipRings(TerrainInput.clipRingsEnabled)
            elseif key == "P" or key == "p" then
                -- Print terrain stats
                if Terrain.IsInitialized() then
                    local stats = Terrain.GetStats()
                    print("=== Terrain Stats ===")
                    print("  CPU Cache: " .. stats.cpuCacheSize .. " tiles")
                    print("  GPU Cache: " .. stats.gpuCacheSize .. " slots")
                    print("  Tiles Loaded: " .. stats.tilesLoaded)
                    print("  Cache Hits: " .. stats.cacheHits)
                    print("  Cache Misses: " .. stats.cacheMisses)
                    print("  Pending Edits: " .. stats.pendingDirtyRects)
                end
            end

            -- Test height edit with 'H' key
            if key == "H" or key == "h" then
                local camPos = camera.transform.Pos
                local x = math.floor(camPos.x)
                local z = math.floor(camPos.z)
                print("Marking height delta at (" .. x .. ", " .. z .. ")")
                Terrain.MarkHeightDelta(x - 10, z - 10, 20, 20, -5.0)
            end

            -- Quit with ESC
            if key == "ESCAPE" then
                game.window:CloseWindow()
            end

        elseif event.type == InputTypes.CURSOR then
            -- Mouse look (when right mouse button is held)
            if mouseCaptured then
                camera:RotateByMouse(event.input.x, event.input.y)
            end

        elseif event.type == InputTypes.CLICK then
            -- Right click to capture mouse for look
            if event.input.z == 1 and event.action == 0 then -- Right button, GLFW_RELEASE
                mouseCaptured = not mouseCaptured
                if mouseCaptured then
                    -- Sync initial mouse position to prevent a jump on first look frame.
                    camera:RotateByMouse(event.input.x, event.input.y)
                end
            end

        elseif event.type == InputTypes.SCROLL then
            -- Scroll to adjust camera height
            local scrollAmount = event.input.y * 10.0
            local pos = camera.transform.Pos
            pos.y = pos.y + scrollAmount
            camera.transform.Pos = pos
        end
    end
end

TerrainInput.wireframeEnabled = false
TerrainInput.clipRingsEnabled = false

return TerrainInput
