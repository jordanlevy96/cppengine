-- TetrisGrid

dofile(RES_PATH .. "scripts/Tetrimino.lua")

local function selectRandomTetrimino()
    local shapes = {'I', 'O', 'T', 'J', 'L', 'S', 'Z'}
    local index = math.random(#shapes)
    return shapes[index]
end

TetrisGrid = {
    -- read from YAML
    model = nil,
    borderShader = nil,
    gameObjectShader = nil,

    -- initialized in ready
    borderCube = nil,
    cube = nil,
    grid = {},
    activePiece = nil,

    -- static
    borderColor = vec3(0.471, 0.471, 0.471),
    cubeSize = 2,
    gridWidth = 10,
    gridHeight = 20,

    moveDownInterval = 1,
    timeSinceLastMove = 0,
    
    ready = function(self)
        print("TetrisGrid readying")
        for i = 1, self.gridHeight do
            self.grid[i] = {}
            for j = 1, 10 do
                self.grid[i][j] = false
            end
        end
        self.borderCube = CreateRenderComponent(self.gameObjectShader, self.model)
        self.cube = CreateRenderComponent(self.gameObjectShader, self.model)
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
            CreateCube(self.borderCube, vec3(-self.cubeSize, y * self.cubeSize, 0), self.borderColor) -- Left border
            CreateCube(self.borderCube, vec3(self.gridWidth * self.cubeSize, y * self.cubeSize, 0), self.borderColor) -- Right border
        end
        for x = -1, self.gridWidth do
            CreateCube(self.borderCube, vec3(x * self.cubeSize, self.gridHeight * self.cubeSize, 0), self.borderColor) -- Top border
            CreateCube(self.borderCube, vec3(x * self.cubeSize, -self.cubeSize, 0), self.borderColor) -- Bottom border
        end
    end,

    placeTetrimino = function(self, tetriminoID, pos)
        local childMap = GetTetriminoChildMap(tetriminoID)

        for i = 1, 4 do
            for j = 1, 4 do
                local childId = childMap[i][j]
                if childId ~= -1 then
                    local gridX = position.x + j
                    local gridY = position.y + i
                    self.grid[gridY][gridX] = childId
                end
            end
        end

    end,

    process = function(self, delta)
        if (self.activePiece == nil) then
            local id = CreateTetrimino(self.cube, selectRandomTetrimino())
            AttachScript(id, "Tetrimino", Tetrimino)
            self.activePiece = id
            MoveTetrimino(id, vec2(10, 34))
        end
    end,

    canRotateTetrimino = function(tetriminoId, rotation)
        local tetriminoMatrix = GetTetriminoChildMap(tetriminoId) -- Get current layout of Tetrimino 📍
        local rotatedMatrix = self:rotateMatrix(tetriminoMatrix, rotation) -- Get the rotated layout 🔄
    
        -- Check if the rotatedMatrix fits in the grid without colliding ⛔
        for i = 1, 4 do
            for j = 1, 4 do
                if rotatedMatrix[i][j] ~= 0 then
                    -- Check if out of bounds or collides with other pieces
                    if j + tetriminoX < 1 or j + tetriminoX > self.gridWidth or 
                       i + tetriminoY > self.gridHeight or self.grid[i + tetriminoY][j + tetriminoX] ~= 0 then
                        return false -- Rotation not possible ❌
                    end
                end
            end
        end
    
        return true -- Rotation is possible ✅
    end
    
}
