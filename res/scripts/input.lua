HandleInput = function()
    while #eventQueue > 0 do
        local event = table.remove(eventQueue, 1)
        if event.type == InputTypes.KEY then
            print('Key press! Key: ' .. event.input)
            OnKeyPress(event.input)
        end
    end
end

OnKeyPress = function(key)
    if key == nil then
        return
    end

    if key == "Escape" then
        print('closing window')
        GameManager:CloseWindow()
    -- elseif key == "W" then
    --     moveForward()
    -- elseif key == "S" then
    --     moveBackward()
    -- elseif key == "A" then
    --     moveLeft()
    -- elseif key == "D" then
    --     moveRight()
    -- elseif key == "Z" then
    --     moveDown()
    -- elseif key == "X" then
    --     moveUp()
    end
end



return {
    HandleInput, OnKeyPress
}