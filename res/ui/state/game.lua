-- UI State for Game
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
        gamePaused = false,       -- Controls pause screen

        -- Tetris game stats
        score = 0,
        lines = 0,
        level = 1,
        notifyMain = "",         -- Primary notification (DOUBLE, TRIPLE, N I C E, etc.)
        notifyExtra = "",        -- Secondary notification (COMBO)

        -- Next piece preview cells (4x2 grid, np1-np4 = row 1, np5-np8 = row 2)
        np1 = "transparent", np2 = "transparent", np3 = "transparent", np4 = "transparent",
        np5 = "transparent", np6 = "transparent", np7 = "transparent", np8 = "transparent",
        -- Hold piece preview cells (same layout)
        hp1 = "transparent", hp2 = "transparent", hp3 = "transparent", hp4 = "transparent",
        hp5 = "transparent", hp6 = "transparent", hp7 = "transparent", hp8 = "transparent",
        finalScore = 0,          -- Score displayed on game over screen

        -- BackgroundTiles debug (updated by C++)
        bg_tilesSelected = 0,
        bg_tilesCandidates = 0,
        bg_cacheResident = 0,
        bg_uploads = 0,
        bg_pending = 0,
        bg_cacheHits = 0,
        bg_cacheMisses = 0,
        bg_evictions = 0,

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
    },

    -- Methods section: event handlers called from UI interactions
    methods = {
        onStartGame = function(self)
            print("START GAME clicked!")
            local game = SceneModules and SceneModules.game or TetrisGame
            if game and not game.isStarted then
                game:start()
            end
        end,

        onRestart = function(self)
            print("RESTART clicked!")
            local game = SceneModules and SceneModules.game or TetrisGame
            if game then
                game:reset()
            end
        end,

        onMainMenu = function(self)
            print("MAIN MENU clicked!")
            local game = SceneModules and SceneModules.game or TetrisGame
            if game then
                game:returnToMenu()
            end
        end
    }
}
