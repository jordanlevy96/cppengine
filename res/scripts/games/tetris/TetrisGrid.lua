-- TetrisGrid
--
-- Grid management and game logic for Tetris

local function ResolveModule(role, fallback)
    if SceneModules and SceneModules[role] then
        return SceneModules[role]
    end
    return fallback
end

local C = ResolveModule("constants", TetrisConstants)
local TetriminoModule = ResolveModule("entity", Tetrimino)
local TetriminoDataModule = ResolveModule("data", TetriminoData)
local GameModule = ResolveModule("game", TetrisGame)

local function InitModules(ctx)
    if ctx then
        C = ctx.constants or C
        TetriminoModule = ctx.entity or TetriminoModule
        TetriminoDataModule = ctx.data or TetriminoDataModule
        GameModule = ctx.game or GameModule
    end
    if not C then
        C = ResolveModule("constants", TetrisConstants)
    end
    if not TetriminoModule then
        TetriminoModule = ResolveModule("entity", Tetrimino)
    end
    if not TetriminoDataModule then
        TetriminoDataModule = ResolveModule("data", TetriminoData)
    end
    if not GameModule then
        GameModule = ResolveModule("game", TetrisGame)
    end
end

local function RequireModules()
    if not C then
        error("TetrisGrid missing constants module")
    end
    if not TetriminoModule then
        error("TetrisGrid missing entity module")
    end
    if not TetriminoDataModule then
        error("TetrisGrid missing data module")
    end
    if not GameModule then
        error("TetrisGrid missing game module")
    end
end

InitModules(nil)
RequireModules()

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

-- 7-bag randomizer: shuffle all 7 pieces, deal them out, reshuffle when empty.
-- Guarantees each piece appears once per bag (no long droughts or floods).
local pieceBag = {}

local function selectRandomTetrimino()
    if #pieceBag == 0 then
        pieceBag = {'I', 'O', 'T', 'J', 'L', 'S', 'Z'}
        -- Fisher-Yates shuffle
        for i = #pieceBag, 2, -1 do
            local j = math.random(i)
            pieceBag[i], pieceBag[j] = pieceBag[j], pieceBag[i]
        end
    end
    return table.remove(pieceBag)
end

-- ============================================================================
-- TETRIS GRID CLASS
-- ============================================================================

TetrisGrid = {
    _contract = {
        role = "grid",
        requires = {"constants", "data", "entity", "game"}
    },
    -- Read from YAML
    model = nil,
    shader = nil,

    -- Initialized in ready
    cube = nil,
    grid = {},
    activeTetrimino = nil,  -- Tetrimino instance (OOP)
    nextPieceType = nil,     -- Next piece to spawn

    -- Static
    borderColor = C.BORDER_COLOR,

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

    -- Lock delay state (grace period when piece lands)
    lockDelay = {
        isGrounded = false,      -- Is piece sitting on surface?
        timer = 0,               -- Time accumulated in grounded state (ms)
        resetCount = 0,          -- Number of times lock delay was reset
    },

    -- T-spin and combo tracking
    lastMoveWasRotation = false,
    lastPlacedShape = nil,
    lastPlacedPosition = nil,
    lastPlacedRotation = 0,
    combo = -1,

    -- Gravity timer (independent of tween system)
    gravityTimer = 0,

    -- DAS (Delayed Auto Shift) state
    das = {
        direction = 0,       -- -1 = left, 0 = none, 1 = right
        timer = 0,           -- Time held in current direction (ms)
        charged = false,     -- Has DAS delay elapsed?
        arrTimer = 0,        -- Auto-repeat timer (ms)
    },

    -- Hold piece state
    holdPieceType = nil,
    canHold = true,

    -- Notification popup state
    notification = {
        timer = 0,              -- Time remaining (ms)
        duration = 1500,        -- How long to show (ms)
    },

    -- Ghost preview state
    ghostPreview = {
        isVisible = false,         -- Is ghost currently shown?
        parentEntityID = nil,      -- Parent entity for ghost blocks
        childBlocks = {},          -- Array of child entity IDs (up to 4 blocks)
        dropDistance = 0           -- How far down from active piece
    },

    init = function(self, ctx)
        InitModules(ctx)
        RequireModules()
        self.borderColor = C.BORDER_COLOR
    end,

    ready = function(self)
        RequireModules()
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

            -- Check boundaries (grid is 0-indexed: 0 to GRID_WIDTH-1)
            -- Allow blocks above the visible grid (buffer zone, standard Tetris guideline)
            if newX < 0 or newX >= C.GRID_WIDTH or newY < 0 then
                return true
            end

            -- Only check grid collisions within the visible grid
            if newY < C.GRID_HEIGHT and self.grid[newX][newY] ~= C.GRID_EMPTY_CELL then
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
            self.activeTetrimino:move(vec2(0, -1))
            self:updateGhostPreview()
        else
            -- Enter grounded state (lock delay) instead of immediate lock
            self.lockDelay.isGrounded = true
            self.lockDelay.timer = 0
        end
    end,

    -- Lock the piece (place it, clean up, reset lock delay)
    lockPiece = function(self)
        if self.activeTetrimino == nil then return end

        -- Store info for T-spin detection before placing
        self.lastPlacedShape = self.activeTetrimino.shapeKey
        self.lastPlacedPosition = self.activeTetrimino:getGridPosition()
        self.lastPlacedRotation = self.activeTetrimino.rotationState

        self:placeTetrimino()
        self:destroyGhostPreview()
        DestroyEntity(self.activeTetrimino:getEntityID())
        self.activeTetrimino = nil
        self.lockDelay.isGrounded = false
        self.lockDelay.timer = 0
        self.lockDelay.resetCount = 0
        self.canHold = true
    end,

    moveTetriminoLateral = function(self, direction)
        if self.activeTetrimino == nil then
            return
        end

        local gridPos = self.activeTetrimino:getGridPosition()
        local newPos = vec2(gridPos.x + direction.x, gridPos.y + direction.y)

        if not self:isCollision(newPos, self.activeTetrimino:getChildMap()) then
            self.lastMoveWasRotation = false
            self.activeTetrimino:move(direction)
            -- Update ghost after lateral movement
            self:updateGhostPreview()

            -- Reset lock delay if grounded (piece may have moved off ledge)
            if self.lockDelay.isGrounded and self.lockDelay.resetCount < C.LOCK_DELAY_MAX_RESETS then
                self.lockDelay.timer = 0
                self.lockDelay.resetCount = self.lockDelay.resetCount + 1
            end
        end
    end,

    -- DAS: poll LEFT/RIGHT keys each frame and auto-repeat lateral movement
    updateDAS = function(self, delta)
        if self.activeTetrimino == nil then return end

        local window = GameManager.window
        local leftHeld = window:IsKeyPressed("LEFT")
        local rightHeld = window:IsKeyPressed("RIGHT")

        -- Determine current direction (if both held, cancel out)
        local dir = 0
        if leftHeld and not rightHeld then
            dir = -1
        elseif rightHeld and not leftHeld then
            dir = 1
        end

        if dir == 0 then
            -- No direction held: reset DAS
            self.das.direction = 0
            self.das.timer = 0
            self.das.charged = false
            self.das.arrTimer = 0
            return
        end

        if dir ~= self.das.direction then
            -- Direction changed: reset DAS, fire first move immediately
            self.das.direction = dir
            self.das.timer = 0
            self.das.charged = false
            self.das.arrTimer = 0
            self:moveTetriminoLateral(vec2(dir, 0))
            return
        end

        -- Same direction held: accumulate timer
        self.das.timer = self.das.timer + delta

        if not self.das.charged then
            -- Waiting for initial DAS delay
            if self.das.timer >= C.DAS_DELAY_MS then
                self.das.charged = true
                self.das.arrTimer = 0
                self:moveTetriminoLateral(vec2(dir, 0))
            end
        else
            -- DAS charged: auto-repeat at ARR rate
            self.das.arrTimer = self.das.arrTimer + delta
            if self.das.arrTimer >= C.DAS_ARR_MS then
                self.das.arrTimer = self.das.arrTimer - C.DAS_ARR_MS
                self:moveTetriminoLateral(vec2(dir, 0))
            end
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
            self.score = self.score + 1  -- 1 point per cell soft dropped
            self:updateGhostPreview()
            -- If piece is now grounded, enter lock delay
            local newGridPos = self.activeTetrimino:getGridPosition()
            local belowPos = vec2(newGridPos.x, newGridPos.y - 1)
            if self:isCollision(belowPos, self.activeTetrimino:getChildMap()) then
                self.lockDelay.isGrounded = true
                self.lockDelay.timer = 0
            end
        else
            -- Already grounded and soft drop pressed - lock immediately
            self:lockPiece()
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
        self.score = self.score + (dropDistance * 2)  -- 2 points per cell hard dropped

        -- Hard drop bypasses lock delay - lock immediately
        self:lockPiece()
    end,

    -- ========================================================================
    -- HOLD PIECE
    -- ========================================================================

    holdPiece = function(self)
        if not self.canHold or self.activeTetrimino == nil then
            return
        end

        local currentShape = self.activeTetrimino.shapeKey

        -- Destroy current piece entities
        self.activeTetrimino:destroy()
        self.activeTetrimino = nil
        self:destroyGhostPreview()

        -- Reset lock delay and gravity
        self.lockDelay.isGrounded = false
        self.lockDelay.timer = 0
        self.lockDelay.resetCount = 0
        self.gravityTimer = 0

        if self.holdPieceType == nil then
            -- First hold: store current, let process() spawn next piece naturally
            self.holdPieceType = currentShape
        else
            -- Swap: store current, spawn the held piece
            local heldShape = self.holdPieceType
            self.holdPieceType = currentShape

            -- Spawn the previously held piece
            self.activeTetrimino = self:createTetrimino(heldShape)
            self.activeTetrimino:move(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW))

            -- Check if held piece collides at spawn (game over)
            if self:isCollision(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW), self.activeTetrimino:getChildMap()) then
                self.gameOver = true
                if GameModule then
                    GameModule:gameOver(self.score)
                end
                return
            end

            self:updateGhostPreview()
        end

        -- Can't hold again until next piece is placed
        self.canHold = false

        -- Update UI
        if GameModule then
            GameModule:updateHoldUI(self.holdPieceType)
        end
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

    -- Create ghost preview entities matching the active piece's current rotation
    createGhostPreview = function(self, ghostColor)
        local ghost = self.ghostPreview

        -- Create parent entity
        ghost.parentEntityID = RegisterEntity()
        ghost.childBlocks = {}

        -- Use active piece's current childMap to match rotation state
        local childMap = self.activeTetrimino:getChildMap()

        -- Create child blocks matching active tetrimino's current shape
        for i = 0, 3 do
            for j = 0, 3 do
                if childMap[i][j] ~= C.GRID_EMPTY_CELL then
                    -- Create cube entity
                    local cubeID = RegisterEntity()
                    table.insert(ghost.childBlocks, cubeID)

                    -- Set transform (position relative to parent)
                    -- Offset slightly forward in Z to prevent z-fighting with regular pieces
                    local transform = GetTransform(cubeID)
                    transform.Pos.x = j * C.TETRIMINO_SPACING
                    transform.Pos.y = i * C.TETRIMINO_SPACING
                    transform.Pos.z = 0.1  -- Slightly forward to avoid z-fighting
                    transform.Color = ghostColor  -- Semi-transparent color

                    -- Add render component (no lighting for ghost - should be emissive/unlit)
                    RegisterRenderComponent(cubeID, self.cube)

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

        local tetriminoData = TetriminoDataModule and TetriminoDataModule[self.activeTetrimino.shapeKey] or nil
        if not tetriminoData then
            return
        end
        local ghostColor = vec4(
            tetriminoData.color.x,  
            tetriminoData.color.y,
            tetriminoData.color.z,
            0.2
        )
        self:createGhostPreview(ghostColor)

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
            return false
        end

        local piece = self.activeTetrimino

        -- Cancel any active gravity tween and snap to grid position
        -- (prevents visual desync when rotating mid-gravity-drop)
        piece:cancelTween()

        local newRotationMap = piece:getRotatedChildMap(rotation)
        local gridPos = piece:getGridPosition()
        local fromState = piece.rotationState
        local toState = piece:getNextRotationState(rotation)

        -- Select kick table based on piece type
        local kickKey = fromState .. ">" .. toState
        local kicks

        if piece.shapeKey == "O" then
            -- O-piece: only try original position (no kicks needed)
            kicks = {{0, 0}}
        elseif piece.shapeKey == "I" then
            kicks = C.SRS_KICKS_I[kickKey]
        else
            kicks = C.SRS_KICKS_JLSTZ[kickKey]
        end

        if not kicks then return false end

        -- Try each kick offset
        for _, kick in ipairs(kicks) do
            local testPos = vec2(gridPos.x + kick[1], gridPos.y + kick[2])
            if not self:isCollision(testPos, newRotationMap) then
                -- Apply the kick offset (move piece if kick is non-zero)
                if kick[1] ~= 0 or kick[2] ~= 0 then
                    piece:move(vec2(kick[1], kick[2]))
                end
                piece:rotate(rotation)
                self.lastMoveWasRotation = true

                -- Update ghost after rotation
                self:updateGhostPreview()

                -- Reset lock delay if grounded
                if self.lockDelay.isGrounded and self.lockDelay.resetCount < C.LOCK_DELAY_MAX_RESETS then
                    self.lockDelay.timer = 0
                    self.lockDelay.resetCount = self.lockDelay.resetCount + 1
                end

                return true
            end
        end

        return false  -- All kick positions failed
    end,

    -- ========================================================================
    -- T-SPIN DETECTION
    -- ========================================================================

    -- Check if the last placed T-piece qualifies as a T-spin.
    -- A T-spin requires 3+ of the 4 diagonal corners around the T's center to be occupied.
    -- With SRS-standard rotation states, the T-piece center is always at (j=1, i=2)
    -- in the 4x4 childMap for all rotation states.
    checkTSpin = function(self, gridPos, rotState)
        local cx = gridPos.x + 1
        local cy = gridPos.y + 2

        -- Check 4 diagonal corners around center
        local corners = {
            {cx - 1, cy - 1},
            {cx + 1, cy - 1},
            {cx - 1, cy + 1},
            {cx + 1, cy + 1},
        }

        local occupied = 0
        for _, corner in ipairs(corners) do
            local px, py = corner[1], corner[2]
            if px < 0 or px >= C.GRID_WIDTH or py < 0 or py >= C.GRID_HEIGHT then
                occupied = occupied + 1  -- Wall/floor counts as occupied
            elseif self.grid[px][py] ~= C.GRID_EMPTY_CELL then
                occupied = occupied + 1
            end
        end

        return occupied >= 3
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
        if #linesToClear == 0 then
            -- No lines cleared - reset combo
            self.combo = -1
        end
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

        -- Store affected blocks and apply first color (neon magenta)
        for _, lineY in ipairs(linesToClear) do
            for x = 0, C.GRID_WIDTH - 1 do
                local entity = self.grid[x][lineY]
                if entity ~= C.GRID_EMPTY_CELL then
                    -- Store original color
                    local transform = GetTransform(entity)
                    state.affectedBlocks[entity] = vec4(transform.Color.x, transform.Color.y, transform.Color.z, transform.Color.w)

                    -- Apply first color (neon magenta with full opacity)
                    transform.Color = vec4(1.0, 0.0, 0.8, 1.0)
                end
            end
        end

        -- Don't destroy/collapse yet - that happens in finishClearingLines()
    end,

    updateClearingEffect = function(self, delta)
        local state = self.clearingState
        state.elapsedTime = state.elapsedTime + delta  -- delta is in milliseconds

        -- Stage 1: 0-50ms (Original → Magenta) - already set by clearLines

        -- Stage 2: 50-100ms (Magenta → Cyan)
        if state.elapsedTime >= 50 and state.elapsedTime < 100 then
            for entityID, _ in pairs(state.affectedBlocks) do
                local transform = GetTransform(entityID)
                if transform then
                    transform.Color = vec4(0.0, 0.9, 1.0, 1.0)  -- Electric cyan with full opacity
                end
            end
        end

        -- Stage 3: 100-150ms (Cyan → White flash)
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
        local cleared = #state.clearedLines

        -- T-spin detection (must happen BEFORE grid destruction/collapse)
        local isTSpin = false
        if self.lastPlacedShape == "T" and self.lastMoveWasRotation and self.lastPlacedPosition then
            isTSpin = self:checkTSpin(self.lastPlacedPosition, self.lastPlacedRotation)
        end

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

        -- 3. Calculate and update score (with T-spin, combo, back-to-back)
        self.lines = self.lines + cleared

        -- Base points
        local points = 0
        if isTSpin then
            if cleared == 1 then
                points = 800 * self.level     -- T-Spin Single
            elseif cleared == 2 then
                points = 1200 * self.level    -- T-Spin Double
            elseif cleared == 3 then
                points = 1600 * self.level    -- T-Spin Triple
            end
        else
            if cleared == 1 then
                points = 40 * self.level
            elseif cleared == 2 then
                points = 100 * self.level
            elseif cleared == 3 then
                points = 300 * self.level
            elseif cleared >= 4 then
                points = 1200 * self.level    -- TETRIS!
            end
        end

        -- Combo bonus
        self.combo = self.combo + 1
        if self.combo > 0 then
            points = points + (50 * self.combo * self.level)
        end

        -- Build notification for special clears
        local notifyMain = ""   -- Primary line (DOUBLE, TRIPLE, N I C E)
        local notifyExtra = ""  -- Secondary line (COMBO)
        local hasNotification = false

        if isTSpin then
            if cleared == 1 then notifyMain = "T-SPIN SINGLE"
            elseif cleared == 2 then notifyMain = "T-SPIN DOUBLE"
            elseif cleared == 3 then notifyMain = "T-SPIN TRIPLE"
            end
            hasNotification = true
        else
            if cleared == 2 then notifyMain = "DOUBLE"
            elseif cleared == 3 then notifyMain = "TRIPLE"
            elseif cleared >= 4 then notifyMain = "N I C E"
            end
            if cleared >= 2 then hasNotification = true end
        end

        if self.combo > 0 then
            notifyExtra = "COMBO x" .. (self.combo + 1)
            hasNotification = true
        end

        self.score = self.score + points
        self.level = math.floor(self.lines / 10) + 1

        if GameModule then
            GameModule:updateUI(self.score, self.lines, self.level, self.nextPieceType)

            -- Show notification if there's anything special
            if hasNotification then
                GameModule:showNotification(notifyMain, notifyExtra)
                self.notification.timer = self.notification.duration
            end
        end

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
        if not TetriminoModule then
            return nil
        end
        return TetriminoModule.new(shapeKey, self.cube, lightID)
    end,

    -- ========================================================================
    -- GAME LOOP
    -- ========================================================================

    process = function(self, delta)
        -- Wait for game to start before spawning tetriminos
        if not GameModule or not GameModule.isStarted then
            return
        end

        if GameModule.isPaused then
            return
        end

        if self.gameOver then
            return
        end

        -- Update notification timer
        if self.notification.timer > 0 then
            self.notification.timer = self.notification.timer - delta
            if self.notification.timer <= 0 then
                self.notification.timer = 0
                if GameModule then
                    GameModule:showNotification("", "")
                end
            end
        end

        -- DAS (Delayed Auto Shift) processing
        self:updateDAS(delta)

        -- Update clearing effect if active (blocks new piece spawn)
        if self.clearingState.isClearing then
            self:updateClearingEffect(delta)
            return
        end

        if self.activeTetrimino == nil then
            -- Create new tetrimino using nextPieceType
            self.activeTetrimino = self:createTetrimino(self.nextPieceType)

            -- Reset lock delay and gravity for new piece
            self.lockDelay.isGrounded = false
            self.lockDelay.timer = 0
            self.lockDelay.resetCount = 0
            self.gravityTimer = 0
            self.canHold = true

            -- Generate next piece for preview
            self.nextPieceType = selectRandomTetrimino()

            -- Update UI with new next piece
            if GameModule then
                GameModule:updateUI(self.score, self.lines, self.level, self.nextPieceType)
            end

            -- Move to spawn position
            self.activeTetrimino:move(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW))

            -- Check if new piece immediately collides (game over)
            if self:isCollision(vec2(C.SPAWN_COLUMN, C.SPAWN_ROW), self.activeTetrimino:getChildMap()) then
                self.gameOver = true
                if GameModule then
                    GameModule:gameOver(self.score)
                end
                return
            end

            -- Create ghost preview for new piece
            self:updateGhostPreview()
        elseif self.lockDelay.isGrounded then
            -- LOCK DELAY: piece is on ground, waiting for timer or player action
            self.lockDelay.timer = self.lockDelay.timer + delta

            -- Re-check: can piece still move down? (player may have moved it off ledge)
            local gridPos = self.activeTetrimino:getGridPosition()
            local testPos = vec2(gridPos.x, gridPos.y - 1)
            if not self:isCollision(testPos, self.activeTetrimino:getChildMap()) then
                -- Piece is no longer grounded - resume falling
                self.lockDelay.isGrounded = false
                self.lockDelay.timer = 0
                self.gravityTimer = 0
            elseif self.lockDelay.timer >= C.LOCK_DELAY_MS then
                -- Timer expired - lock the piece
                self:lockPiece()
            end
        else
            -- Normal gravity: timer-based, independent of lateral movement
            self.gravityTimer = self.gravityTimer + delta
            local speed = C.GetGravitySpeed(self.level)
            if self.gravityTimer >= speed then
                self.gravityTimer = self.gravityTimer - speed
                self:moveTetriminoDown()
            end
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

        -- Reset lock delay, gravity, and DAS state
        self.lockDelay.isGrounded = false
        self.lockDelay.timer = 0
        self.lockDelay.resetCount = 0
        self.gravityTimer = 0
        self.das.direction = 0
        self.das.timer = 0
        self.das.charged = false
        self.das.arrTimer = 0

        -- Reset T-spin and combo tracking
        self.lastMoveWasRotation = false
        self.lastPlacedShape = nil
        self.lastPlacedPosition = nil
        self.lastPlacedRotation = 0
        self.combo = -1

        -- Reset hold piece
        self.holdPieceType = nil
        self.canHold = true

        -- Reset bag and generate new next piece
        pieceBag = {}
        self.nextPieceType = selectRandomTetrimino()

        -- Update UI
        if GameModule then
            GameModule:updateUI(self.score, self.lines, self.level, self.nextPieceType)
            GameModule:updateHoldUI(nil)
        end
    end
}

return TetrisGrid
