GameManager = App:GetInstance()
RES_PATH = GameManager.conf.resPath;
CameraRotateFlag = false

-- Rotations enum (moved from C++)
Rotations = {
    CW = 0,
    CCW = 1
}

-- Load tetrimino data
TetriminoData = dofile(RES_PATH .. "scripts/TetriminoData.lua")

dofile(RES_PATH .. "scripts/input.lua")

math.randomseed(os.time())

print("Loaded init.lua")