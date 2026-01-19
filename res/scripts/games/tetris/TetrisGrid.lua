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
    nextPieceType = nil,     -- Next piece to spawn

    -- Static
    borderColor = vec3(C.BORDER_COLOR_GRAY, C.BORDER_COLOR_GRAY, C.BORDER_COLOR_GRAY),

    -- Dynamic
    timeSinceLastMove = 0,
    gameOver = false,
    score = 0,
    lines = 0,
    level = 1,

    -- Line clearing effect state
    clearingState = {
        isClearing = false,        -- Effect in progress?
        clearedLines = {},         -- Line numbers being cleared
        affectedBlocks = {},       -- {entityID = originalColor}
        elapsedTime = 0,           -- Milliseconds since effect started
        duration = 150             -- Total effect duration (ms)
    },

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

        -- Generate first next piece
        self.nextPieceType = selectRandomTetrimino()
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
            -- Destroy only the parent entity to orphan the children
            DestroyEntity(self.activeTetrimino:getEntityID())
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

    softDrop = function(self)
        if self.activeTetrimino == nil then
            return
        end

        local gridPos = self.activeTetrimino:getGridPosition()
        local newPos = vec2(gridPos.x, gridPos.y - 1)

        if not self:isCollision(newPos, self.activeTetrimino:getChildMap()) then
            -- Move instantly (no tween) for responsive soft drop
            self.activeTetrimino:move(vec2(0, -1))
        else
            -- Can't move down - place the piece
            self:placeTetrimino()
            DestroyEntity(self.activeTetrimino:getEntityID())
            self.activeTetrimino = nil
        end
    end,

    hardDrop = function(self)
        if self.activeTetrimino == nil then
            return
        end

        -- Find the lowest position where the piece can be placed
        local gridPos = self.activeTetrimino:getGridPosition()
        local dropDistance = 0

        while true do
            local testPos = vec2(gridPos.x, gridPos.y - dropDistance - 1)
            if self:isCollision(testPos, self.activeTetrimino:getChildMap()) then
                break
            end
            dropDistance = dropDistance + 1
        end

        -- Move the piece instantly to the lowest position
        if dropDistance > 0 then
            self.activeTetrimino:move(vec2(0, -dropDistance))
        end

        -- Place the piece immediately
        self:placeTetrimino()
        DestroyEntity(self.activeTetrimino:getEntityID())
        self.activeTetrimino = nil
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
        local parentID = self.activeTetrimino:getEntityID()

        -- Detach all child blocks from parent and place them in grid
        for i = 0, C.ROTATION_MATRIX_SIZE - 1 do
            for j = 0, C.ROTATION_MATRIX_SIZE - 1 do
                local block = rotationMap[i][j]
                if block ~= C.GRID_EMPTY_CELL then
                    local gridX = gridPos.x + j
                    local gridY = gridPos.y + i

                    if gridX >= 0 and gridX < C.GRID_WIDTH and gridY >= 0 and gridY < C.GRID_HEIGHT then
                        -- Detach child from parent (converts to absolute positioning)
                        RemoveChild(parentID, block)

                        -- Store in grid
                        self.grid[gridX][gridY] = block
                    end
                end
            end
        end

        -- Check for completed lines after placing
        local linesToClear = self:checkLines()
        self:clearLines(linesToClear)
    end,

    -- ========================================================================
    -- LINE CLEARING
    -- ========================================================================

    checkLines = function(self)
        local linesToClear = {}

        for y = 0, C.GRID_HEIGHT - 1 do
            local lineComplete = true

            for x = 0, C.GRID_WIDTH - 1 do
                if self.grid[x][y] == C.GRID_EMPTY_CELL then
                    lineComplete = false
                    break
                end
            end

            if lineComplete then
                table.insert(linesToClear, y)
            end
        end

        return linesToClear
    end,

    clearLines = function(self, linesToClear)
        if #linesToClear == 0 then return end

        local state = self.clearingState

        -- Store which lines are being cleared
        state.clearedLines = linesToClear
        state.isClearing = true
        state.elapsedTime = 0
        state.affectedBlocks = {}

        -- Store affected blocks and apply first color (green)
        for _, lineY in ipairs(linesToClear) do
            for x = 0, C.GRID_WIDTH - 1 do
                local entity = self.grid[x][lineY]
                if entity ~= C.GRID_EMPTY_CELL then
                    -- Store original color
                    local transform = GetTransform(entity)
                    state.affectedBlocks[entity] = vec4(transform.Color.x, transform.Color.y, transform.Color.z, transform.Color.w)

                    -- Apply first color (green with full opacity)
                    transform.Color = vec4(0, 0.8, 0, 1.0)
                end
            end
        end

        -- Don't destroy/collapse yet - that happens in finishClearingLines()
    end,

    updateClearingEffect = function(self, delta)
        local state = self.clearingState
        state.elapsedTime = state.elapsedTime + delta  -- delta is in milliseconds

        -- Stage 1: 0-50ms (Original → Green) - already set by clearLines

        -- Stage 2: 50-100ms (Green → Yellow)
        if state.elapsedTime >= 50 and state.elapsedTime < 100 then
            for entityID, _ in pairs(state.affectedBlocks) do
                local transform = GetTransform(entityID)
                if transform then
                    transform.Color = vec4(0.8, 0.8, 0, 1.0)  -- Yellow with full opacity
                end
            end
        end

        -- Stage 3: 100-150ms (Yellow → White)
        if state.elapsedTime >= 100 and state.elapsedTime < 150 then
            for entityID, _ in pairs(state.affectedBlocks) do
                local transform = GetTransform(entityID)
                if transform then
                    transform.Color = vec4(1, 1, 1, 1.0)  -- White flash with full opacity
                end
            end
        end

        -- Stage 4: 150ms+ (Effect complete - destroy and collapse)
        if state.elapsedTime >= state.duration then
            self:finishClearingLines()
        end
    end,

    finishClearingLines = function(self)
        local state = self.clearingState

        -- 1. Destroy all blocks in cleared lines
        for _, lineY in ipairs(state.clearedLines) do
            for x = 0, C.GRID_WIDTH - 1 do
                if self.grid[x][lineY] ~= C.GRID_EMPTY_CELL then
                    DestroyEntity(self.grid[x][lineY])
                    self.grid[x][lineY] = C.GRID_EMPTY_CELL
                end
            end
        end

        -- 2. Collapse grid (move rows down to fill gaps)
        for y = 0, C.GRID_HEIGHT - 1 do
            local linesBelow = 0
            for _, clearedY in ipairs(state.clearedLines) do
                if clearedY < y then
                    linesBelow = linesBelow + 1
                end
            end

            if linesBelow > 0 then
                local newY = y - linesBelow
                for x = 0, C.GRID_WIDTH - 1 do
                    self.grid[x][newY] = self.grid[x][y]
                    self.grid[x][y] = C.GRID_EMPTY_CELL

                    local entity = self.grid[x][newY]
                    if entity ~= C.GRID_EMPTY_CELL then
                        local transform = GetTransform(entity)
                        transform.Pos.y = newY * C.CUBE_SIZE
                    end
                end
            end
        end

        -- 3. Calculate and update score
        local cleared = #state.clearedLines
        self.lines = self.lines + cleared

        local points = 0
        if cleared == 1 then
            points = 40 * self.level
        elseif cleared == 2 then
            points = 100 * self.level
        elseif cleared == 3 then
            points = 300 * self.level
        elseif cleared >= 4 then
            points = 1200 * self.level  -- TETRIS!
        end

        self.score = self.score + points
        self.level = math.floor(self.lines / 10) + 1

        TetrisGame:updateUI(self.score, self.lines, self.level, self.nextPieceType)

        -- 4. Reset clearing state
        state.isClearing = false
        state.clearedLines = {}
        state.affectedBlocks = {}
        state.elapsedTime = 0
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
        -- Wait for game to start before spawning tetriminos
        if not TetrisGame.isStarted then
            return
        end

        if self.gameOver then
            return
        end

        -- Update clearing effect if active (blocks new piece spawn)
        if self.clearingState.isClearing then
            self:updateClearingEffect(delta)
            return
        end

        if self.activeTetrimino == nil then
            -- Create new tetrimino using nextPieceType
            self.activeTetrimino = self:createTetrimino(self.nextPieceType)

            -- Generate next piece for preview
            self.nextPieceType = selectRandomTetrimino()

            -- Update UI with new next piece
            TetrisGame:updateUI(self.score, self.lines, self.level, self.nextPieceType)

            -- Move to spawn position
            self.activeTetrimino:move(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW))

            -- Check if new piece immediately collides (game over)
            if self:isCollision(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW), self.activeTetrimino:getChildMap()) then
                self.gameOver = true
                TetrisGame:gameOver(self.score)
                return
            end

            -- Start the piece falling immediately
            self:moveTetriminoDown()
        elseif self.activeTetrimino:isMovementFinished() then
            -- Snap position after tween completes to fix floating-point errors
            self.activeTetrimino:updateChildPositions()
            self:moveTetriminoDown()
        end
    end,

    -- ========================================================================
    -- GAME RESET
    -- ========================================================================

    reset = function(self)
        -- Destroy all entities in the grid
        for x = 0, C.GRID_WIDTH - 1 do
            for y = 0, C.GRID_HEIGHT - 1 do
                if self.grid[x][y] ~= C.GRID_EMPTY_CELL then
                    DestroyEntity(self.grid[x][y])
                    self.grid[x][y] = C.GRID_EMPTY_CELL
                end
            end
        end

        -- Destroy active tetrimino if it exists
        if self.activeTetrimino ~= nil then
            self.activeTetrimino:destroy()
            self.activeTetrimino = nil
        end

        -- Reset game stats
        self.score = 0
        self.lines = 0
        self.level = 1
        self.gameOver = false

        -- Reset clearing state
        self.clearingState.isClearing = false
        self.clearingState.clearedLines = {}
        self.clearingState.affectedBlocks = {}
        self.clearingState.elapsedTime = 0

        -- Generate new next piece
        self.nextPieceType = selectRandomTetrimino()

        -- Update UI
        TetrisGame:updateUI(self.score, self.lines, self.level, self.nextPieceType)
    end
}
