local light = GameObject("../res/shaders/Basic.shader", "../res/models/cube.obj")
light:Scale(vec3(0.2))
light:Translate(vec3(-4, 6, 10))
-- light:Rotate(-55, EulerAngles.ROLL)
local bunny = GameObject("../res/shaders/Lighting.shader", "../res/models/xbunny.obj")
bunny:Scale(vec3(3))
bunny:AddUniform("objectColor", vec3(1, 0.5, 0.31), UniformTypeMap.VEC3)
bunny:AddUniform("lightColor", vec3(1), UniformTypeMap.VEC3)
bunny:AddUniform("lightPos", light:GetPosition(), UniformTypeMap.VEC3)

local gm = GameManager:GetInstance()
local registry = gm.registry
registry:RegisterFromLua("light", light)
registry:RegisterFromLua("bunny", bunny)
