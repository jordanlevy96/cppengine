-- TetrisGrid
--
-- Grid management and game logic for Tetris

-- TetrisConstants and Tetrimino are loaded globally by init.lua
-- Tetrimino will be loaded by dofile after this file
local C = TetrisConstants  -- Shorthand alias

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

local function selectRandomTetrimino()
    local shapes = {'I', 'O', 'T', 'J', 'L', 'S', 'Z'}
    local index = math.random(#shapes)
    return shapes[index]
end

-- ============================================================================
-- TETRIS GRID CLASS
-- ============================================================================

TetrisGrid = {
    -- Read from YAML
    model = nil,
    shader = nil,

    -- Initialized in ready
    cube = nil,
    grid = {},
    activeTetrimino = nil,  -- Tetrimino instance (OOP)

    -- Static
    borderColor = vec3(C.BORDER_COLOR_GRAY, C.BORDER_COLOR_GRAY, C.BORDER_COLOR_GRAY),

    -- Dynamic
    timeSinceLastMove = 0,
    gameOver = false,

    ready = function(self)
        -- Initialize grid
        for i = 0, C.GRID_WIDTH - 1 do
            self.grid[i] = {}
            for j = 0, C.GRID_HEIGHT - 1 do
                self.grid[i][j] = C.GRID_EMPTY_CELL
            end
        end

        self.cube = CreateRenderComponent(self.shader, self.model)
        TetrisGrid:setCamera()
        TetrisGrid:renderBorder()
    end,

    setCamera = function(self)
        local camera = GameManager.camera
        local maxDimension = math.max(C.GRID_WIDTH + C.CAMERA_PADDING, C.GRID_HEIGHT + C.CAMERA_PADDING)
        camera:SetPerspective(C.CAMERA_FOV_DEGREES)
        local distance = maxDimension / math.tan(math.rad(camera.fov) / 2)

        camera.transform.Pos = vec3(C.GRID_WIDTH - 1, C.GRID_HEIGHT - 1, distance + C.CAMERA_Z_OFFSET)
    end,

    renderBorder = function(self)
        for y = 0, C.GRID_HEIGHT do
            CreateCube(self.cube, vec3(-C.CUBE_SIZE, y * C.CUBE_SIZE, 0), self.borderColor) -- Left border
            CreateCube(self.cube, vec3(C.GRID_WIDTH * C.CUBE_SIZE, y * C.CUBE_SIZE, 0), self.borderColor) -- Right border
        end
        for x = -1, C.GRID_WIDTH do
            CreateCube(self.cube, vec3(x * C.CUBE_SIZE, C.GRID_HEIGHT * C.CUBE_SIZE, 0), self.borderColor) -- Top border
            CreateCube(self.cube, vec3(x * C.CUBE_SIZE, -C.CUBE_SIZE, 0), self.borderColor) -- Bottom border
        end
    end,

    -- ========================================================================
    -- COLLISION DETECTION
    -- ========================================================================

    isCollision = function(self, newPosition, rotationMap)
        -- First, find which blocks exist in this rotation
        local blockOffsets = {}
        for i = 0, C.ROTATION_MATRIX_SIZE - 1 do
            for j = 0, C.ROTATION_MATRIX_SIZE - 1 do
                if rotationMap[i][j] ~= C.GRID_EMPTY_CELL then
                    table.insert(blockOffsets, {j=j, i=i})
                end
            end
        end

        for _, block in ipairs(blockOffsets) do
            local newX = math.floor(newPosition.x + block.j)
            local newY = math.floor(newPosition.y + block.i)

            -- Check all boundaries (grid is 0-indexed: 0 to GRID_WIDTH-1)
            if newX < 0 or newX >= C.GRID_WIDTH or newY < 0 or newY >= C.GRID_HEIGHT then
                return true
            end

            if self.grid[newX][newY] ~= C.GRID_EMPTY_CELL then
                return true
            end
        end

        return false
    end,

    -- ========================================================================
    -- TETRIMINO MOVEMENT
    -- ========================================================================

    moveTetriminoDown = function(self)
        if self.activeTetrimino == nil then
            return
        end

        local gridPos = self.activeTetrimino:getGridPosition()
        local newPos = vec2(gridPos.x, gridPos.y - 1)

        if not self:isCollision(newPos, self.activeTetrimino:getChildMap()) then
            self.activeTetrimino:tweenMove(vec3(0, -1, 0), C.MOVE_SPEED_MS)
        else
            self:placeTetrimino()
            self.activeTetrimino = nil
        end
    end,

    moveTetriminoLateral = function(self, direction)
        if self.activeTetrimino == nil then
            return
        end

        local gridPos = self.activeTetrimino:getGridPosition()
        local newPos = vec2(gridPos.x + direction.x, gridPos.y + direction.y)

        if not self:isCollision(newPos, self.activeTetrimino:getChildMap()) then
            self.activeTetrimino:move(direction)
        end
    end,

    -- ========================================================================
    -- TETRIMINO ROTATION
    -- ========================================================================

    rotateTetrimino = function(self, rotation)
        if self.activeTetrimino == nil then
            return
        end

        local newRotationMap = self.activeTetrimino:getRotatedChildMap(rotation)
        local gridPos = self.activeTetrimino:getGridPosition()

        if not self:isCollision(gridPos, newRotationMap) then
            self.activeTetrimino:rotate(rotation)
        end
    end,

    -- ========================================================================
    -- TETRIMINO PLACEMENT
    -- ========================================================================

    placeTetrimino = function(self)
        if self.activeTetrimino == nil then
            return
        end

        local gridPos = self.activeTetrimino:getGridPosition()
        local rotationMap = self.activeTetrimino:getChildMap()

        for i = 0, C.ROTATION_MATRIX_SIZE - 1 do
            for j = 0, C.ROTATION_MATRIX_SIZE - 1 do
                local block = rotationMap[i][j]
                if block ~= C.GRID_EMPTY_CELL then
                    local gridX = gridPos.x + j
                    local gridY = gridPos.y + i

                    if gridX >= 0 and gridX < C.GRID_WIDTH and gridY >= 0 and gridY < C.GRID_HEIGHT then
                        self.grid[gridX][gridY] = block
                    end
                end
            end
        end
    end,

    -- ========================================================================
    -- TETRIMINO CREATION
    -- ========================================================================

    createTetrimino = function(self, shapeKey)
        local lightID = GetEntityByName("light")
        return Tetrimino.new(shapeKey, self.cube, lightID)
    end,

    -- ========================================================================
    -- GAME LOOP
    -- ========================================================================

    process = function(self, delta)
        if self.gameOver then
            return
        end

        if self.activeTetrimino == nil then
            -- Create new tetrimino
            self.activeTetrimino = self:createTetrimino("I") --selectRandomTetrimino())

            -- Move to spawn position
            self.activeTetrimino:move(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW))

            -- Check if new piece immediately collides (game over)
            if self:isCollision(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW), self.activeTetrimino:getChildMap()) then
                self.gameOver = true
                return
            end
        elseif self.activeTetrimino:isMovementFinished() then
            -- Snap position after tween completes to fix floating-point errors
            self.activeTetrimino:updateChildPositions()
            self:moveTetriminoDown()
        end
    end
}
