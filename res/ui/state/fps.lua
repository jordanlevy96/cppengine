-- UI State for FPS Display
-- This file returns a table with reactive data that drives the UI template
-- Values can be updated from C++ using luaState->SetValue("data.fps", 120)

return {
    -- Data section: reactive values that can be updated from C++
    data = {
        fps = 0,
        frameTime = 0.0,
        showDebug = true,

        -- Game mode and simulation speed
        gameMode = "FIXED",       -- "FIXED" or "VARIABLE"
        simSpeed = "NORMAL",      -- Speed name (PAUSED, NORMAL, FAST, etc.)
        simMultiplier = "1.0x",   -- Speed multiplier string

        -- Startup screen control
        gameStarted = false,      -- Controls game start state
        gameOver = false,         -- Controls game over screen

        -- Tetris game stats
        score = 0,
        lines = 0,
        level = 1,
        nextPiece = "I",         -- Next tetromino type
        finalScore = 0,          -- Score displayed on game over screen

        -- Example list for v-for directive
        metrics = {
            { label = "FPS", value = "0", unit = "" },
            { label = "Frame Time", value = "0.00", unit = "ms" },
            { label = "Draw Calls", value = "0", unit = "" }
        }
    },

    -- Computed section: functions that derive values from data
    -- (Not used in Phase 1, but reserved for future)
    computed = {
        fpsColor = function(self)
            if self.data.fps > 60 then
                return "green"
            elseif self.data.fps > 30 then
                return "yellow"
            else
                return "red"
            end
        end,

        performanceStatus = function(self)
            if self.data.fps > 60 then
                return "Excellent"
            elseif self.data.fps > 30 then
                return "Good"
            else
                return "Poor"
            end
        end
    }
}
