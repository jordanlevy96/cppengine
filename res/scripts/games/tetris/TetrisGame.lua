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
    start = function(self)
        print("[TetrisGame] Starting game")
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
        RefreshUI()

        -- Reset the grid
        local grid = ResolveGrid()
        if grid then
            grid:reset()
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
        SetUIValue("data.nextPiece", nextPiece)
        -- Note: No RefreshUI() here - let dirty flag handle batching
    end
}

return TetrisGame
