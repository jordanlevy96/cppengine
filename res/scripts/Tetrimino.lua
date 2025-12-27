-- Tetrimino
--
-- Object-oriented tetrimino class that encapsulates tetrimino state and behavior

-- TetriminoData and TetrisConstants are loaded globally by init.lua

-- ============================================================================
-- TETRIMINO CLASS
-- ============================================================================

Tetrimino = {}
Tetrimino.__index = Tetrimino

-- Constructor: Creates a new tetrimino instance
-- @param shapeKey: String key for the shape ('I', 'O', 'T', 'J', 'L', 'S', 'Z')
-- @param renderComponent: Render component to use for cubes
-- @param lightID: Entity ID of the light source
-- @return: New Tetrimino instance
function Tetrimino.new(shapeKey, renderComponent, lightID)
    local self = setmetatable({}, Tetrimino)

    local tetriminoData = TetriminoData[shapeKey]
    if not tetriminoData then
        error("Unknown tetrimino shape: " .. shapeKey)
    end

    -- Store shape type
    self.shapeKey = shapeKey

    -- Create parent entity
    self.entityID = RegisterEntity()

    -- Initialize child map (2D Lua table, 0-indexed)
    self.childMap = {}
    for i = 0, 3 do
        self.childMap[i] = {}
        for j = 0, 3 do
            self.childMap[i][j] = TetrisConstants.GRID_EMPTY_CELL
        end
    end

    -- Create child cube entities where shape has blocks
    for i = 0, 3 do
        for j = 0, 3 do
            if tetriminoData.shape[i + 1][j + 1] == 1 then  -- Lua arrays are 1-indexed
                -- Create cube entity
                local cubeID = RegisterEntity()
                self.childMap[i][j] = cubeID

                -- Set transform (position relative to parent, set color)
                local transform = GetTransform(cubeID)
                transform.Pos.x = j * TetrisConstants.TETRIMINO_SPACING
                transform.Pos.y = i * TetrisConstants.TETRIMINO_SPACING
                transform.Pos.z = 0
                transform.Color = tetriminoData.color

                -- Add render component
                RegisterRenderComponent(cubeID, renderComponent)

                -- Add lighting component
                RegisterLighting(cubeID, lightID)

                -- Establish hierarchy
                AddChild(self.entityID, cubeID)
            end
        end
    end

    -- Add tween component to parent
    CreateTweenComponent(self.entityID, 1000)

    return self
end

-- ============================================================================
-- POSITION & MOVEMENT
-- ============================================================================

-- Get current position of tetrimino
-- @return: vec2 with x, y coordinates
function Tetrimino:getPosition()
    local transform = GetTransform(self.entityID)
    return vec2(transform.Pos.x, transform.Pos.y)
end

-- Get grid position (rounded to integer)
-- @return: vec2 with integer grid coordinates
function Tetrimino:getGridPosition()
    local pos = self:getPosition()
    return vec2(
        math.floor(pos.x + TetrisConstants.GRID_POSITION_ROUNDING_OFFSET),
        math.floor(pos.y + TetrisConstants.GRID_POSITION_ROUNDING_OFFSET)
    )
end

-- Update all child positions to match parent position and childMap
function Tetrimino:updateChildPositions()
    -- Get parent position and snap to grid to avoid floating-point errors
    local parentTransform = GetTransform(self.entityID)
    local parentPos = parentTransform.Pos

    -- Snap parent to integer grid coordinates
    local snappedX = math.floor(parentPos.x + TetrisConstants.GRID_POSITION_ROUNDING_OFFSET)
    local snappedY = math.floor(parentPos.y + TetrisConstants.GRID_POSITION_ROUNDING_OFFSET)
    parentTransform.Pos.x = snappedX
    parentTransform.Pos.y = snappedY

    -- Update each child's position based on childMap using snapped parent position
    for i = 0, TetrisConstants.ROTATION_MATRIX_SIZE - 1 do
        for j = 0, TetrisConstants.ROTATION_MATRIX_SIZE - 1 do
            local childID = self.childMap[i][j]
            if childID ~= TetrisConstants.GRID_EMPTY_CELL then
                local transform = GetTransform(childID)
                transform.Pos.x = snappedX + (j * TetrisConstants.TETRIMINO_SPACING)
                transform.Pos.y = snappedY + (i * TetrisConstants.TETRIMINO_SPACING)
                transform.Pos.z = parentPos.z
            end
        end
    end
end

-- Move tetrimino instantly (no animation)
-- @param direction: vec2 with x, y offset
function Tetrimino:move(direction)
    -- Get tween component and update it
    local tween = GetTween(self.entityID)
    tween.Start.x = tween.Start.x + direction.x
    tween.Start.y = tween.Start.y + direction.y
    tween.End.x = tween.End.x + direction.x
    tween.End.y = tween.End.y + direction.y
    tween.isActive = false  -- Instant movement, no animation

    -- Translate parent entity immediately
    TranslateEntity(self.entityID, vec3(direction.x, direction.y, 0))

    -- Update all child positions to match parent's new position
    self:updateChildPositions()
end

-- Move tetrimino with animation
-- @param direction: vec3 with x, y, z offset
-- @param duration: Animation duration in milliseconds
function Tetrimino:tweenMove(direction, duration)
    local transform = GetTransform(self.entityID)
    local tween = GetTween(self.entityID)

    -- Snap current position to grid to avoid accumulating floating-point errors
    local startX = math.floor(transform.Pos.x + TetrisConstants.GRID_POSITION_ROUNDING_OFFSET)
    local startY = math.floor(transform.Pos.y + TetrisConstants.GRID_POSITION_ROUNDING_OFFSET)
    transform.Pos.x = startX
    transform.Pos.y = startY

    tween.elapsed = 0
    tween.Start = vec3(startX, startY, transform.Pos.z)
    tween.End = vec3(startX + direction.x,
                      startY + direction.y,
                      transform.Pos.z + direction.z)
    tween.Duration = duration
    tween.isActive = true
end

-- Check if tetrimino has finished moving
-- @return: true if movement is complete, false otherwise
function Tetrimino:isMovementFinished()
    local tween = GetTween(self.entityID)
    return not tween.isActive
end

-- ============================================================================
-- ROTATION
-- ============================================================================

-- Rotate the childMap 90 degrees clockwise
-- @return: New rotated childMap
function Tetrimino:turnMatrixCW()
    local rotated = {}
    for i = 0, 3 do
        rotated[i] = {}
        for j = 0, 3 do
            rotated[i][j] = TetrisConstants.GRID_EMPTY_CELL
        end
    end

    for i = 0, 3 do
        for j = 0, 3 do
            rotated[j][3 - i] = self.childMap[i][j]
        end
    end

    return rotated
end

-- Rotate the childMap 90 degrees counter-clockwise
-- @return: New rotated childMap
function Tetrimino:turnMatrixCCW()
    local rotated = {}
    for i = 0, 3 do
        rotated[i] = {}
        for j = 0, 3 do
            rotated[i][j] = TetrisConstants.GRID_EMPTY_CELL
        end
    end

    for i = 0, 3 do
        for j = 0, 3 do
            rotated[3 - j][i] = self.childMap[i][j]
        end
    end

    return rotated
end

-- Get the rotated childMap for a given rotation direction
-- @param rotation: Rotation direction (from Rotations enum)
-- @return: New rotated childMap
function Tetrimino:getRotatedChildMap(rotation)
    if rotation == Rotations.CW then
        return self:turnMatrixCW()
    elseif rotation == Rotations.CCW then
        return self:turnMatrixCCW()
    end
end

-- Rotate the tetrimino
-- @param rotation: Rotation direction (from Rotations enum)
function Tetrimino:rotate(rotation)
    -- Update the child map to new rotation
    local newChildMap = self:getRotatedChildMap(rotation)
    self.childMap = newChildMap

    -- Update child positions to match new rotation
    self:updateChildPositions()
end

-- ============================================================================
-- ACCESSORS
-- ============================================================================

-- Get the childMap
-- @return: 2D table representing the 4x4 rotation matrix
function Tetrimino:getChildMap()
    return self.childMap
end

-- Get the entity ID
-- @return: Parent entity ID
function Tetrimino:getEntityID()
    return self.entityID
end

return Tetrimino
