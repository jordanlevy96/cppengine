GameManager = Game:GetInstance()
RES_PATH = GameManager.conf.resPath;
CameraRotateFlag = false
GameStarted = false  -- Controls whether Tetris game has started

-- Rotations enum (moved from C++)
Rotations = {
    CW = 0,
    CCW = 1
}

-- TODO: Generalize import of game modules
-- Load Tetris modules (order matters: constants -> data -> classes)
TetrisConstants = dofile(RES_PATH .. "scripts/TetrisConstants.lua")
TetriminoData = dofile(RES_PATH .. "scripts/TetriminoData.lua")
Tetrimino = dofile(RES_PATH .. "scripts/Tetrimino.lua")

dofile(RES_PATH .. "scripts/input.lua")

math.randomseed(os.time())

print("Loaded init.lua")
