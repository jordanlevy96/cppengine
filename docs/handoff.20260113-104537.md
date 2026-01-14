# Developer Handoff: Web-Based UI Editor

**Generated**: 2026-01-13 10:45:37
**Branch**: `web-based-ui`
**Target**: Merge to `develop`

---

## 1. Problem Statement

The Imhotep game engine needs a functional scene editor built using the new HTML/CSS + Lua reactive UI system (replacing deprecated ImGui). The editor currently launches and renders but **is not interactive** - users cannot select entities, edit properties, or manipulate the scene.

**Why it matters**: Without a working editor, scene creation requires manual YAML editing. The web-based UI approach was chosen to leverage the existing ReactiveUI/HTMLRendererMT infrastructure.

**Trigger**: Feature branch `web-based-ui` needs to reach minimum viable functionality before merging.

---

## 2. Original Requirements

### Explicit

- Make the editor "feel complete enough to merge to develop"
- Editor must be functional (currently it is not)

### Discovered/Clarified

- **Phase 1 foundation is ~60% complete** - window launches, 3D viewport renders, basic layout exists
- **Blocking issue**: Input handlers not wired up (TODO in code)
- Need to normalize editor loop with game loop first
- Minimum viable = entity selection + basic inspector (read-only is acceptable)

### Implicit

- Follow existing patterns in `App.cpp` for main loop structure
- Leverage existing `@click` event binding syntax from ReactiveUI
- Must work with multi-threaded HTMLRendererMT architecture

---

## 3. Current Status

### Completed

- Editor application shell and initialization sequence
- Three-panel UI layout (Scene Hierarchy | Viewport | Inspector)
- FBO rendering of 3D scene to viewport via base64 PNG
- Scene tree population from Registry entities
- Dark theme styling (VS Code inspired)
- HiDPI/Retina display support
- Logger, window management, clean shutdown
- **[NEW] Normalized editor loop with game loop** (see below)
  - Created `FrameTiming` abstraction for unified timing across all loops
  - Standardized event polling order (all loops now poll events first)
  - Unified system update order: Input → Scripts → Tweens → Hierarchy → Render
  - Reduced code duplication in Game::RunFixedLoop(), Game::RunVariableLoop(), and Editor::Run()

### In Progress

- Wire up input handlers in Editor

### Not Started

1. Scene tree click-to-select functionality
2. Basic Inspector panel (Transform display)
3. Selection highlight in viewport

### Known Issues

- `src/editor/Editor.cpp:63` has `TODO: Get editor input handlers working`
- Viewport texture encoding is slow (~5-10ms/frame) - acceptable for Phase 1
- Magic numbers in HTML (1580x1185 viewport size hardcoded)

---

## 4. Technical Context

### Relevant Files

**Editor-specific:**

- `include/editor/Editor.h` - Main editor controller (updated with FrameTiming)
- `src/editor/Editor.cpp` - Main loop, UI loading, scene tree (uses FrameTiming; line 65 has input TODO)
- `include/editor/SceneViewport.h` - Viewport rendering
- `src/editor/SceneViewport.cpp` - FBO rendering, PNG encoding (complete)
- `src/editor/main.cpp` - Entry point

**UI Templates:**

- `res/ui/editor.html` - Layout template (needs `@click` events added)
- `res/ui/styles/editor.css` - Styling
- `res/ui/state/editor.lua` - Lua state (needs `selectedEntityId`)

**Frame Timing Abstraction (NEW):**

- `include/util/FrameTiming.h` - Unified timing for FIXED/VARIABLE/SIMPLE modes
- `src/util/FrameTiming.cpp` - Implementation
- **Modes**:
  - `FIXED`: Action games (Tetris) - fixed 60 FPS timestep
  - `VARIABLE`: Strategy games - adjustable speed with decoupled rendering
  - `SIMPLE`: Editor, free-running - delta-only timing

**Game Loop Files (refactored with FrameTiming):**

- `src/controllers/Game.cpp` - RunFixedLoop() and RunVariableLoop() now use FrameTiming
- `include/controllers/Game.h` - Game class definition

**Engine systems:**

- `src/controllers/EngineCore.cpp` - Subsystem initialization
- `src/systems/ReactiveUI.cpp` - Template parsing, directive handling, event binding
- `src/systems/HTMLRendererMT.cpp` - Multi-threaded UI rendering
- `src/controllers/WindowManager.cpp` - Input event registration

### Key Classes/Functions

**FrameTiming (NEW):**

- `FrameTiming::FrameTiming(mode, targetFPS, renderFPS)` - Constructor for timing mode
- `FrameTiming::Update()` - Call once per frame to update delta/accumulators
- `FrameTiming::GetDelta()` - Delta time in milliseconds
- `FrameTiming::ShouldUpdateFixedStep()` - Use in while loop for FIXED mode (returns true when timestep consumed)
- `FrameTiming::ShouldUpdateSimulation()` - Use in while loop for VARIABLE mode
- `FrameTiming::ShouldRenderFrame()` - Check if render should occur in VARIABLE mode
- `FrameTiming::SetSimulationMultiplier(float)` - Set speed for VARIABLE mode
- `FrameTiming::GetFrameElapsedMS()` - Elapsed since frame started (for sleep calculation)

**Editor:**

- `Editor::Initialize()` - Where input handlers should be registered
- `Editor::Run()` - Main loop tick (now uses FrameTiming)
- `Editor::UpdateSceneTree()` - Populates Lua state with entities
- `Editor::UpdateSystems(delta)` - Updates TweenSystem and HierarchySystem

**Other key classes:**

- `WindowManager::RegisterInputHandler()` - Input registration API (exists but unused by Editor)
- `ReactiveUI::GetRenderedHTML()` - Processes templates with directives
- `SceneViewport::Render()` - FBO rendering
- `SceneViewport::GetTextureAsDataURI()` - Texture to base64 conversion

### Architecture Notes

- **Multi-threading**: HTMLRendererMT runs litehtml on separate thread. Main thread only pays for texture upload (~2ms)
- **Data flow**: Registry → `UpdateSceneTree()` → Lua table → `v-for` template → Scene Hierarchy panel
- **Event binding syntax**: `@click="functionName"` in HTML triggers Lua/C++ handlers
- **Input routing**: Input → WindowManager callbacks → ScriptManager event queue → Lua (editor needs to intercept)
- **[NEW] Unified loop structure**: All loops now follow: Poll Events → Input → Scripts → Tweens → Hierarchy → Render
  - This ensures consistent behavior and reduces maintenance burden
  - FrameTiming encapsulates accumulator logic, allowing clean while loops
  - Game supports both FIXED (tight coupling) and VARIABLE (decoupled) modes
  - Editor uses SIMPLE mode (delta-only) for straightforward free-running

### Dependencies

- litehtml (HTML/CSS rendering)
- Lua + Sol2 (state management)
- GLFW (window/input)
- OpenGL 3.3 Core (rendering)
- FreeType (fonts)
- Quill (logging)

---

## 5. Validation Criteria

### Definition of Done

- [ ] Can click entity in Scene Hierarchy tree → entity becomes selected
- [ ] Inspector panel shows selected entity's Transform (position, rotation, scale)
- [ ] Selected entity is visually highlighted in 3D viewport
- [ ] Keyboard input works (ESC to quit, future: Ctrl+S, Ctrl+Z)
- [ ] Editor loop structure matches game loop patterns

### Manual Verification

1. Build: `cd build && make -j8`
2. Run: `./imhotep-editor`
3. Click on entity name in left panel → should highlight
4. Inspector (right panel) should show Transform values
5. 3D viewport should show visual indication of selection
6. Press ESC → editor should close cleanly

### Edge Cases

- Empty scene (no entities)
- Entity with no Transform component
- Very long entity names in tree
- Rapid selection changes

---

## 6. Constraints & Considerations

### Technical Constraints

- Must use existing `@click` event binding (don't reinvent)
- Input handlers must be registered via `WindowManager::RegisterInputHandler()`
- UI updates require marking Lua state dirty: `m_luaState->MarkDirty()`
- Run from `build/` directory (paths use `../res/` prefix)

### Performance Requirements

- Target 60 FPS (16.67ms per frame)
- Current main thread budget: ~7ms (plenty of headroom)
- Viewport PNG encoding acceptable at 5-10ms (async, doesn't block)

### Known Limitations to Document

- Inspector is read-only (no transform editing in Phase 1)
- No viewport picking (select via tree only)
- No undo/redo
- No gizmos
- No hot reload

---

## 7. Next Steps

### Ordered Task List

1. **[COMPLETED] Normalize editor loop with game loop**

   ✅ **Completed**: Created `FrameTiming` abstraction to unify timing logic

   **What was done:**
   - Created `util/FrameTiming.h` and `util/FrameTiming.cpp` with three timing modes:
     - `FIXED`: Fixed timestep with single accumulator (action games like Tetris)
     - `VARIABLE`: Dual accumulators for decoupled sim/render (strategy games)
     - `SIMPLE`: Delta-only for editor and simple free-running loops
   - Refactored `Game::RunFixedLoop()`: 43 → 30 lines, uses `FrameTiming::ShouldUpdateFixedStep()`
   - Refactored `Game::RunVariableLoop()`: 76 → 48 lines, uses `FrameTiming::ShouldUpdateSimulation()` and `ShouldRenderFrame()`
   - Refactored `Editor::Run()`: Now uses `FrameTiming(SIMPLE)` for consistency
   - **Standardized event polling order** across all loops: `glfwPollEvents()` now happens first (before simulation)
   - **Unified system update order**: Input → Scripts → Tweens → Hierarchy → Render
   - Both game modes (FIXED and VARIABLE) tested successfully
   - Editor tested and working

2. **Wire up input handlers in Editor** (~2-3 hrs)

   - Complete TODO at `Editor.cpp:63`
   - Call `WindowManager::RegisterInputHandler()` in `Editor::Initialize()`
   - Implement keyboard shortcut dispatch (ESC works, add Ctrl+S, etc.)

3. **Implement scene tree click-to-select** (~3-4 hrs)

   - Add `@click="selectEntity(entity.id)"` to entity items in `editor.html`
   - Add `selectedEntityId` to `editor.lua` state
   - Add CSS class binding for selection highlight: `v-bind:class="..."`
   - Implement `selectEntity()` handler in C++ or Lua

4. **Build basic Inspector panel** (~2-3 hrs)

   - Update `editor.html` inspector section with `v-if="selectedEntityId"`
   - Display entity name and Transform component values
   - Read position/rotation/scale from selected entity's Transform
   - Update when selection changes

5. **Add selection highlight in viewport** (~2 hrs)
   - Track selected entity ID in C++
   - Modify render pass to highlight selected entity (color tint or outline)
   - Update highlight when selection changes

### Blockers

- None identified - all infrastructure exists

---

## 8. Open Questions

- **Selection highlight approach**: Color tint vs outline shader vs bounding box? (Color tint is simplest)
- **Inspector data binding**: Push from C++ to Lua, or have Lua query C++ on demand?
- **Future phases**: Should Phase 2 items (camera control, transform editing) be separate PRs?
- **FrameTiming edge case**: UNCAPPED mode in VARIABLE loop - should there be a frame cap? Currently limited to 100 updates/frame for safety.

---

## 9. Useful Commands

```bash
# Build
cd /Users/jordan/dev/cppengine/build && make -j8

# Run editor
./imhotep-editor

# Run game (for comparison)
./imhotep

# Full rebuild (if needed)
rm -rf build && mkdir build && cd build && cmake .. && make -j8

# Check recent commits on branch
git log --oneline -10

# View architecture docs
cat docs/architecture/EDITOR_ARCHITECTURE.md
cat docs/architecture/EDITOR_PHASE1_NOTES.md
cat docs/architecture/UI_SYSTEM.md
```

---

## Key Documentation References

| Document                                   | Purpose                                      |
| ------------------------------------------ | -------------------------------------------- |
| `docs/architecture/EDITOR_ARCHITECTURE.md` | Full Phase 1-6 roadmap, anti-patterns        |
| `docs/architecture/EDITOR_PHASE1_NOTES.md` | Critical fixes applied, known limitations    |
| `docs/architecture/UI_SYSTEM.md`           | Reactive UI directives, event binding syntax |
| `CLAUDE.md`                                | Project context and conventions              |

---

_Handoff generated by Claude Code_
