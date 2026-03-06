-- TetrisGame.lua
-- Main game controller - handles lifecycle and UI state updates
-- Replaces C++ Game::StartGame(), ResetGame(), ReturnToMainMenu()

local function ResolveGrid()
    if SceneModules and SceneModules.grid then
        return SceneModules.grid
    end
    if TetrisGrid then
        return TetrisGrid
    end
    return nil
end

local function ResolveConstants()
    if SceneModules and SceneModules.constants then
        return SceneModules.constants
    end
    if TetrisConstants then
        return TetrisConstants
    end
    return nil
end

local TetrisGame = {
    _contract = {
        role = "game",
        needs = {"ui"}
    },
    -- Game state
    isStarted = false,
    isGameOver = false,
    isPaused = false,

    -- Initialize game (called from input handlers or UI events)
    -- mode: optional string ("standard" or "mini"), defaults to "standard"
    start = function(self, mode)
        mode = mode or "standard"
        print("[TetrisGame] Starting game (mode: " .. mode .. ")")

        -- Apply mode preset before anything else
        local C = ResolveConstants()
        if C and C.ApplyMode then
            C.ApplyMode(mode)
        end

        self.isStarted = true
        self.isGameOver = false
        self.isPaused = false

        -- Update UI state
        SetUIValue("data.gameStarted", true)
        SetUIValue("data.gameOver", false)
        SetUIValue("data.gamePaused", false)
        SetUIValue("data.score", 0)
        SetUIValue("data.lines", 0)
        SetUIValue("data.level", 1)
        self:updatePiecePreview("data.np", nil)
        self:updatePiecePreview("data.hp", nil)
        RefreshUI()

        -- Setup or reset the grid for current mode
        local grid = ResolveGrid()
        if grid then
            if not grid.playfieldReady then
                grid:setupPlayfield()
            else
                grid:reset()
                grid:setCamera()
                grid:renderBorder()
            end
        end
    end,

    -- Reset game (keep playing, reset stats)
    reset = function(self)
        print("[TetrisGame] Resetting game")
        self.isStarted = true
        self.isGameOver = false
        self.isPaused = false

        SetUIValue("data.gameOver", false)
        SetUIValue("data.gameStarted", true)
        SetUIValue("data.gamePaused", false)
        SetUIValue("data.score", 0)
        SetUIValue("data.lines", 0)
        SetUIValue("data.level", 1)
        self:updatePiecePreview("data.np", nil)
        self:updatePiecePreview("data.hp", nil)
        RefreshUI()

        local grid = ResolveGrid()
        if grid then
            grid:reset()
        end
    end,

    -- Return to main menu
    returnToMenu = function(self)
        print("[TetrisGame] Returning to main menu")
        self.isStarted = false
        self.isGameOver = false
        self.isPaused = false

        SetUIValue("data.gameOver", false)
        SetUIValue("data.gameStarted", false)
        SetUIValue("data.gamePaused", false)
        SetUIValue("data.score", 0)
        SetUIValue("data.lines", 0)
        SetUIValue("data.level", 1)
        self:updatePiecePreview("data.np", nil)
        self:updatePiecePreview("data.hp", nil)
        RefreshUI()

        local grid = ResolveGrid()
        if grid then
            grid:reset()
        end
    end,

    -- Game over (show game over screen)
    gameOver = function(self, finalScore)
        print("[TetrisGame] Game Over - Score: " .. finalScore)
        self.isGameOver = true
        self.isPaused = false

        SetUIValue("data.gameOver", true)
        SetUIValue("data.gamePaused", false)
        SetUIValue("data.finalScore", finalScore)
        RefreshUI()
    end,

    pause = function(self)
        if not self.isStarted or self.isGameOver then
            return
        end
        if self.isPaused then
            return
        end
        self.isPaused = true
        SetUIValue("data.gamePaused", true)
        RefreshUI()
    end,

    resume = function(self)
        if not self.isPaused then
            return
        end
        self.isPaused = false
        SetUIValue("data.gamePaused", false)
        RefreshUI()
    end,

    togglePause = function(self)
        if self.isPaused then
            self:resume()
        else
            self:pause()
        end
    end,

    -- Update UI with game stats (called frequently, no RefreshUI to let dirty flag handle batching)
    updateUI = function(self, score, lines, level, nextPiece)
        SetUIValue("data.score", score)
        SetUIValue("data.lines", lines)
        SetUIValue("data.level", level)
        self:updatePiecePreview("data.np", nextPiece)
        -- Note: No RefreshUI() here - let dirty flag handle batching
    end,

    -- Update hold piece display
    updateHoldUI = function(self, holdPiece)
        self:updatePiecePreview("data.hp", holdPiece)
    end,

    -- Show a notification popup (empty string = hide)
    showNotification = function(self, mainText, extraText)
        SetUIValue("data.notifyMain", mainText or "")
        SetUIValue("data.notifyExtra", extraText or "")
    end,

    -- Get hex color string for a piece type (vaporwave palette)
    getPieceColor = function(self, pieceType)
        local colors = {
            I = "#41f0db",  -- Aqua teal
            O = "#ff71ce",  -- Hot pink
            T = "#b967ff",  -- Neon purple
            J = "#01cdfe",  -- Sky blue
            L = "#ff6b9d",  -- Coral pink
            S = "#05ffa1",  -- Mint green
            Z = "#ff00ff",  -- Magenta
        }
        return colors[pieceType] or "#00ff00"
    end,

    -- Set the 8 individual cell color values for a piece preview grid
    -- prefix: "data.np" (next piece) or "data.hp" (hold piece)
    updatePiecePreview = function(self, prefix, pieceType)
        -- Piece shapes as 2 rows of 4 columns (top 2 visible rows of SRS state 0)
        local shapes = {
            I = { {0,0,0,0}, {1,1,1,1} },
            O = { {0,1,1,0}, {0,1,1,0} },
            T = { {0,1,0,0}, {1,1,1,0} },
            J = { {1,0,0,0}, {1,1,1,0} },
            L = { {0,0,1,0}, {1,1,1,0} },
            S = { {0,1,1,0}, {1,1,0,0} },
            Z = { {1,1,0,0}, {0,1,1,0} },
        }

        local shape = (pieceType and pieceType ~= "") and shapes[pieceType] or nil
        local color = shape and self:getPieceColor(pieceType) or "transparent"

        for row = 1, 2 do
            for col = 1, 4 do
                local idx = (row - 1) * 4 + col
                local bg = "transparent"
                if shape and shape[row][col] == 1 then
                    bg = color
                end
                SetUIValue(prefix .. idx, bg)
            end
        end
    end
}

return TetrisGame
