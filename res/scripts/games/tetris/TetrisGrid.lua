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

    -- Ghost preview state
    ghostPreview = {
        isVisible = false,         -- Is ghost currently shown?
        parentEntityID = nil,      -- Parent entity for ghost blocks
        childBlocks = {},          -- Array of child entity IDs (up to 4 blocks)
        dropDistance = 0           -- How far down from active piece
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
            -- Update ghost after downward movement
            self:updateGhostPreview()
        else
            self:placeTetrimino()
            -- Destroy ghost when placing
            self:destroyGhostPreview()
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
            -- Update ghost after lateral movement
            self:updateGhostPreview()
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
            -- Update ghost after soft drop
            self:updateGhostPreview()
        else
            -- Can't move down - place the piece
            self:placeTetrimino()
            -- Destroy ghost when placing
            self:destroyGhostPreview()
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
        -- Destroy ghost when placing
        self:destroyGhostPreview()
        DestroyEntity(self.activeTetrimino:getEntityID())
        self.activeTetrimino = nil
    end,

    -- ========================================================================
    -- GHOST PREVIEW
    -- ========================================================================

    -- Calculate where the active tetrimino would land if hard-dropped
    calculateGhostDropDistance = function(self)
        if self.activeTetrimino == nil then
            return 0
        end

        local gridPos = self.activeTetrimino:getGridPosition()
        local dropDistance = 0

        -- Reuse hard drop logic
        while true do
            local testPos = vec2(gridPos.x, gridPos.y - dropDistance - 1)
            if self:isCollision(testPos, self.activeTetrimino:getChildMap()) then
                break
            end
            dropDistance = dropDistance + 1
        end

        return dropDistance
    end,

    -- Create ghost preview entities
    createGhostPreview = function(self, shapeKey, ghostColor)
        local ghost = self.ghostPreview

        -- Create parent entity
        ghost.parentEntityID = RegisterEntity()
        ghost.childBlocks = {}

        -- Get tetrimino data for shape
        local tetriminoData = TetriminoData[shapeKey]
        if not tetriminoData then
            error("Unknown tetrimino shape: " .. shapeKey)
        end

        -- Create child blocks matching active tetrimino shape
        for i = 0, 3 do
            for j = 0, 3 do
                -- Check if block exists at this position (convert to 1-indexed for Lua)
                local row = tetriminoData.shape[i + 1]
                if row and row[j + 1] == 1 then
                    -- Create cube entity
                    local cubeID = RegisterEntity()
                    table.insert(ghost.childBlocks, cubeID)

                    -- Set transform (position relative to parent)
                    local transform = GetTransform(cubeID)
                    transform.Pos.x = j * C.TETRIMINO_SPACING
                    transform.Pos.y = i * C.TETRIMINO_SPACING
                    transform.Pos.z = 0
                    transform.Color = ghostColor  -- Semi-transparent color

                    -- Add render component and lighting
                    RegisterRenderComponent(cubeID, self.cube)
                    local lightID = GetEntityByName("light")
                    RegisterLighting(cubeID, lightID)

                    -- Establish hierarchy
                    AddChild(ghost.parentEntityID, cubeID)
                end
            end
        end

        ghost.isVisible = true
    end,

    -- Update ghost preview position based on active tetrimino
    updateGhostPreview = function(self)
        if self.activeTetrimino == nil then
            self:destroyGhostPreview()
            return
        end

        -- Calculate drop distance
        local dropDistance = self:calculateGhostDropDistance()

        -- If drop distance is 0, hide ghost (piece is already at bottom)
        if dropDistance == 0 then
            self:hideGhostPreview()
            return
        end

        -- Destroy old ghost if it exists (to handle rotation changes)
        self:destroyGhostPreview()

        -- Create new ghost with semi-transparent color (alpha=0.4)
        local tetriminoData = TetriminoData[self.activeTetrimino.shapeKey]
        local ghostColor = vec4(
            tetriminoData.color.x,
            tetriminoData.color.y,
            tetriminoData.color.z,
            0.4  -- 40% opacity for transparency
        )
        self:createGhostPreview(self.activeTetrimino.shapeKey, ghostColor)

        -- Sync ghost rotation to match active piece's current rotation
        self:syncGhostRotation()

        -- Get active tetrimino position
        local activePos = self.activeTetrimino:getGridPosition()

        -- Update ghost position (active position minus drop distance)
        local ghost = self.ghostPreview
        local ghostTransform = GetTransform(ghost.parentEntityID)
        ghostTransform.Pos.x = activePos.x * C.CUBE_SIZE
        ghostTransform.Pos.y = (activePos.y - dropDistance) * C.CUBE_SIZE
        ghostTransform.Pos.z = 0

        -- Store drop distance for future comparison
        ghost.dropDistance = dropDistance
    end,

    -- Synchronize ghost rotation to match active tetrimino
    syncGhostRotation = function(self)
        if self.activeTetrimino == nil or not self.ghostPreview.isVisible then
            return
        end

        local ghost = self.ghostPreview
        local activeChildMap = self.activeTetrimino:getChildMap()

        -- Update each ghost child position to match active rotation pattern
        local blockIndex = 1
        for i = 0, 3 do
            for j = 0, 3 do
                if activeChildMap[i][j] ~= C.GRID_EMPTY_CELL then
                    local ghostBlockID = ghost.childBlocks[blockIndex]
                    if ghostBlockID then
                        local transform = GetTransform(ghostBlockID)
                        transform.Pos.x = j * C.TETRIMINO_SPACING
                        transform.Pos.y = i * C.TETRIMINO_SPACING
                        transform.Pos.z = 0
                        blockIndex = blockIndex + 1
                    end
                end
            end
        end
    end,

    -- Hide ghost preview (without destroying entities)
    hideGhostPreview = function(self)
        local ghost = self.ghostPreview

        if not ghost.isVisible then
            return
        end

        -- Move ghost off-screen (y = -100)
        if ghost.parentEntityID then
            local transform = GetTransform(ghost.parentEntityID)
            transform.Pos.y = -100 * C.CUBE_SIZE
        end

        ghost.isVisible = false
    end,

    -- Destroy ghost preview entities
    destroyGhostPreview = function(self)
        local ghost = self.ghostPreview

        if not ghost.parentEntityID then
            return
        end

        -- Destroy all child blocks
        for _, blockID in ipairs(ghost.childBlocks) do
            DestroyEntity(blockID)
        end

        -- Destroy parent entity
        if ghost.parentEntityID then
            DestroyEntity(ghost.parentEntityID)
        end

        -- Reset ghost state
        ghost.parentEntityID = nil
        ghost.childBlocks = {}
        ghost.isVisible = false
        ghost.dropDistance = 0
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
            -- Update ghost after rotation
            self:updateGhostPreview()
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

        -- Hide ghost during clearing effect
        self:hideGhostPreview()

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

            -- Create ghost preview for new piece
            self:updateGhostPreview()

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

        -- Destroy ghost preview
        self:destroyGhostPreview()

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
