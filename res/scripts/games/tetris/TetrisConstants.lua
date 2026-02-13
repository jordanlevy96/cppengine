-- TetrisConstants
--
-- Configuration constants for Tetris game
-- TODO: Better data management for scene data

--[[
================================================================================
TETRIS COORDINATE SYSTEM DOCUMENTATION
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
- grid[x][y] stores block ID or -1 (empty)
- First index is column (x), second is row (y)
- Matches world coordinate convention
================================================================================
]]--

local TetrisConstants = {
    _contract = {
        role = "constants"
    }
}

-- ----------------------------------------------------------------------------
-- GRID CONFIGURATION
-- ----------------------------------------------------------------------------
-- Standard Tetris playfield dimensions (official guideline).
-- Grid is 0-indexed: columns [0, 9], rows [0, 19].
TetrisConstants.GRID_WIDTH = 10
TetrisConstants.GRID_HEIGHT = 20

-- Actual rendered size of cube meshes in world units.
-- The cube.obj model has vertices from -1 to +1, making it 2x2x2 units at default scale.
TetrisConstants.CUBE_SIZE = 2

-- Sentinel value indicating an empty grid cell (no tetrimino block present).
TetrisConstants.GRID_EMPTY_CELL = -1

-- ----------------------------------------------------------------------------
-- SPAWN CONFIGURATION
-- ----------------------------------------------------------------------------
-- Tetriminos spawn near the top-center of the grid.
-- Horizontal: Center column minus 1 (accounts for 4-wide I-tetrimino).
-- Vertical: 4 rows from top (standard Tetris spawn height, allows visibility).
TetrisConstants.SPAWN_COLUMN = math.floor(TetrisConstants.GRID_WIDTH / 2) - 2  -- Evaluates to 3 for width=10
TetrisConstants.SPAWN_ROW = TetrisConstants.GRID_HEIGHT - 4                    -- Evaluates to 16 for height=20

-- ----------------------------------------------------------------------------
-- RENDERING CONFIGURATION
-- ----------------------------------------------------------------------------
-- Border color
TetrisConstants.BORDER_COLOR = vec3(1.0, 0.0, 0.8)

-- Camera field of view in degrees.
TetrisConstants.CAMERA_FOV_DEGREES = 45

-- Extra padding around grid dimensions for camera framing calculation.
TetrisConstants.CAMERA_PADDING = 2

-- Z-axis offset to position camera away from grid plane.
TetrisConstants.CAMERA_Z_OFFSET = 1

-- ----------------------------------------------------------------------------
-- GAMEPLAY CONFIGURATION
-- ----------------------------------------------------------------------------
-- Legacy fixed speed (deprecated - use GetGravitySpeed instead)
TetrisConstants.MOVE_SPEED_MS = 200

-- NES-inspired gravity curve (milliseconds per cell drop)
TetrisConstants.GRAVITY_TABLE = {
    [1]  = 800,  [2]  = 717,  [3]  = 633,  [4]  = 550,  [5]  = 467,
    [6]  = 383,  [7]  = 300,  [8]  = 217,  [9]  = 133,  [10] = 100,
    [11] = 83,   [12] = 83,   [13] = 67,   [14] = 67,   [15] = 50,
    [16] = 50,   [17] = 33,   [18] = 33,   [19] = 17,   [20] = 17,
}

function TetrisConstants.GetGravitySpeed(level)
    if level >= 20 then return 17 end
    return TetrisConstants.GRAVITY_TABLE[level] or 800
end

-- Lock delay constants
TetrisConstants.LOCK_DELAY_MS = 500         -- Grace period after landing (ms)
TetrisConstants.LOCK_DELAY_MAX_RESETS = 15  -- Max move/rotate resets while grounded

-- DAS (Delayed Auto Shift) constants
TetrisConstants.DAS_DELAY_MS = 170          -- Initial delay before auto-repeat (ms)
TetrisConstants.DAS_ARR_MS = 50             -- Auto-repeat rate once DAS charges (ms)

-- Rounding offset for converting floating-point positions to integer grid indices.
-- Using 0.5 ensures proper rounding (e.g., 2.3 + 0.5 = 2.8 → floor → 2).
TetrisConstants.GRID_POSITION_ROUNDING_OFFSET = 0.5

-- ----------------------------------------------------------------------------
-- ROTATION MATRIX CONFIGURATION
-- ----------------------------------------------------------------------------
-- Tetriminos use 4×4 rotation matrices (matching C++ implementation).
-- Valid indices: 0 to ROTATION_MATRIX_SIZE - 1 (i.e., 0 to 3).
TetrisConstants.ROTATION_MATRIX_SIZE = 4

-- ----------------------------------------------------------------------------
-- TETRIMINO-SPECIFIC CONFIGURATION
-- ----------------------------------------------------------------------------
-- Tetrimino cube spacing within the 4x4 matrix (matches C++ spacingX/Y).
TetrisConstants.TETRIMINO_SPACING = 2.0

-- Rotation angles in degrees
TetrisConstants.ROTATION_ANGLE_CW = 90.0
TetrisConstants.ROTATION_ANGLE_CCW = -90.0

-- Euler axis for tetrimino rotation (yaw/Z axis - rotates in XY plane)
TetrisConstants.ROTATION_AXIS = vec3(0, 0, 1)

-- ----------------------------------------------------------------------------
-- SRS WALL KICK DATA
-- ----------------------------------------------------------------------------
-- Rotation states: 0=spawn, 1=CW, 2=180, 3=CCW
-- Format: KICKS[fromState .. ">" .. toState] = {{dx, dy}, ...}
-- Offsets are in grid units (positive X = right, positive Y = up)

-- Wall kicks for J, L, S, T, Z pieces
TetrisConstants.SRS_KICKS_JLSTZ = {
    ["0>1"] = {{0,0}, {-1,0}, {-1,1},  {0,-2}, {-1,-2}},
    ["1>0"] = {{0,0}, {1,0},  {1,-1},  {0,2},  {1,2}},
    ["1>2"] = {{0,0}, {1,0},  {1,-1},  {0,2},  {1,2}},
    ["2>1"] = {{0,0}, {-1,0}, {-1,1},  {0,-2}, {-1,-2}},
    ["2>3"] = {{0,0}, {1,0},  {1,1},   {0,-2}, {1,-2}},
    ["3>2"] = {{0,0}, {-1,0}, {-1,-1}, {0,2},  {-1,2}},
    ["3>0"] = {{0,0}, {-1,0}, {-1,-1}, {0,2},  {-1,2}},
    ["0>3"] = {{0,0}, {1,0},  {1,1},   {0,-2}, {1,-2}},
}

-- Wall kicks for I piece (different offset table)
TetrisConstants.SRS_KICKS_I = {
    ["0>1"] = {{0,0}, {-2,0}, {1,0},  {-2,-1}, {1,2}},
    ["1>0"] = {{0,0}, {2,0},  {-1,0}, {2,1},   {-1,-2}},
    ["1>2"] = {{0,0}, {-1,0}, {2,0},  {-1,2},  {2,-1}},
    ["2>1"] = {{0,0}, {1,0},  {-2,0}, {1,-2},  {-2,1}},
    ["2>3"] = {{0,0}, {2,0},  {-1,0}, {2,1},   {-1,-2}},
    ["3>2"] = {{0,0}, {-2,0}, {1,0},  {-2,-1}, {1,2}},
    ["3>0"] = {{0,0}, {1,0},  {-2,0}, {1,-2},  {-2,1}},
    ["0>3"] = {{0,0}, {-1,0}, {2,0},  {-1,2},  {2,-1}},
}

return TetrisConstants
