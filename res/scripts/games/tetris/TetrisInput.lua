-- ============================================================================
-- TetrisInput.lua
-- Tetris-specific input handling
-- ============================================================================

-- ============================================================================
-- INPUT CONSTANTS
-- ============================================================================

-- Camera field of view limits for scroll zoom functionality.
-- FOV_MAX matches TetrisConstants.CAMERA_FOV_DEGREES (default camera FOV).
FOV_MIN = 1
FOV_MAX = TetrisConstants.CAMERA_FOV_DEGREES

-- Directional movement vectors for lateral tetrimino movement.
-- X-axis: -1 = left, +1 = right; Y-axis: 0 = no vertical movement.
DIRECTION_LEFT = vec2(-1, 0)
DIRECTION_RIGHT = vec2(1, 0)

-- ============================================================================
-- END INPUT CONSTANTS
-- ============================================================================

HandleInput = function()
    while #EventQueue > 0 do
        local event = table.remove(EventQueue, 1)
        print(event)
        if type(event.input) == "table" then
        for key, value in pairs(event.input) do
            print(key, value)
        end
        end
        for key, value in pairs(event) do
            print(key, value)
        end
        print("[Lua HandleInput] Event type: " .. tostring(event.type))

        if event.type == InputTypes.KEY then
            OnKeyPress(event.input)
        elseif event.type == InputTypes.CLICK then
            OnClick(event.input)
        elseif event.type == InputTypes.CURSOR then
            if CameraRotateFlag then
                OnCursor(event.input)
            end
            -- Also update hover state for UI
            OnCursorMove(event.input)
        elseif event.type == InputTypes.SCROLL then
            OnScroll(event.input)
        end
    end
end

OnClick = function(input)
    -- input is vec3: {x, y, button}
    local x = input.x
    local y = input.y
    local button = input.z

    print(string.format("[Lua OnClick] Click received: x=%.2f, y=%.2f, button=%d", x, y, button))

    -- Pass to HTMLRendererMT for hit-testing
    local htmlRenderer = GameManager.htmlRenderer
    if htmlRenderer then
        print("[Lua OnClick] Calling htmlRenderer:HandleClickEvent()")
        htmlRenderer:HandleClickEvent(x, y, button)
    else
        print("[Lua OnClick] ERROR: GameManager.htmlRenderer is nil!")
    end
end

OnCursorMove = function(input)
    -- input is vec2: {x, y}
    local x = input.x
    local y = input.y

    print(string.format("[Lua OnCursorMove] Cursor move received: x=%.2f, y=%.2f", x, y))

    -- Update hover state for UI
    local htmlRenderer = GameManager.htmlRenderer
    if htmlRenderer then
        htmlRenderer:UpdateHoverState(x, y)
    end
end

OnScroll = function(input)
    local camera = GameManager.camera
    local size = GameManager.window:GetSize()
    local fov = camera.fov - input.y
    if fov < FOV_MIN then
        fov = FOV_MIN
    end
    if fov > FOV_MAX then
        fov = FOV_MAX
    end

    camera:SetPerspective(fov, size.x, size.y)
end

OnCursor = function(input)
    GameManager.camera:RotateByMouse(input.x, input.y)
end

OnKeyPress = function(key)
    print("[Lua OnKeyPress] Key pressed: " .. key)

    if key == nil then
        return
    end


    local camera = GameManager.camera

    -- Game Over Screen Handlers
    if TetrisGrid.gameOver then
        if key == "R" then
            -- Restart the game
            TetrisGame:reset()
        elseif key == "M" then
            -- Return to main menu
            TetrisGame:returnToMenu()
        elseif key == "ESCAPE" then
            -- Allow closing window from game over screen
            GameManager.window:CloseWindow()
        end
        return
    end

    -- Normal Game Handlers
    if key == "ESCAPE" then
        GameManager.window:CloseWindow()
    elseif key == "ENTER" then
        -- Start the game when ENTER is pressed
        if not TetrisGame.isStarted then
            TetrisGame:start()
        end
    elseif key == "SPACE" then
        -- Hard drop - instantly drop piece to bottom
        TetrisGrid:hardDrop()
    elseif key == "DOWN" then
        -- Soft drop - move piece down one row immediately
        TetrisGrid:softDrop()
    elseif key == "Z" then
        TetrisGrid:rotateTetrimino(Rotations.CCW)
    elseif key == "X" or key == "UP" then
        TetrisGrid:rotateTetrimino(Rotations.CW)
    elseif key == "LEFT" then
        TetrisGrid:moveTetriminoLateral(DIRECTION_LEFT)
    elseif key == "RIGHT" then
        TetrisGrid:moveTetriminoLateral(DIRECTION_RIGHT)
    elseif key == "P" then
        -- TODO: pause
    end
end

print("Loaded TetrisInput.lua")

return {
    HandleInput
}
