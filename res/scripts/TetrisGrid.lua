-- TetrisGrid

--[[
================================================================================
TETRIS GRID COORDINATE SYSTEM DOCUMENTATION
================================================================================

Grid Structure:
- Grid is 0-indexed: columns [0, GRID_WIDTH-1], rows [0, GRID_HEIGHT-1]
- Standard Tetris dimensions: 10 columns wide × 20 rows tall
- Origin: Bottom-left corner at (0, 0)
- X-axis: Horizontal, increases left-to-right
- Y-axis: Vertical, increases bottom-to-top

World Coordinates:
- Tetrimino positions use logical grid coordinates (integer positions)
- Rendering uses CUBE_SIZE to scale visual representation
- Grid position [x, y] renders at world position (x * CUBE_SIZE, y * CUBE_SIZE)
- With CUBE_SIZE = 2, grid renders at double scale of logical positions

Tetrimino Spacing:
- TETRIMINO_SPACING = 1.0 controls spacing within tetriminos
- Blocks within a tetrimino are positioned 1 unit apart logically
- Lua CUBE_SIZE = 2 represents actual rendered cube dimensions (cube.obj is 2x2x2)
- These values are independent: spacing is logical, CUBE_SIZE is visual
- Tetrimino blocks positioned at spacing=1 will visually overlap (intentional seamless look)

Tetrimino Representation:
- Each tetrimino uses a 4×4 rotation matrix (indices 0-3)
- Matrix cells contain either block ID or -1 (empty)
- Tetrimino position is its parent transform (bottom-left of 4×4 matrix)
- Rotation matrices handled in Lua (turnMatrixCW/CCW methods)

Grid Storage:
- self.grid[x][y] stores block ID or -1 (empty)
- First index is column (x), second is row (y)
- Matches world coordinate convention
================================================================================
]]--

-- ============================================================================
-- CONSTANTS CONFIGURATION
-- ============================================================================

-- ----------------------------------------------------------------------------
-- GRID CONFIGURATION
-- ----------------------------------------------------------------------------
-- Standard Tetris playfield dimensions (official guideline).
-- Grid is 0-indexed: columns [0, 9], rows [0, 19].
GRID_WIDTH = 10
GRID_HEIGHT = 20

-- Actual rendered size of cube meshes in world units.
-- The cube.obj model has vertices from -1 to +1, making it 2x2x2 units at default scale.
-- NOTE: This is independent of C++ spacingX/spacingY (1.0f in src/Tetris.cpp:10-11).
-- C++ spacing controls logical positioning within tetriminos (blocks can overlap).
-- CUBE_SIZE controls visual size for border rendering and grid display.
CUBE_SIZE = 2

-- Sentinel value indicating an empty grid cell (no tetrimino block present).
GRID_EMPTY_CELL = -1

-- ----------------------------------------------------------------------------
-- SPAWN CONFIGURATION
-- ----------------------------------------------------------------------------
-- Tetriminos spawn near the top-center of the grid.
-- Horizontal: Center column minus 1 (accounts for 4-wide I-tetrimino).
-- Vertical: 4 rows from top (standard Tetris spawn height, allows visibility).
SPAWN_COLUMN = math.floor(GRID_WIDTH / 2) - 1  -- Evaluates to 3 for width=10
SPAWN_ROW = GRID_HEIGHT - 4                    -- Evaluates to 16 for height=20

-- ----------------------------------------------------------------------------
-- RENDERING CONFIGURATION
-- ----------------------------------------------------------------------------
-- Border color: medium gray (same value for R, G, B channels).
BORDER_COLOR_GRAY = 0.471

-- Camera field of view in degrees.
-- Note: Also used as FOV_MAX in input.lua for scroll zoom limiting.
CAMERA_FOV_DEGREES = 45

-- Extra padding around grid dimensions for camera framing calculation.
CAMERA_PADDING = 2

-- Z-axis offset to position camera away from grid plane.
CAMERA_Z_OFFSET = 1

-- ----------------------------------------------------------------------------
-- GAMEPLAY CONFIGURATION
-- ----------------------------------------------------------------------------
-- Time in milliseconds between automatic downward movements.
MOVE_SPEED_MS = 100

-- Rounding offset for converting floating-point positions to integer grid indices.
-- Using 0.5 ensures proper rounding (e.g., 2.3 + 0.5 = 2.8 → floor → 2).
GRID_POSITION_ROUNDING_OFFSET = 0.5

-- ----------------------------------------------------------------------------
-- ROTATION MATRIX CONFIGURATION
-- ----------------------------------------------------------------------------
-- Tetriminos use 4×4 rotation matrices (matching C++ implementation).
-- Valid indices: 0 to ROTATION_MATRIX_SIZE - 1 (i.e., 0 to 3).
ROTATION_MATRIX_SIZE = 4

-- ----------------------------------------------------------------------------
-- TETRIMINO-SPECIFIC CONFIGURATION
-- ----------------------------------------------------------------------------
-- Tetrimino cube spacing within the 4x4 matrix (matches C++ spacingX/Y).
TETRIMINO_SPACING = 1.0

-- Rotation angles in degrees
ROTATION_ANGLE_CW = 90.0
ROTATION_ANGLE_CCW = -90.0

-- Euler axis for tetrimino rotation (yaw/Z axis - rotates in XY plane)
ROTATION_AXIS = vec3(0, 0, 1)

-- NOTE: Rotations enum is defined globally in init.lua

-- ============================================================================
-- END CONSTANTS CONFIGURATION
-- ============================================================================

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
    borderColor = vec3(BORDER_COLOR_GRAY, BORDER_COLOR_GRAY, BORDER_COLOR_GRAY),

    -- dynamic
    timeSinceLastMove = 0,
    gameOver = false,

    -- Tetrimino state (replaces C++ static ActiveTetriminoChildMap)
    activeTetriminoChildMap = nil,  -- 2D Lua table (0-indexed) of entity IDs
    
    ready = function(self)
        for i = 0, GRID_WIDTH - 1 do
            self.grid[i] = {}
            for j = 0, GRID_HEIGHT - 1 do
                self.grid[i][j] = GRID_EMPTY_CELL
            end
        end
        self.cube = CreateRenderComponent(self.shader, self.model)
        TetrisGrid:setCamera()
        TetrisGrid:renderBorder()
    end,

    setCamera = function(self)
        local camera = GameManager.camera
        local maxDimension = math.max(GRID_WIDTH + CAMERA_PADDING, GRID_HEIGHT + CAMERA_PADDING)
        camera:SetPerspective(CAMERA_FOV_DEGREES)
        local distance = maxDimension / math.tan(math.rad(camera.fov) / 2)

        camera.transform.Pos = vec3(GRID_WIDTH - 1, GRID_HEIGHT - 1, distance + CAMERA_Z_OFFSET)
    end,

    renderBorder = function(self)
        for y = 0, GRID_HEIGHT do
            CreateCube(self.cube, vec3(-CUBE_SIZE, y * CUBE_SIZE, 0), self.borderColor) -- Left border
            CreateCube(self.cube, vec3(GRID_WIDTH * CUBE_SIZE, y * CUBE_SIZE, 0), self.borderColor) -- Right border
        end
        for x = -1, GRID_WIDTH do
            CreateCube(self.cube, vec3(x * CUBE_SIZE, GRID_HEIGHT * CUBE_SIZE, 0), self.borderColor) -- Top border
            CreateCube(self.cube, vec3(x * CUBE_SIZE, -CUBE_SIZE, 0), self.borderColor) -- Bottom border
        end
    end,

    isCollision = function(self, newPosition, newRotationMap)
        -- First, find which blocks exist in this rotation
        local blockOffsets = {}
        for i = 0, ROTATION_MATRIX_SIZE - 1 do
            for j = 0, ROTATION_MATRIX_SIZE - 1 do
                if newRotationMap[i][j] ~= GRID_EMPTY_CELL then
                    table.insert(blockOffsets, {j=j, i=i})
                end
            end
        end

        for _, block in ipairs(blockOffsets) do
            local newX = math.floor(newPosition.x + block.j)
            local newY = math.floor(newPosition.y + block.i)

            -- Check all boundaries (grid is 0-indexed: 0 to GRID_WIDTH-1)
            if newX < 0 or newX >= GRID_WIDTH or newY < 0 or newY >= GRID_HEIGHT then
                return true
            end

            if self.grid[newX][newY] ~= GRID_EMPTY_CELL then
                return true
            end
        end

        return false
    end,

    moveTetriminoDown = function(self)
        local id = self.activePiece
        local pos = self:getTetriminoLoc(id)
        -- Round to integer position for collision checks
        local gridPos = vec2(math.floor(pos.x + GRID_POSITION_ROUNDING_OFFSET), math.floor(pos.y + GRID_POSITION_ROUNDING_OFFSET))
        local newPos = vec2(gridPos.x, gridPos.y - 1)
        if not self:isCollision(newPos, self:getActiveTetriminoChildMap()) then
            self:tweenTetriminoInternal(self.activePiece, vec3(0, -1, 0), MOVE_SPEED_MS)
        else
            self:placeTetrimino()
            self.activePiece = nil
        end
    end,

    moveTetriminoLateral = function(self, direction)
        if self.activePiece == nil then
            return
        end

        local pos = self:getTetriminoLoc(self.activePiece)
        -- Round to integer position for collision checks
        local gridPos = vec2(math.floor(pos.x + GRID_POSITION_ROUNDING_OFFSET), math.floor(pos.y + GRID_POSITION_ROUNDING_OFFSET))
        local newPos = vec2(gridPos.x + direction.x, gridPos.y + direction.y)

        if not self:isCollision(newPos, self:getActiveTetriminoChildMap()) then
            self:moveTetriminoInternal(self.activePiece, direction)
        end
    end,

    rotateTetrimino = function(self, rotation)
        local newRotationMap = self:checkRotation(rotation)
        local pos = self:getTetriminoLoc(self.activePiece)
        -- Round to integer position for collision checks
        local gridPos = vec2(math.floor(pos.x + GRID_POSITION_ROUNDING_OFFSET), math.floor(pos.y + GRID_POSITION_ROUNDING_OFFSET))
        if not self:isCollision(gridPos, newRotationMap) then
            self:rotateTetriminoInternal(self.activePiece, rotation)
        end
    end,

    placeTetrimino = function(self)
        local id = self.activePiece
        local tetriminoLoc = self:getTetriminoLoc(id)
        -- Round to integer position
        local gridPos = vec2(math.floor(tetriminoLoc.x + GRID_POSITION_ROUNDING_OFFSET), math.floor(tetriminoLoc.y + GRID_POSITION_ROUNDING_OFFSET))
        local rotationMap = self:getActiveTetriminoChildMap()

        for i = 0, ROTATION_MATRIX_SIZE - 1 do
            for j = 0, ROTATION_MATRIX_SIZE - 1 do
                local block = rotationMap[i][j]
                if block ~= GRID_EMPTY_CELL then
                    local gridX = gridPos.x + j
                    local gridY = gridPos.y + i

                    if gridX >= 0 and gridX < GRID_WIDTH and gridY >= 0 and gridY < GRID_HEIGHT then
                        self.grid[gridX][gridY] = block
                    end
                end
            end
        end
    end,

    -- ========================================================================
    -- TETRIMINO CREATION (replaces C++ CreateTetrimino & RegisterTetrimino)
    -- ========================================================================

    createTetrimino = function(self, shapeKey)
        local tetriminoData = TetriminoData[shapeKey]
        if not tetriminoData then
            error("Unknown tetrimino shape: " .. shapeKey)
        end

        -- Create parent entity
        local parentID = RegisterEntity()

        -- Initialize child map (2D Lua table, 0-indexed to match C++)
        local childMap = {}
        for i = 0, 3 do
            childMap[i] = {}
            for j = 0, 3 do
                childMap[i][j] = GRID_EMPTY_CELL
            end
        end

        -- Create child cube entities where shape has blocks
        for i = 0, 3 do
            for j = 0, 3 do
                if tetriminoData.shape[i + 1][j + 1] == 1 then  -- Lua arrays are 1-indexed
                    -- Create cube entity
                    local cubeID = RegisterEntity()
                    childMap[i][j] = cubeID

                    -- Set transform (position relative to parent, set color)
                    local transform = GetTransform(cubeID)
                    transform.Pos.x = j * TETRIMINO_SPACING
                    transform.Pos.y = i * TETRIMINO_SPACING
                    transform.Pos.z = 0
                    transform.Color = tetriminoData.color

                    -- Add render component
                    RegisterRenderComponent(cubeID, self.cube)

                    -- Add lighting component (uses scene light entity)
                    local lightID = GetEntityByName("light")
                    RegisterLighting(cubeID, lightID)

                    -- Establish hierarchy
                    AddChild(parentID, cubeID)
                end
            end
        end

        -- Store as active tetrimino
        self.activeTetriminoChildMap = childMap

        -- Add tween component to parent (with C++ move_to function)
        CreateTweenComponent(parentID, 1000)

        return parentID
    end,

    -- ========================================================================
    -- ROTATION (replaces turnMatrixCW/CCW, CheckRotation, RotateTetrimino)
    -- ========================================================================

    turnMatrixCW = function(self)
        local rotated = {}
        for i = 0, 3 do
            rotated[i] = {}
            for j = 0, 3 do
                rotated[i][j] = GRID_EMPTY_CELL
            end
        end

        for i = 0, 3 do
            for j = 0, 3 do
                rotated[j][3 - i] = self.activeTetriminoChildMap[i][j]
            end
        end

        return rotated
    end,

    turnMatrixCCW = function(self)
        local rotated = {}
        for i = 0, 3 do
            rotated[i] = {}
            for j = 0, 3 do
                rotated[i][j] = GRID_EMPTY_CELL
            end
        end

        for i = 0, 3 do
            for j = 0, 3 do
                rotated[3 - j][i] = self.activeTetriminoChildMap[i][j]
            end
        end

        return rotated
    end,

    checkRotation = function(self, rotation)
        if rotation == Rotations.CW then
            return self:turnMatrixCW()
        elseif rotation == Rotations.CCW then
            return self:turnMatrixCCW()
        end
    end,

    rotateTetriminoInternal = function(self, entityID, rotation)
        local angle = (rotation == Rotations.CW) and ROTATION_ANGLE_CW or ROTATION_ANGLE_CCW

        -- Update the child map
        self.activeTetriminoChildMap = self:checkRotation(rotation)

        -- Rotate the parent entity (children follow via hierarchy)
        RotateEntity(entityID, angle, ROTATION_AXIS)
    end,

    -- ========================================================================
    -- MOVEMENT (replaces MoveTetrimino, TweenTetrimino, GetTetriminoLoc)
    -- ========================================================================

    moveTetriminoInternal = function(self, entityID, direction)
        -- Get tween component and update it
        local tween = GetTween(entityID)
        tween.Start.x = tween.Start.x + direction.x
        tween.End.x = tween.End.x + direction.x
        tween.isActive = false  -- Instant movement, no animation

        -- Translate entity immediately
        TranslateEntity(entityID, vec3(direction.x, direction.y, 0))
    end,

    tweenTetriminoInternal = function(self, entityID, direction, duration)
        local transform = GetTransform(entityID)
        local tween = GetTween(entityID)

        tween.elapsed = 0
        tween.Start = transform.Pos
        tween.End = vec3(transform.Pos.x + direction.x,
                          transform.Pos.y + direction.y,
                          transform.Pos.z + direction.z)
        tween.Duration = duration
        tween.isActive = true
    end,

    getTetriminoLoc = function(self, entityID)
        local transform = GetTransform(entityID)
        return vec2(transform.Pos.x, transform.Pos.y)
    end,

    tetriminoFinishedMovement = function(self, entityID)
        local tween = GetTween(entityID)
        return not tween.isActive
    end,

    getActiveTetriminoChildMap = function(self)
        return self.activeTetriminoChildMap
    end,

    -- ========================================================================

    process = function(self, delta)
        if self.gameOver then
            return
        end

        if (self.activePiece == nil) then
            local id = self:createTetrimino("I") --selectRandomTetrimino())
            self.activePiece = id
            self:moveTetriminoInternal(id, vec2(SPAWN_COLUMN, SPAWN_ROW))

            -- Ensure tween is inactive after spawn (instant movement)
            local tween = GetTween(id)
            tween.isActive = false

            -- Check if new piece immediately collides (game over)
            if self:isCollision(vec2(SPAWN_COLUMN, SPAWN_ROW), self:getActiveTetriminoChildMap()) then
                self.gameOver = true
                return
            end
        elseif (self:tetriminoFinishedMovement(self.activePiece)) then
            self:moveTetriminoDown()
        end
    end
}
