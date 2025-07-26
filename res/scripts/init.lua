GameManager = App:GetInstance()
RES_PATH = GameManager.conf.resPath;
CameraRotateFlag = false

dofile(RES_PATH .. "scripts/input.lua")

math.randomseed(os.time())

print("Loaded init.lua")