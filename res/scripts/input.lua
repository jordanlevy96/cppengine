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
        if event.type == InputTypes.KEY then
            OnKeyPress(event.input)
        elseif event.type == InputTypes.CURSOR then
            if CameraRotateFlag then
                OnCursor(event.input)
            end
        elseif event.type == InputTypes.SCROLL then
            OnScroll(event.input)
        end
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
    if key == nil then
        return
    end

    local camera = GameManager.camera

    if key == "ESCAPE" then
        GameManager.window:CloseWindow()
    elseif key == "ENTER" then
        -- Start the game when ENTER is pressed
        if not GameStarted then
            GameStarted = true
            GameManager:StartGame()
        end
    elseif key == "SPACE" then
        CameraRotateFlag = not CameraRotateFlag
    elseif key == "W" then
        camera:Move(CameraDirections.FORWARD, GameManager.delta)
    elseif key == "S" then
        camera:Move(CameraDirections.BACK, GameManager.delta)
    elseif key == "A" then
        camera:Move(CameraDirections.LEFT, GameManager.delta)
    elseif key == "D" then
        camera:Move(CameraDirections.RIGHT, GameManager.delta)
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

return {
    HandleInput
}
