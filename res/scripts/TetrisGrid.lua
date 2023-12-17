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
    moveDownInterval = 1,
    timeSinceLastMove = 0,
    moveSpeed = 500,
    
    ready = function(self)
        for i = 1, self.gridHeight do
            self.grid[i] = {}
            for j = 1, 10 do
                self.grid[i][j] = false
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

    placeTetrimino = function(self, tetriminoID, pos)
        local childMap = GetTetriminoChildMap(tetriminoID)

        for i = 1, 4 do
            for j = 1, 4 do
                local childId = childMap[i][j]
                local gridX = pos.x + j
                local gridY = pos.y + i
                self.grid[gridY][gridX] = childId
            end
        end

    end,

    process = function(self, delta)
        if (self.activePiece == nil) then
            local id = CreateTetrimino(self.cube, selectRandomTetrimino())
            AttachScript(id, "Tetrimino", Tetrimino)
            self.activePiece = id
            MoveTetrimino(id, vec2(5, 17))
        elseif (TetriminoFinishedMovement(self.activePiece)) then
            -- check for collisions
            TweenTetrimino(self.activePiece, vec3(0,-1,0), self.moveSpeed)
        end
    end,

    canRotateTetrimino = function(rotation)
        local rotatedMatrix = CheckRotation(rotation)
    
        -- Check if the rotatedMatrix fits in the grid without colliding ⛔
        for i = 1, 4 do
            for j = 1, 4 do
                if rotatedMatrix[i][j] ~= -1 then
                    -- Check if out of bounds or collides with other pieces
                    print("Looking at " .. rotatedMatrix[i][j] .. " at " .. i .. " " .. j)
                    if j + tetriminoX < 1 or j + tetriminoX > self.gridWidth or 
                       i + tetriminoY > self.gridHeight or self.grid[i + tetriminoY][j + tetriminoX] ~= 0 then
                        return false
                    end
                end
            end
        end
    
        return true 
    end
    
}
