GameManager = App:GetInstance()
RES_PATH = GameManager.conf.resPath;
CameraRotateFlag = false

-- Rotations enum (moved from C++)
Rotations = {
    CW = 0,
    CCW = 1
}

-- Load Tetris modules (order matters: constants -> data -> classes)
TetrisConstants = dofile(RES_PATH .. "scripts/TetrisConstants.lua")
TetriminoData = dofile(RES_PATH .. "scripts/TetriminoData.lua")
Tetrimino = dofile(RES_PATH .. "scripts/Tetrimino.lua")

dofile(RES_PATH .. "scripts/input.lua")

math.randomseed(os.time())

print("Loaded init.lua")