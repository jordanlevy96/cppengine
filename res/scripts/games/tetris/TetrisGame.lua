-- TetrisGame.lua
-- Main game controller - handles lifecycle and UI state updates
-- Replaces C++ Game::StartGame(), ResetGame(), ReturnToMainMenu()

TetrisGame = {
    -- Game state
    isStarted = false,
    isGameOver = false,

    -- Initialize game (called from input handlers or UI events)
    start = function(self)
        print("[TetrisGame] Starting game")
        self.isStarted = true
        self.isGameOver = false

        -- Update UI state
        SetUIValue("data.gameStarted", true)
        SetUIValue("data.gameOver", false)
        SetUIValue("data.score", 0)
        SetUIValue("data.lines", 0)
        SetUIValue("data.level", 1)
        RefreshUI()

        -- Reset the grid
        TetrisGrid:reset()
    end,

    -- Reset game (keep playing, reset stats)
    reset = function(self)
        print("[TetrisGame] Resetting game")
        self.isStarted = true
        self.isGameOver = false

        SetUIValue("data.gameOver", false)
        SetUIValue("data.gameStarted", true)
        SetUIValue("data.score", 0)
        SetUIValue("data.lines", 0)
        SetUIValue("data.level", 1)
        RefreshUI()

        TetrisGrid:reset()
    end,

    -- Return to main menu
    returnToMenu = function(self)
        print("[TetrisGame] Returning to main menu")
        self.isStarted = false
        self.isGameOver = false

        SetUIValue("data.gameOver", false)
        SetUIValue("data.gameStarted", false)
        SetUIValue("data.score", 0)
        SetUIValue("data.lines", 0)
        SetUIValue("data.level", 1)
        RefreshUI()

        TetrisGrid:reset()
    end,

    -- Game over (show game over screen)
    gameOver = function(self, finalScore)
        print("[TetrisGame] Game Over - Score: " .. finalScore)
        self.isGameOver = true

        SetUIValue("data.gameOver", true)
        SetUIValue("data.finalScore", finalScore)
        RefreshUI()
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

-- Global accessor for backward compatibility with TetrisGrid
GameStarted = false  -- Will be synced with TetrisGame.isStarted

return TetrisGame
