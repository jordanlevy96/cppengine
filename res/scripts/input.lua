HandleInput = function()
    while #eventQueue > 0 do
        local event = table.remove(eventQueue, 1)
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

    if key == "Escape" then
        GameManager.window:CloseWindow()
    elseif key == "Space" then
        CameraRotateFlag = not CameraRotateFlag
    elseif key == "W" then
        camera:Move(CameraDirections.FORWARD, GameManager.delta)
    elseif key == "S" then
        camera:Move(CameraDirections.BACK, GameManager.delta)
    elseif key == "A" then
        camera:Move(CameraDirections.LEFT, GameManager.delta)
    elseif key == "D" then
        camera:Move(CameraDirections.RIGHT, GameManager.delta)
    end
end

return {
    HandleInput
}
