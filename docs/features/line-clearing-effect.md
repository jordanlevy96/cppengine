# Line Clearing Visual Effect - Implementation Plan

## Overview

Add a Tetris Effect-style visual effect when lines are cleared: blocks pulse through colors (green → yellow → white) over 400ms before disappearing and the grid collapses. Architecture designed to support adding particle effects later.

**Current behavior**: Lines disappear instantly (same frame)
**Target behavior**: Color pulse effect → grid collapse → spawn new piece
**Future enhancement**: Particle explosion during color pulse

---

## Design Summary

### Immediate Implementation: Color Pulse Effect

**Visual sequence (400ms total)**:
- 0-100ms: Original color → Green `(0, 0.8, 0)`
- 100-200ms: Green → Yellow `(0.8, 0.8, 0)`
- 200-300ms: Yellow → White `(1, 1, 1)`
- 300-400ms: Hold white (flash)
- 400ms: Destroy blocks, collapse grid, update score

**Key architectural decisions**:
- All changes in Lua (no C++ modifications needed)
- State machine approach: track clearing in progress, prevent new spawns during effect
- Manual color updates (no Tween components - simpler for multi-stage transitions)
- Original destroy/collapse logic moved to separate function, called after effect

### Future Enhancement: Particle System

**When ready** (estimated 10-15 hours):
- New `ParticleEmitter` component with particle pool
- New `ParticleSystem` for spawn/update/physics
- Point sprite shader for efficient rendering
- Lua bindings: `SpawnLineClearParticles(position, color)`
- Can be added without modifying color pulse code

---

## Implementation Steps

### Step 1: Add State Management (30 min)

**File**: `res/scripts/TetrisGrid.lua`

Add clearing state to TetrisGrid table (~line 42):

```lua
TetrisGrid = {
    -- ... existing fields ...

    -- Line clearing effect state
    clearingState = {
        isClearing = false,        -- Effect in progress?
        clearedLines = {},         -- Line numbers being cleared
        affectedBlocks = {},       -- {entityID = originalColor}
        elapsedTime = 0,           -- Milliseconds since effect started
        duration = 400             -- Total effect duration (ms)
    }
}
```

### Step 2: Add Effect Update Function (45 min)

**File**: `res/scripts/TetrisGrid.lua`

Add new function (after `clearLines()`):

```lua
updateClearingEffect = function(self, delta)
    local state = self.clearingState
    state.elapsedTime = state.elapsedTime + (delta * 1000)  -- delta is in seconds

    -- Stage 1: 0-100ms (Original → Green) - already set by clearLines

    -- Stage 2: 100-200ms (Green → Yellow)
    if state.elapsedTime >= 100 and state.elapsedTime < 200 then
        for entityID, _ in pairs(state.affectedBlocks) do
            local transform = GetTransform(entityID)
            if transform then
                transform.Color = vec3(0.8, 0.8, 0)  -- Yellow
            end
        end
    end

    -- Stage 3: 200-300ms (Yellow → White)
    if state.elapsedTime >= 200 and state.elapsedTime < 300 then
        for entityID, _ in pairs(state.affectedBlocks) do
            local transform = GetTransform(entityID)
            if transform then
                transform.Color = vec3(1, 1, 1)  -- White flash
            end
        end
    end

    -- Stage 4: 400ms+ (Effect complete - destroy and collapse)
    if state.elapsedTime >= state.duration then
        self:finishClearingLines()
    end
end
```

**Note**: Color changes are instant at stage boundaries (arcade-style). Can add smooth interpolation later if desired.

### Step 3: Add Finish Function (45 min)

**File**: `res/scripts/TetrisGrid.lua`

Move the original destroy/collapse logic to a new function:

```lua
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

    UpdateGameUI(self.score, self.lines, self.level, self.nextPieceType)

    -- 4. Reset clearing state
    state.isClearing = false
    state.clearedLines = {}
    state.affectedBlocks = {}
    state.elapsedTime = 0
end
```

### Step 4: Modify clearLines() to Initiate Effect (45 min)

**File**: `res/scripts/TetrisGrid.lua` (lines 236-263)

Replace the current `clearLines()` implementation:

```lua
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
                state.affectedBlocks[entity] = vec3(transform.Color.x, transform.Color.y, transform.Color.z)

                -- Apply first color (green)
                transform.Color = vec3(0, 0.8, 0)
            end
        end
    end

    -- Don't destroy/collapse yet - that happens in finishClearingLines()
end
```

### Step 5: Modify process() to Update Effect (30 min)

**File**: `res/scripts/TetrisGrid.lua` (~line 374)

Update the main process function to handle clearing state:

```lua
process = function(self, delta)
    -- Wait for game to start before spawning tetriminos
    if not GameStarted then
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

    -- ... rest of original process logic (tetrimino movement, spawning, etc.)
end
```

### Step 6: Testing and Polish (30 min)

**Test cases**:
1. **Single line clear**: Verify green → yellow → white → collapse sequence
2. **Tetris (4 lines)**: Verify all lines animate in sync, correct score
3. **Timing**: Use stopwatch - should be ~400ms from clear to collapse
4. **No new spawns**: Verify no piece spawns during effect
5. **Performance**: FPS should stay at 60 during effect

**Debug helpers** (add to `updateClearingEffect()` temporarily):
```lua
-- Print timing for verification
if state.elapsedTime % 100 < delta * 1000 then
    print(string.format("Clearing: %.0fms", state.elapsedTime))
end
```

**Edge cases to verify**:
- Game over doesn't trigger during effect
- Multiple rapid line clears (second waits for first to complete)
- Line clear near top of grid doesn't break spawn logic

---

## Critical Files

**Primary implementation** (all changes here):
- `/Users/jordan/dev/cppengine/res/scripts/TetrisGrid.lua` - Game logic and line clearing

**Reference files** (no changes needed):
- `/Users/jordan/dev/cppengine/res/scripts/TetrisConstants.lua` - Constants like CUBE_SIZE, GRID_WIDTH
- `/Users/jordan/dev/cppengine/include/components/Transform.h` - Color is vec3(r,g,b) in [0-1]
- `/Users/jordan/dev/cppengine/src/systems/TweenSystem.cpp` - Reference for timing patterns

---

## Future: Particle System Architecture

**When ready to add particles** (separate implementation):

### New Components
- `include/components/ParticleEmitter.h` - Particle pool and emitter config
- Each particle: position, velocity, color, lifetime, size

### New System
- `src/systems/ParticleSystem.cpp` - Spawn, update, physics, cleanup
- Updates particles every frame (velocity, gravity, lifetime)

### Rendering
- `res/shaders/ParticleShader.shader` - Point sprite rendering
- Integrate into `RenderSystem.cpp` - Add particle render pass

### Integration
- `src/controllers/ScriptManager.cpp` - Add Lua binding:
  ```cpp
  lua.set_function("SpawnLineClearParticles", [](vec3 pos, vec3 color) { ... });
  ```
- Call from `clearLines()` when initiating effect:
  ```lua
  for _, lineY in ipairs(linesToClear) do
      for x = 0, C.GRID_WIDTH - 1 do
          local entity = self.grid[x][lineY]
          local transform = GetTransform(entity)
          SpawnLineClearParticles(transform.Pos, transform.Color)
      end
  end
  ```

**Estimated effort**: 10-15 hours (component 30min, system 5hrs, shader 2hrs, rendering 3hrs, integration 1hr, polish 3hrs)

**Key advantage**: Can add particles without modifying color pulse code - just call spawn function at effect start.

---

## Verification Plan

### Manual Testing

Run the game and test:

1. **Clear 1 line**:
   - Watch blocks: should see green → yellow → white flash
   - Time it: ~400ms from clear to collapse
   - Check score: +40 points (level 1)

2. **Clear 4 lines (Tetris)**:
   - All 4 lines should pulse in sync
   - Check score: +1200 points (level 1)
   - Verify grid collapses correctly

3. **Rapid clears**:
   - Set up two consecutive line clears
   - Second should wait for first to complete
   - No visual glitches or overlapping effects

4. **Performance**:
   - Clear 4 lines (40 blocks animating)
   - FPS should stay at 60
   - No stuttering during color transitions

5. **Edge cases**:
   - Clear line at top of grid
   - Clear line when grid nearly full
   - Verify game over doesn't trigger mid-effect

### Success Criteria

- ✅ Color pulse visible and smooth
- ✅ Grid collapse happens after effect (not during)
- ✅ Score updates correctly after effect completes
- ✅ New piece spawns only after effect done
- ✅ 60 FPS maintained during effect
- ✅ No crashes or visual glitches

---

## Implementation Time Estimate

**Total: 3-4 hours** (conservative estimate)

- Step 1 (State management): 30 min
- Step 2 (Update function): 45 min
- Step 3 (Finish function): 45 min
- Step 4 (Modify clearLines): 45 min
- Step 5 (Modify process): 30 min
- Step 6 (Testing): 30-60 min

**Note**: First implementation might take longer due to debugging and tuning. Subsequent effect implementations will be much faster using this pattern.

---

## Design Rationale

### Why not use Tween components?

- Blocks get destroyed anyway - don't need full tween lifecycle
- Multi-stage transitions (3 colors) easier with manual timing
- Current Tween bindings hardcoded for position tweens
- Instant color changes at stage boundaries look arcade-style (snappy)

### Why 400ms duration?

- Based on Tetris Effect reference
- Visible feedback without breaking game flow rhythm
- At 60fps = 24 frames (plenty for visual clarity)
- Shorter (<200ms) = players miss it; Longer (>600ms) = feels sluggish

### Why state machine approach?

- Clean separation: effect logic vs game logic
- Easy to extend: sound effects, screen shake, combos
- Prevents edge cases: spawning during effect, game over during effect
- Enables queuing if multiple clears happen rapidly

---

## Extension Points

This architecture enables future features without modifying core code:

1. **Sound effects**: Play audio at each color stage
2. **Screen shake**: Camera wobble during effect
3. **Score popups**: Floating "+1200 TETRIS!" text
4. **Particle effects**: As described above
5. **Combo system**: Track consecutive clears, apply multiplier
6. **Visual variations**: Different colors for different line counts

All can be added by extending `updateClearingEffect()` or `finishClearingLines()` without touching the state machine logic.

---

## Status

**Implemented**: ✅ Complete (as of 2026-01-17)

All steps have been implemented and tested. The color pulse effect works as designed, with blocks transitioning through green → yellow → white before disappearing. Grid collapse and scoring work correctly after the effect completes.
