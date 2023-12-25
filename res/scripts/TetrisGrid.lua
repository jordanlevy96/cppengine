-- TetrisGrid

local function selectRandomTetrimino()
    local shapes = {'I', 'O', 'T', 'J', 'L', 'S', 'Z'}
    local index = math.random(#shapes)
    return shapes[index]
end

TetrisGrid = {
    -- read from YAML
    model = nil,
    shader = nil,

    -- initialized in ready
    cube = nil,
    grid = {},
    activePiece = nil,

    -- static
    borderColor = vec3(0.471, 0.471, 0.471),
    cubeSize = 2,
    gridWidth = 10,
    gridHeight = 20,

    -- dynamic
    timeSinceLastMove = 0,
    moveSpeed = 100,
    
    ready = function(self)
        for i = 1, self.gridWidth do
            self.grid[i] = {}
            for j = 1, self.gridHeight do
                self.grid[i][j] = -1
            end
        end
        self.cube = CreateRenderComponent(self.shader, self.model)
        TetrisGrid:setCamera()
        TetrisGrid:renderBorder()
    end,

    setCamera = function(self)
        local camera = GameManager.camera
        local maxDimension = math.max(self.gridWidth + 2, self.gridHeight + 2)
        camera:SetPerspective(45)
        local distance = maxDimension / math.tan(math.rad(camera.fov) / 2)

        camera.transform.Pos = vec3(self.gridWidth - 1, self.gridHeight - 1, distance + 1) 
    end,

    renderBorder = function(self)
        for y = 0, self.gridHeight do
            CreateCube(self.cube, vec3(-self.cubeSize, y * self.cubeSize, 0), self.borderColor) -- Left border
            CreateCube(self.cube, vec3(self.gridWidth * self.cubeSize, y * self.cubeSize, 0), self.borderColor) -- Right border
        end
        for x = -1, self.gridWidth do
            CreateCube(self.cube, vec3(x * self.cubeSize, self.gridHeight * self.cubeSize, 0), self.borderColor) -- Top border
            CreateCube(self.cube, vec3(x * self.cubeSize, -self.cubeSize, 0), self.borderColor) -- Bottom border
        end
    end,

    isCollision = function(self, newPosition, newRotationMap)
        local tetriminoLoc = GetTetriminoLoc(self.activePiece)
        local moveX = newPosition.x - tetriminoLoc.x
        local moveY = newPosition.y - tetriminoLoc.y

        for i = 0, 3 do
            for j = 0, 3 do
                if newRotationMap:get(i, j) ~= -1 then
                    local newX = tetriminoLoc.x + i + moveX + 1
                    local newY = tetriminoLoc.y + j + moveY + 1

                    if newY < 0 then
                        return true
                    end

                    if self.grid[newX][newY] ~= -1 then
                        return true -- Collision with an existing block
                    end

                end
            end
        end

        return false
    end,

    moveTetriminoDown = function(self)
        local id = self.activePiece
        local pos = GetTetriminoLoc(id)
        local newPos = vec2(pos.x, pos.y - 1)
        if not self:isCollision(newPos, GetTetriminoChildMap()) then
            TweenTetrimino(self.activePiece, vec3(0, -1, 0), self.moveSpeed)
        else
            self:placeTetrimino()
            self.activePiece = nil
        end
    end,

    rotateTetrimino = function(self, rotation)
        local newRotationMap = CheckRotation(rotation)
        local pos = GetTetriminoLoc(self.activePiece)
        if not self:isCollision(pos, newRotationMap) then
            RotateTetrimino(self.activePiece, rotation)
        end
    end,

    placeTetrimino = function(self)
        local id = self.activePiece
        local tetriminoLoc = GetTetriminoLoc(id)
        local rotationMap = GetTetriminoChildMap()

        for x = 0, 3 do
            for y = 0, 3 do
                local block = rotationMap:get(x, y)
                if block ~= -1 then
                    local gridX = tetriminoLoc.x + x + 1
                    local gridY = tetriminoLoc.y + y + 1

                    if gridX >= 0 and gridX < self.gridWidth and gridY >= 0 and gridY < self.gridHeight then
                        self.grid[gridY][gridX] = block
                    end

                    if self.grid[gridY][gridX] == -1 then
                        self.grid[gridY][gridX] = block
                    end
                end
            end
        end
    end,

    process = function(self, delta)
        if (self.activePiece == nil) then
            local id = CreateTetrimino(self.cube, "I") --selectRandomTetrimino())
            self.activePiece = id
            MoveTetrimino(id, vec2(5, 17))
        elseif (TetriminoFinishedMovement(self.activePiece)) then
            self:moveTetriminoDown()
        end
    end,
--[[ 
    canRotateTetrimino = function(self, rotation)
        local rotatedMatrix = CheckRotation(rotation)
        local pos = GetTetriminoLoc(self.activePiece)
        local tetriminoX = pos.x
        local tetriminoY = pos.y
        for i = 0, 3 do
            for j = 0, 3 do
                if rotatedMatrix:get(j, i) ~= -1 then
                    local x = math.floor(j + tetriminoX)
                    local y = math.floor(i + tetriminoY)
                    if x < 1 or x > self.gridWidth or 
                       y > self.gridHeight or self.grid[x][y] ~= -1 then
                        return false
                    end
                end
            end
        end
    
        return true 
    end,

    canMoveDown = function(self)
        local pos = GetTetriminoLoc(self.activePiece)
        print("Parent Location: ", pos.x + 1, pos.y + 1)
        local m = GetTetriminoChildMap()
        local tetriminoX = pos.x
        local tetriminoY = pos.y
        for i = 0, 3 do
            for j = 0, 3 do
                local x = math.floor(1 + j + tetriminoX)
                local y = math.floor(1 + i + tetriminoY)
                if m:get(j, i) ~= -1 then
                    -- if y <= 1 or self.grid[x][y - 1] ~= -1 then
                    --     return false
                    if y <= 1 then
                        print('Y too low')
                        return false
                    elseif self.grid[x][y-1] ~= -1 then
                        print('Y-1 filled')
                        print(x, y - 1)
                        print(self.grid[x][y-1])
                        return false
                    end
                end
            end
        end
        return true
    end,
    
    placeTetrimino = function(self)
        local childMap = GetTetriminoChildMap()
        local pos = GetTetriminoLoc(self.activePiece)
        print('------------------------------')
        
        for i = 0, 3 do
            for j = 0, 3 do
                local childId = childMap:get(j, i)
                if (childId ~= -1) then
                    local gridX = pos.x + j + 1
                    local gridY = pos.y + i + 1
                    print("placing " .. childId .. " at " .. "(" .. gridX .. ", " .. gridY .. ")")
                    self.grid[gridX][gridY] = childId
                end
            end
        end
    end,
    
    ]]
    
}
