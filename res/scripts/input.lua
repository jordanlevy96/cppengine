HandleInput = function()
    while #event_queue > 0 do
        local event = table.remove(event_queue, 1)
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
    if fov < 1 then
        fov = 1
    end
    if fov > 45 then
        fov = 45
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
        MoveTetrimino(TetrisGrid.activePiece, vec2(-1, 0))
    elseif key == "RIGHT" then
        MoveTetrimino(TetrisGrid.activePiece, vec2(1, 0))
    elseif key == "P" then
        -- TODO: pause
    end
end

return {
    HandleInput
}
