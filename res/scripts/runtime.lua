local gm = GameManager:GetInstance()
local registry = gm.registry
local bunny = registry:GetObjectByName("bunny")
print("Setting uniform at runtime")
bunny:AddUniform("viewPos", cameraPos, UniformTypeMap.VEC3);
print("Finished runtime script")