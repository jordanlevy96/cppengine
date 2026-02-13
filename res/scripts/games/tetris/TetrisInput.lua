-- ============================================================================
-- TetrisInput.lua
-- Tetris-specific input handling
-- ============================================================================

local TetrisInput = {
    _contract = {
        role = "input",
        requires = {"constants", "game"},
        needs = {"input"}
    }
}

local constants = nil
local gameModule = nil
local gridModule = nil

-- Camera field of view limits for scroll zoom functionality.
local FOV_MIN = 1
local FOV_MAX = 45

-- Directional movement vectors for lateral tetrimino movement.
-- X-axis: -1 = left, +1 = right; Y-axis: 0 = no vertical movement.
local DIRECTION_LEFT = vec2(-1, 0)
local DIRECTION_RIGHT = vec2(1, 0)

local OnClick
local OnCursorMove
local OnScroll
local OnCursor
local OnKeyPress

local function ResolveConstants()
    if not constants then
        if SceneModules and SceneModules.constants then
            constants = SceneModules.constants
        elseif TetrisConstants then
            constants = TetrisConstants
        end
    end
    return constants
end

local function ResolveGame()
    if not gameModule then
        if SceneModules and SceneModules.game then
            gameModule = SceneModules.game
        elseif TetrisGame then
            gameModule = TetrisGame
        end
    end
    return gameModule
end

local function ResolveGrid()
    if not gridModule then
        if SceneModules and SceneModules.grid then
            gridModule = SceneModules.grid
        elseif TetrisGrid then
            gridModule = TetrisGrid
        end
    end
    return gridModule
end

TetrisInput.init = function(self, ctx)
    if ctx then
        constants = ctx.constants or constants
        gameModule = ctx.game or gameModule
    end

    local resolved = ResolveConstants()
    if resolved and resolved.CAMERA_FOV_DEGREES then
        FOV_MAX = resolved.CAMERA_FOV_DEGREES
    end
end

local function HandleInput()
    while #EventQueue > 0 do
        local event = table.remove(EventQueue, 1)
        log_trace("HandleInput event:", event)
        if type(event.input) == "table" then
        for key, value in pairs(event.input) do
            log_trace("input", key, value)
        end
        end
        for key, value in pairs(event) do
            log_trace("event", key, value)
        end
        log_trace("HandleInput event type:", tostring(event.type))

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

    log_trace(string.format("[Lua OnClick] Click received: x=%.2f, y=%.2f, button=%d", x, y, button))

    -- Pass to HTMLRendererMT for hit-testing
    local htmlRenderer = GameManager.htmlRenderer
    if htmlRenderer then
        log_trace("[Lua OnClick] Calling htmlRenderer:HandleClickEvent()")
        htmlRenderer:HandleClickEvent(x, y, button)
    else
        log_error("[Lua OnClick] ERROR: GameManager.htmlRenderer is nil!")
    end
end

OnCursorMove = function(input)
    -- input is vec2: {x, y}
    local x = input.x
    local y = input.y

    log_trace(string.format("[Lua OnCursorMove] Cursor move received: x=%.2f, y=%.2f", x, y))

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
    log_debug("[Lua OnKeyPress] Key pressed: " .. key)

    if key == nil then
        return
    end

    local game = ResolveGame()
    local grid = ResolveGrid()

    if key == "P" then
        if game then
            game:togglePause()
        end
        return
    end

    local camera = GameManager.camera

    -- Game Over Screen Handlers
    if grid and grid.gameOver then
        if key == "R" then
            -- Restart the game
            if game then
                game:reset()
            end
        elseif key == "M" then
            -- Return to main menu
            if game then
                game:returnToMenu()
            end
        elseif key == "ESCAPE" then
            -- Allow closing window from game over screen
            GameManager.window:CloseWindow()
        end
        return
    end

    if game and game.isPaused then
        if key == "ESCAPE" then
            GameManager.window:CloseWindow()
        end
        return
    end

    -- Normal Game Handlers
    if key == "ESCAPE" then
        GameManager.window:CloseWindow()
    elseif key == "ENTER" then
        -- Start the game when ENTER is pressed
        if game and not game.isStarted then
            game:start()
        end
    elseif key == "SPACE" then
        -- Hard drop - instantly drop piece to bottom
        if grid then
            grid:hardDrop()
        end
    elseif key == "DOWN" then
        -- Soft drop - move piece down one row immediately
        if grid then
            grid:softDrop()
        end
    elseif key == "Z" then
        if grid then
            grid:rotateTetrimino(Rotations.CCW)
        end
    elseif key == "X" or key == "UP" then
        if grid then
            grid:rotateTetrimino(Rotations.CW)
        end
    -- LEFT/RIGHT handled by DAS polling in TetrisGrid.process()
    elseif key == "C" then
        if grid then
            grid:holdPiece()
        end
    end
end

log_info("Loaded TetrisInput.lua")

TetrisInput.handleInput = function(self)
    HandleInput()
end

return TetrisInput
