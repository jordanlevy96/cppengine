-- FlyingProps.lua
--
-- Scene-agnostic background prop spawner.
-- Attach as a LuaScript component to an entity named "FlyingProps" (or rename the table to match).

local function Trim(s)
    if type(s) ~= "string" then
        return s
    end
    return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function SplitCSV(s)
    local out = {}
    if type(s) ~= "string" then
        return out
    end

    for token in string.gmatch(s, "([^,]+)") do
        local v = Trim(token)
        if v ~= "" then
            table.insert(out, v)
        end
    end

    return out
end

local function ToNumber(v, default)
    if type(v) == "number" then
        return v
    end
    if type(v) == "string" then
        local n = tonumber(v)
        if n ~= nil then
            return n
        end
    end
    return default
end

local function ToBool(v, default)
    if type(v) == "boolean" then
        return v
    end
    if type(v) == "string" then
        local s = string.lower(Trim(v))
        if s == "true" or s == "1" or s == "yes" then
            return true
        end
        if s == "false" or s == "0" or s == "no" then
            return false
        end
    end
    return default
end

local function RandRange(minVal, maxVal)
    local a = minVal
    local b = maxVal
    if b < a then
        a = maxVal
        b = minVal
    end
    return a + (b - a) * math.random()
end

FlyingProps = {
    -- ------------------------------------------------------------------------
    -- Config (YAML overrides arrive as strings)
    -- ------------------------------------------------------------------------
    enabled = true,

    shader = "Basic.shader",
    models = "cube.obj,pyramid.obj,octahedron.obj,bunny.obj",

    spawnRate = 0.8,         -- props/second
    maxProps = 20,           -- active props cap
    speed = 12.0,            -- world units/second along camera.front
    travelSign = 1.0,        -- 1 = along camera.front (toward horizon), -1 = toward camera

    spawnDistanceMin = 60.0, -- distance along camera.front from camera
    spawnDistanceMax = 140.0,
    despawnDistance = 600.0, -- distance along camera.front before despawn

    lateralRange = 50.0,     -- units along camera right vector
    verticalRange = 25.0,    -- units along +Y
    baseYOffset = 0.0,       -- vertical bias

    scaleMin = 0.6,
    scaleMax = 2.0,

    bobAmplitudeMin = 0.0,
    bobAmplitudeMax = 3.0,
    bobSpeedMin = 0.5,
    bobSpeedMax = 1.5,

    -- ------------------------------------------------------------------------
    -- Internal
    -- ------------------------------------------------------------------------
    _modelList = {},
    _renderComponents = {},
    _activeProps = {},
    _spawnAccumulatorMs = 0.0,
    _timeMs = 0.0,

    ready = function(self)
        self:_NormalizeConfig()
        self:_BuildRenderComponents()

        self._activeProps = {}
        self._spawnAccumulatorMs = 0.0
        self._timeMs = 0.0

        -- Seed a small initial set so the scene isn't empty.
        local initial = math.min(8, self.maxProps)
        for _ = 1, initial do
            self:_SpawnProp()
        end
    end,

    process = function(self, deltaMs)
        if not self.enabled then
            return
        end

        local camera = GameManager and GameManager.camera or nil
        if not camera then
            return
        end

        local deltaSec = deltaMs * 0.001
        self._timeMs = self._timeMs + deltaMs

        local front = camera.front
        local camPos = camera.transform.Pos

        -- Camera right vector assuming Y-up world: right = normalize(cross(front, up))
        -- with up=(0,1,0) => right=( -front.z, 0, front.x )
        local rightX = -front.z
        local rightZ = front.x
        local rightLen = math.sqrt(rightX * rightX + rightZ * rightZ)
        if rightLen < 0.0001 then
            rightX = 1.0
            rightZ = 0.0
            rightLen = 1.0
        end
        rightX = rightX / rightLen
        rightZ = rightZ / rightLen

        local moveX = front.x * self.speed * deltaSec * self.travelSign
        local moveY = front.y * self.speed * deltaSec * self.travelSign
        local moveZ = front.z * self.speed * deltaSec * self.travelSign

        local timeSec = self._timeMs * 0.001

        for i = #self._activeProps, 1, -1 do
            local p = self._activeProps[i]
            local t = GetTransform(p.id)
            if t then
                t.Pos.x = t.Pos.x + moveX
                t.Pos.y = t.Pos.y + moveY
                t.Pos.z = t.Pos.z + moveZ

                if p.bobAmplitude > 0.0 then
                    t.Pos.y = p.baseY + math.sin(timeSec * p.bobSpeed + p.bobPhase) * p.bobAmplitude
                end

                local dx = t.Pos.x - camPos.x
                local dy = t.Pos.y - camPos.y
                local dz = t.Pos.z - camPos.z
                local distAlongFront = dx * front.x + dy * front.y + dz * front.z

                if self.travelSign > 0.0 then
                    if distAlongFront > self.despawnDistance then
                        DestroyEntity(p.id)
                        table.remove(self._activeProps, i)
                    end
                else
                    if distAlongFront < -self.despawnDistance then
                        DestroyEntity(p.id)
                        table.remove(self._activeProps, i)
                    end
                end
            else
                table.remove(self._activeProps, i)
            end
        end

        local intervalMs = (self.spawnRate > 0.0) and (1000.0 / self.spawnRate) or 0.0
        if intervalMs > 0.0 and #self._activeProps < self.maxProps then
            self._spawnAccumulatorMs = self._spawnAccumulatorMs + deltaMs
            while self._spawnAccumulatorMs >= intervalMs and #self._activeProps < self.maxProps do
                self._spawnAccumulatorMs = self._spawnAccumulatorMs - intervalMs
                self:_SpawnProp()
            end
        end
    end,

    _NormalizeConfig = function(self)
        self.enabled = ToBool(self.enabled, true)

        self.spawnRate = ToNumber(self.spawnRate, 0.8)
        self.maxProps = math.floor(ToNumber(self.maxProps, 20))
        self.speed = ToNumber(self.speed, 12.0)
        self.travelSign = ToNumber(self.travelSign, 1.0)

        self.spawnDistanceMin = ToNumber(self.spawnDistanceMin, 60.0)
        self.spawnDistanceMax = ToNumber(self.spawnDistanceMax, 140.0)
        self.despawnDistance = ToNumber(self.despawnDistance, 600.0)

        self.lateralRange = ToNumber(self.lateralRange, 50.0)
        self.verticalRange = ToNumber(self.verticalRange, 25.0)
        self.baseYOffset = ToNumber(self.baseYOffset, 0.0)

        self.scaleMin = ToNumber(self.scaleMin, 0.6)
        self.scaleMax = ToNumber(self.scaleMax, 2.0)

        self.bobAmplitudeMin = ToNumber(self.bobAmplitudeMin, 0.0)
        self.bobAmplitudeMax = ToNumber(self.bobAmplitudeMax, 3.0)
        self.bobSpeedMin = ToNumber(self.bobSpeedMin, 0.5)
        self.bobSpeedMax = ToNumber(self.bobSpeedMax, 1.5)

        if self.spawnDistanceMax < self.spawnDistanceMin then
            local tmp = self.spawnDistanceMin
            self.spawnDistanceMin = self.spawnDistanceMax
            self.spawnDistanceMax = tmp
        end

        if self.scaleMax < self.scaleMin then
            local tmp = self.scaleMin
            self.scaleMin = self.scaleMax
            self.scaleMax = tmp
        end

        if type(self.models) == "string" then
            self._modelList = SplitCSV(self.models)
        elseif type(self.models) == "table" then
            self._modelList = self.models
        else
            self._modelList = {"cube.obj"}
        end

        if self.maxProps < 0 then
            self.maxProps = 0
        end
        if self.spawnRate < 0.0 then
            self.spawnRate = 0.0
        end
    end,

    _BuildRenderComponents = function(self)
        self._renderComponents = {}

        for _, model in ipairs(self._modelList) do
            local m = Trim(model)
            if m and m ~= "" then
                local rc = CreateRenderComponent(self.shader, m)
                table.insert(self._renderComponents, rc)
            end
        end

        if #self._renderComponents == 0 then
            table.insert(self._renderComponents, CreateRenderComponent(self.shader, "cube.obj"))
        end
    end,

    _PickColor = function(self)
        -- Default palette (can be overridden by editing this script or adding a config hook later).
        local palette = {
            vec3(1.0, 0.0, 0.8),
            vec3(0.0, 0.9, 1.0),
            vec3(1.0, 0.8, 0.0),
            vec3(0.6, 0.1, 0.9),
            vec3(0.9, 0.2, 0.6)
        }
        local c = palette[math.random(#palette)]
        return vec4(c.x, c.y, c.z, 1.0)
    end,

    _SpawnProp = function(self)
        local camera = GameManager and GameManager.camera or nil
        if not camera then
            return
        end

        local front = camera.front
        local camPos = camera.transform.Pos

        local rightX = -front.z
        local rightZ = front.x
        local rightLen = math.sqrt(rightX * rightX + rightZ * rightZ)
        if rightLen < 0.0001 then
            rightX = 1.0
            rightZ = 0.0
            rightLen = 1.0
        end
        rightX = rightX / rightLen
        rightZ = rightZ / rightLen

        local dist = RandRange(self.spawnDistanceMin, self.spawnDistanceMax)
        local lateral = RandRange(-self.lateralRange, self.lateralRange)
        local vertical = RandRange(-self.verticalRange, self.verticalRange) + self.baseYOffset

        local baseX = camPos.x + front.x * dist
        local baseY = camPos.y + front.y * dist
        local baseZ = camPos.z + front.z * dist

        local x = baseX + rightX * lateral
        local y = baseY + vertical
        local z = baseZ + rightZ * lateral

        local entity = RegisterEntity()
        if self.__entityId then
            AddChild(self.__entityId, entity)
        end

        local t = GetTransform(entity)
        t.Pos.x = x
        t.Pos.y = y
        t.Pos.z = z

        local scale = RandRange(self.scaleMin, self.scaleMax)
        t.Scale = vec3(scale, scale, scale)
        t.Color = self:_PickColor()

        local rc = self._renderComponents[math.random(#self._renderComponents)]
        RegisterRenderComponent(entity, rc)

        local bobAmplitude = RandRange(self.bobAmplitudeMin, self.bobAmplitudeMax)
        local bobSpeed = RandRange(self.bobSpeedMin, self.bobSpeedMax)
        local bobPhase = RandRange(0.0, 6.28318)

        table.insert(self._activeProps, {
            id = entity,
            baseY = y,
            bobAmplitude = bobAmplitude,
            bobSpeed = bobSpeed,
            bobPhase = bobPhase
        })
    end
}

