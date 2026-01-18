-- Engine-level initialization only - NO game-specific code
GameManager = Game:GetInstance()
RES_PATH = GameManager.conf.resPath

-- Generic engine flags
CameraRotateFlag = false

-- Generic enum definitions
Rotations = {
    CW = 0,
    CCW = 1
}

-- Initialize random seed
math.randomseed(os.time())

-- NOTE: Game scripts are now loaded via scene YAML "scripts" section
-- See Registry::LoadScene() for the loading mechanism

print("Engine initialized (init.lua)")
