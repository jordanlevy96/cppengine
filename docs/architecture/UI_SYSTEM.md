# Declarative UI System - Architecture & Implementation

**Last Updated**: January 8, 2026

## Vision

Build a web-based declarative UI system for complex, data-driven game interfaces (Paradox Interactive-style).

---

## Architecture Overview

### High-Level Flow

```
┌─────────────────────────────────┐
│  Game State (C++)               │  ← Your game logic
│  - Entity data                  │
│  - Game rules                   │
└─────────────────────────────────┘
         ↓ (expose via Lua bindings)
┌─────────────────────────────────┐
│  UI State Layer (Lua)           │  ← Reactive state management
│  - data = { ... }               │
│  - computed = { ... }           │
│  - methods = { ... }            │
└─────────────────────────────────┘
         ↓ (bind to templates)
┌─────────────────────────────────┐
│  Templates (HTML + Directives)  │  ← Vue/Jomini-style templates
│  - {{ interpolation }}          │
│  - v-if, v-for, v-show          │
│  - @click, @hover               │
│  - CSS styling                  │
└─────────────────────────────────┘
         ↓ (parse & compile)
┌─────────────────────────────────┐
│  Template Compiler (C++)        │  ← Converts templates to render tree
│  - Parse directives             │
│  - Build dependency graph       │
│  - Optimize for diffing         │
└─────────────────────────────────┘
         ↓ (render only what changed)
┌─────────────────────────────────┐
│  Renderer (HTMLRendererMT)      │  ← litehtml + FreeType + OpenGL
│  - Multi-threaded rendering     │
│  - Font caching                 │
│  - Dirty region rendering       │
└─────────────────────────────────┘
```

### Multi-Threaded Rendering Flow

```
Main Thread (Game Loop)        Render Thread (HTMLRendererMT)
───────────────────────        ──────────────────────────────
Game::RunFixedLoop()           RenderThreadLoop() [running async]
  │                                 │
  ├─ Game::Render()                 │
  │   │                             │
  │   ├─ RenderSystem::Update()     │
  │   │                             │
  │   ├─ htmlRenderer->Render()     │
  │   │   ├─ Lock m_bufferMutex     │
  │   │   ├─ Check frameNumber      │
  │   │   ├─ Upload GL texture ─────┼─ (if new frame)
  │   │   └─ Unlock                 │
  │   │                             │
  │   └─ Composite overlay          │
  │                                 │
  ├─ Game::TrackFPS()               │
  │   └─ luaState->SetValue()       │
  │       └─ Marks dirty flag       │
  │                                 │
  ├─ reactiveUI.GetRenderedHTML()   │
  │   ├─ Check IsDirty()            │
  │   ├─ RenderWithLua() ───────────┼─ (if dirty)
  │   │   ├─ parser->Evaluate()     │
  │   │   └─ Lock m_mutex           │
  │   │       └─ UpdateHTML() ──────┼─ Signals m_cv
  │   └─ ClearDirty()               │
  │                                 │
  └─ glfwPollEvents()               ├─ m_cv.wait() wakes up
                                    ├─ Render to m_backBuffer
                                    │   ├─ litehtml layout
                                    │   ├─ FreeType rasterization
                                    │   └─ Software rendering
                                    ├─ Lock m_bufferMutex
                                    ├─ std::swap(m_frontBuffer, m_backBuffer)
                                    ├─ Increment frameNumber
                                    └─ Unlock (back to wait)
```

**Why Multi-Threading?** HTML rendering can take 5-15ms, causing frame drops at 60 FPS. IPC would introduce too much overhead. Solution: Render HTML on background thread, swap buffers when ready, composite on main thread. Main loop never blocks.

---

## System Architecture (EngineCore + Game)

### Initialization Flow

The engine was refactored (Jan 2026) to use **EngineCore** for common initialization shared between Game and Editor:

```
Game::Initialize()
  └─► EngineCore::Initialize()
      ├─► InitializeLogger()      - Quill logging system
      ├─► InitializeWindow()      - GLFW window + OpenGL context
      ├─► InitializeHTMLRenderer() - HTMLRendererMT (starts render thread)
      ├─► InitializeScriptManager() - Lua + Python VMs
      ├─► InitializeRegistry()    - ECS system
      └─► InitializeUI()          - Loads Lua state + HTML template
          ├─► LuaUIState::LoadStateFile()
          ├─► ReactiveUI::BindLuaState()
          ├─► ReactiveUI::LoadTemplateFromFiles()
          └─► ReactiveUI::RegisterTemplateWithDirectives()

Game::Run()
  └─► Game::RunFixedLoop() or RunVariableLoop()
      ├─► Game::Render()          - 3D scene + UI overlay
      ├─► Game::TrackFPS()        - Updates Lua state with metrics
      └─► glfwPollEvents()        - CRITICAL: macOS needs this every frame!
```

**Key files:**
- `include/controllers/EngineCore.h` + `src/controllers/EngineCore.cpp` - Common init
- `include/controllers/Game.h` + `src/controllers/Game.cpp` - Game loop
- `src/controllers/Game.cpp:79-118` - RunFixedLoop() main game loop
- `src/controllers/Game.cpp:210-217` - Render() orchestration
- `src/controllers/Game.cpp:220-295` - TrackFPS() updates Lua state

---

## Core Components

### ReactiveUI (`include/systems/ReactiveUI.h`)

Template engine with reactive state management.

**Responsibilities**:
- Load HTML templates from files (`LoadTemplateFromFile()`)
- Integrate with LuaUIState for data binding
- Delegate template parsing to TemplateParser
- Change detection (dirty flag on Lua state changes)
- Event dispatching to Lua methods
- Cache rendered HTML to avoid re-rendering

**Key Methods**:
- `GetRenderedHTML()` - Returns cached HTML or re-renders if dirty
- `DispatchEvent(elementId, eventType, eventData)` - Calls Lua event handlers
- `SetEventHandlers()` / `GetEventHandlers()` - Thread-safe handler sync

**Event Handling Flow**:
1. User clicks UI element
2. GLFW → WindowManager → Lua input handler
3. Lua calls `htmlRenderer:HandleClickEvent(x, y, button)`
4. HTMLRendererMT hit-tests interactive elements
5. ReactiveUI dispatches event to Lua method
6. Lua method executes, state marked dirty
7. Next frame re-renders with updated state

### TemplateParser (`include/systems/TemplateParser.h`)

Parses Vue.js-style directives from HTML templates.

**Supported Directives**:
- `{{ variable }}` - Interpolation (outputs Lua state values)
- `v-if="condition"` - Conditional rendering
- `v-for="item in items"` - List rendering
- `@click="handler"` - Event binding (click, mouseover, etc.)

**Process**:
1. Parse HTML and extract directives
2. Evaluate Lua expressions for v-if conditions
3. Iterate Lua tables for v-for loops
4. Replace {{ }} interpolations with actual values
5. Extract @event handlers and assign data-event-id attributes
6. Return processed HTML + event handler map

**Example**:
```html
<div v-if="showPanel">
  <button @click="onStart">{{ buttonText }}</button>
  <div v-for="item in items">{{ item.name }}</div>
</div>
```

### LuaUIState (`include/systems/LuaUIState.h`)

Lua state file loader and dirty flag manager.

**Lua State File Format**:
```lua
-- res/ui/state/my_screen.lua
return {
    data = {
        title = "My Screen",
        items = {{name = "Item 1"}, {name = "Item 2"}}
    },
    methods = {
        onClick = function(self)
            print("Clicked!")
        end
    }
}
```

**Responsibilities**:
- Load .lua state files
- Expose `data` table to TemplateParser
- Expose `methods` table to ReactiveUI for event handling
- Maintain dirty flag for change detection
- Provide `MarkDirty()` for triggering re-renders

**✅ Change Detection Optimization:**

`LuaUIState::SetValue()` implements intelligent change detection:

```cpp
// include/systems/LuaUIState.h:190-263
template<typename T>
void LuaUIState::SetValue(const std::string& key, const T& value) {
    // Get current value from Lua state
    sol::object currentValue = GetValue(key);

    // Compare with new value (type-aware comparison)
    bool hasChanged = /* compare currentValue with value */;

    if (hasChanged) {
        // Only update and mark dirty if value actually changed
        parentTable[finalKey] = value;
        m_isDirty = true;
    }
    // If unchanged, skip update and keep dirty flag clean ✅
}
```

**Benefits:**
- `Game::TrackFPS()` calls `SetValue("data.fps", 60)` every second
- When FPS is stable at 60, value comparison prevents dirty flag
- **No unnecessary re-renders** when values unchanged
- Example: FPS stable at 60 for 10 seconds = 0 re-renders ✅

**Type-aware comparison:**
- **int/double**: Numeric equality with type coercion (60 == 60.0)
- **string**: Direct string comparison
- **bool**: Direct boolean comparison
- **tables/other**: Conservative (always marks changed for safety)

**Performance impact:**
- Eliminates 12-15 unnecessary HTML re-renders per second
- Reduces HTMLRendererMT wake-ups from ~12/sec to ~1/sec (only on actual changes)
- Main thread no longer blocked by redundant template parsing

### HTMLRendererMT (`include/systems/HTMLRendererMT.h`)

Multi-threaded HTML/CSS renderer using litehtml + FreeType.

**Key Architecture**: Background render thread to prevent blocking main game loop.

#### Double Buffering

Prevents race conditions between threads:

```cpp
struct FrameBuffer {
    uint32_t width;
    uint32_t height;
    uint32_t frameNumber;   // Incremented on each render
    std::vector<uint8_t> pixels;  // RGBA format
};

FrameBuffer m_frontBuffer;  // Read by main thread
FrameBuffer m_backBuffer;   // Written by render thread
```

**Flow**:
1. Render thread writes to `m_backBuffer`
2. When complete, lock `m_bufferMutex` and `std::swap(m_frontBuffer, m_backBuffer)`
3. Increment `m_frontBuffer.frameNumber`
4. Main thread detects new frame number, uploads to GL texture

#### Thread Synchronization

- `m_mutex` - Protects HTML string and resize requests
- `m_bufferMutex` - Protects buffer swap (short critical section)
- `m_cv` - Wakes render thread on new HTML/resize
- `m_running` - Atomic shutdown flag

#### Render Thread Lifecycle

**Pattern**: Wait for work → render HTML → swap buffers → repeat

1. `m_cv.wait()` blocks until new HTML or resize
2. Render to `m_backBuffer` (outside critical section)
3. Swap buffers under `m_bufferMutex`
4. Increment `frameNumber` to signal main thread

See `HTMLRendererMT::RenderThreadLoop()` (src/systems/HTMLRendererMT.cpp:790) for full implementation.

#### SoftwareRenderer (Inner Class)

Runs on render thread, implements litehtml `document_container`:

- **Font management**: FreeType + glyph cache
- **Text rasterization**: Alpha blending
  - Uses `FT_LOAD_NO_HINTING` to prevent glyph corruption
  - Row-by-row bitmap copy to handle pitch correctly
  - Supports negative pitch for bottom-up bitmaps
- **Layout**: litehtml HTML/CSS parsing
- **Software rendering**: Renders to pixel buffer

**Why not OpenGL on render thread?** Context sharing is complex; FreeType + pixel blitting is fast enough for UI.

#### Event Handling

HTMLRendererMT also handles UI event hit-testing:

- **HandleClickEvent(x, y, button)** - Hit-tests interactive elements and dispatches events
- **UpdateHoverState(x, y)** - Updates hover state for UI elements
- **SetEventHandlers()** / **GetEventHandlers()** - Thread-safe handler synchronization
- **InteractiveElement** struct - Tracks clickable regions with bounding boxes

Event handlers are double-buffered like pixel data to prevent race conditions during UI updates.

#### Input Event Architecture

The engine uses a three-tiered input dispatch system with LIFO priority:

**1. C++ Input Handlers (Highest Priority - LIFO)**
- Registered via `WindowManager::RegisterInputHandler()`
- Handlers called in **reverse registration order** (last registered = highest priority)
- Used by EngineCore to detect UI clicks, Editor to intercept input
- Return `true` to consume event (stops propagation), `false` to pass through

**2. Lua Event Queue (Medium Priority)**
- Events not consumed by C++ handlers go to `ScriptManager::AddInputEventToQueue()`
- Queued as Lua tables to `EventQueue` global
- Processed by `HandleInput()` in game scripts (e.g., TetrisInput.lua)
- Used for game logic input (player movement, camera controls)

**3. UI Event Handlers (UI-Specific)**
- Triggered when C++ handlers detect clicks on UI elements
- Dispatched via `ReactiveUI::DispatchEvent()`
- Executes Lua methods bound in templates (`@click="methodName"`)
- Used for button clicks, form inputs, UI interactions

**Priority Example**:
```cpp
// Register in this order:
id1 = RegisterInputHandler(gameHandler);   // Called 3rd (lowest priority)
id2 = RegisterInputHandler(uiHandler);     // Called 2nd
id3 = RegisterInputHandler(editorHandler); // Called 1st (highest priority)
```

**Event Flow**:
```
GLFW Event → WindowManager callback
    ↓
[1] Try C++ handlers (REVERSE order, LIFO)
    ├─→ Handler returns true → Event consumed (STOP)
    └─→ All return false → Continue to [2]
    ↓
[2] ScriptManager::AddInputEventToQueue()
    ├─→ UI handler detects hit → ReactiveUI::DispatchEvent() [3]
    └─→ Otherwise → Lua HandleInput() processes queue
    ↓
[3] ReactiveUI executes Lua method from template
```

**Important Notes**:
- `MouseButton` events only go to C++ handlers, not Lua queue (for @mousedown/@mouseup)
- Once consumed by C++ handler, Lua never sees the event
- LIFO allows UI/editor to intercept before game logic

**See**: `include/util/InputEvent.h` for comprehensive architecture documentation

#### Thread Safety

**Safe operations**:
- Main thread: reads `m_frontBuffer` (with `m_bufferMutex`), GL operations
- Render thread: writes `m_backBuffer`, FreeType operations

**Critical**: Never touch `m_backBuffer` from main thread. Never OpenGL from render thread.

**Pattern for texture upload**:
```cpp
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    if (m_frontBuffer.frameNumber != m_lastFrameNumber) {
        glTexSubImage2D(..., m_frontBuffer.pixels.data());  // ✅ Safe
        m_lastFrameNumber = m_frontBuffer.frameNumber;
    }
}
```

---

## Implementation Phases

### Phase 1: Foundation

**Goal**: Basic reactive templating with change detection

**Implemented**:
- `ReactiveUI` - Template engine with change detection
- `HTMLRendererMT` - Multi-threaded rendering + font caching
- `TemplateParser` - Parses v-if, v-for, {{ }} directives
- `LuaUIState` - Loads UI state from .lua files
- External HTML/CSS files (res/ui/templates/, res/ui/styles/)
- Tetris UI - Working example with all Phase 1 features

**Example** (Working in Tetris UI):

```lua
-- res/ui/state/character_list.lua
return {
    data = {
        characters = {
            {name = "King John", age = 45},
            {name = "Duke Peter", age = 32}
        },
        showList = true
    }
}
```

```html
<!-- res/ui/templates/character_list.html -->
<div v-if="showList">
  <h2>Characters</h2>
  <div v-for="char in characters">{{ char.name }} (Age: {{ char.age }})</div>
</div>
```

### Phase 2: Interactivity

**Goal**: User interaction and dynamic updates

**Implemented Features**:

1. **Event Handling**
   - `@click` - Click handlers with Lua event methods
   - `@mouseover` - Mouse hover support (infrastructure ready)
   - Event handler extraction and dispatching via TemplateParser
   - Full event flow: GLFW → WindowManager → Lua → HTMLRendererMT → ReactiveUI
   - Event objects with coordinates, button data, element ID
   - Auto-mark state dirty after event execution

2. **Lua Event Methods**
   - methods table in Lua state files
   - Event handler expressions: "method", "method(arg)", "method($event)"
   - Working example in Tetris UI (onStartGame, onRestart, onMainMenu)

**Example** (Working in Tetris UI):

```lua
-- res/ui/state/fps.lua
return {
    data = { gameState = "menu" },
    methods = {
        onStartGame = function(self)
            GameManager:StartGame()
        end,
        onRestart = function(self)
            GameManager:RestartGame()
        end
    }
}
```

```html
<!-- res/ui/templates/tetris.html -->
<button @click="onStartGame">START GAME</button>
<button @click="onRestart">RESTART</button>
```

**Additional Features**:

1. **Advanced Event Handling**
   - Event propagation and bubbling
   - Event modifiers (@click.stop, @click.prevent)
   - More event types (focus, blur, keydown, etc.)

2. **Two-Way Binding**
   - `v-model` - Form inputs
   - Sync C++ game state ↔ Lua ↔ UI

3. **Tooltip System**
   - `v-tooltip` - Rich nested tooltips (critical for PDX games)
   - Lazy evaluation (only compute when shown)

### Phase 3: Performance & Scale

**Goal**: Handle complex UIs with thousands of elements

**Features**:
- Virtual DOM / diffing algorithm
- List virtualization (render only visible items)
- Lazy rendering for off-screen content
- Dirty rectangle rendering

**Target**: 10,000+ UI elements at 60 FPS

### Phase 4: Modding & Polish

**Goal**: Production-ready for game release

**Features**:
- Component system (reusable UI widgets)
- Modding support (hot reload, template overrides)
- Developer tools (UI inspector, state debugger)
- Advanced CSS (animations, transitions)

---

## Technical Decisions

### Why Lua (not Python/V8)?

- **Fast**: ~10x faster than Python, crucial for real-time
- **Small**: ~200KB vs Python's ~10MB, V8's 20MB
- **Game-standard**: Industry proven for scripting
- **Already integrated**: Sol2 working and mature

### Why HTML/CSS (not custom format)?

- **Familiar**: Web developers can contribute
- **Proven**: litehtml is mature and tested
- **Styling**: CSS is powerful and well-understood
- **Tooling**: Editors, formatters exist

### Why Templates (not Immediate Mode)?

- **Data-driven**: PDX games are all about data
- **Declarative**: Easier to reason about complex UIs
- **Modding**: Text files are easy to mod
- **Designer-friendly**: No C++ required

### Why Multi-Threading (not IPC)?

- **Simpler**: Shared memory, no serialization
- **Faster**: No process context switches
- **Sufficient**: UI rendering doesn't need full isolation
- **Easier debugging**: Single process, standard tools

### Change Detection Strategy

- **Current**: Dirty checking (compare values on update - simple, works)
- **Future**: Proxy-based reactivity (Vue 3 style)
- **Future**: Fine-grained reactivity (Solid.js style)

---

## Performance

### Benchmarks (M1 Mac, Jan 2026)

- **HTML rendering**: 5-15ms (on render thread, doesn't block!)
- **Texture upload**: 1-2ms (main thread cost)
- **UI composite**: <1ms
- **Total main thread impact**: ~2ms per frame

### Optimizations

- **ReactiveUI dirty flag**: Prevents re-renders unless Lua state changes
- **Glyph cache**: FreeType glyphs cached on render thread
- **Double buffering**: Zero-copy buffer swap (std::swap)
- **Frame number tracking**: Only upload texture when buffer changes

**Result**: 60 FPS maintained even with complex HTML/CSS UIs.

---

## Detailed Process Flows

### Complete Render Process (Frame-by-Frame)

**Step 1: Game Loop Iteration** (`src/controllers/Game.cpp:79-118`)
```cpp
void Game::RunFixedLoop() {
    while (!windowManager->ShouldClose()) {
        // 1. Poll input events (CRITICAL for macOS window to appear!)
        glfwPollEvents();  // Line 92

        // 2. Render 3D scene + UI overlay
        Render();  // Line 107

        // 3. Update FPS counter and Lua state
        TrackFPS();  // Line 110

        // 4. Check if UI needs re-rendering
        std::string html = reactiveUI.GetRenderedHTML();  // Line 112
        if (/* html changed */) {
            htmlRenderer->UpdateHTML(html);  // Line 113
        }

        // 5. Swap OpenGL buffers
        glfwSwapBuffers(window);  // Line 115
    }
}
```

**Step 2: FPS Tracking** (`src/controllers/Game.cpp:220-295`)
```cpp
void Game::TrackFPS() {
    // Update every ~1 second
    if (m_fpsTime >= 1000.0) {
        // Calculate FPS metrics
        int currentFPS = (int)(m_frameCount / (m_fpsTime / 1000.0));

        // ✅ SetValue() with change detection (only marks dirty if value changed)
        luaState->SetValue("data.fps", currentFPS);        // Line 242
        luaState->SetValue("data.frameTime", ...);         // Line 243
        luaState->SetValue("data.gameMode", ...);          // Line 247
        // ... more SetValue() calls ...

        // Only triggers re-render if FPS/frameTime actually changed! ✅
    }
}
```

**Step 3: Reactive HTML Rendering** (`src/systems/ReactiveUI.cpp:16-31`)
```cpp
const std::string& ReactiveUI::GetRenderedHTML() {
    // Check if Lua state is dirty
    if (m_luaState && m_luaState->IsDirty()) {  // Line 19
        // Re-render template (expensive: 5-15ms)
        RenderWithLua();                        // Line 20
        m_luaState->ClearDirty();               // Line 21
    }
    return m_cachedHTML;  // Return cached HTML if clean
}

void ReactiveUI::RenderWithLua() {
    LOG_DEBUG("[ReactiveUI] Rendering template (Lua mode, dirty)");  // Line 98

    // Parse template with current Lua state
    m_cachedHTML = m_parser->Evaluate(*m_luaState);  // Line 101

    // Update event handlers for click/hover
    htmlRenderer.SetEventHandlers(m_parser->GetEventHandlers());  // Line 105
}
```

**Step 4: HTML Update to Render Thread** (`src/systems/HTMLRendererMT.cpp:933-1175`)
```cpp
void HTMLRendererMT::UpdateHTML(const std::string& html) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_htmlContent = html;
    m_cv.notify_one();  // Wake up render thread
}

// Render thread wakes up:
void HTMLRendererMT::RenderThreadLoop() {
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_running || m_needsRender; });

        // Render HTML to m_backBuffer (5-15ms)
        RenderToBuffer(m_backBuffer);

        // Swap buffers atomically
        {
            std::lock_guard<std::mutex> bufferLock(m_bufferMutex);
            std::swap(m_frontBuffer, m_backBuffer);
            m_frontBuffer.frameNumber++;  // Signal main thread
        }
    }
}
```

**Step 5: Texture Upload** (`src/systems/HTMLRendererMT.cpp:688-750`)
```cpp
void HTMLRendererMT::Render() {
    // Check if render thread produced new frame
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_frontBuffer.frameNumber != m_lastFrameNumber) {
            // Upload pixel buffer to OpenGL texture
            glTexSubImage2D(..., m_frontBuffer.pixels.data());
            m_lastFrameNumber = m_frontBuffer.frameNumber;
        }
    }

    // Composite UI overlay on screen
    RenderQuad();
}
```

### Complete Input/Event Handling Process

**Step 1: User Clicks UI Element** (User action)

**Step 2: GLFW Receives Click** (OS → GLFW callback)
```
WindowManager mouse callback registered via GLFW
```

**Step 3: Lua Input Handler** (`res/scripts/input.lua` or similar)
```lua
-- GLFW → WindowManager → Lua input handler
function HandleMouseClick(x, y, button)
    -- Forward to HTML renderer for hit-testing
    htmlRenderer:HandleClickEvent(x, y, button)
end
```

**Step 4: HTMLRendererMT Hit-Testing** (`src/systems/HTMLRendererMT.cpp`)
```cpp
void HTMLRendererMT::HandleClickEvent(float x, float y, int button) {
    // Hit-test interactive elements
    for (const auto& elem : m_interactiveElements) {
        if (elem.bounds.contains(x, y)) {
            // Found clicked element - get handler expression
            std::string handlerExpr = elem.eventHandlers["click"];

            // Dispatch to ReactiveUI
            ReactiveUI::GetInstance().DispatchEvent(
                "click", handlerExpr, {x, y, button, elem.id, "click"}
            );
            break;
        }
    }
}
```

**Step 5: ReactiveUI Event Dispatch** (`src/systems/ReactiveUI.cpp:111-216`)
```cpp
void ReactiveUI::DispatchEvent(const std::string& eventType,
                                const std::string& handlerExpr,
                                const EventData& eventData) {
    // Parse handler: "methodName" or "methodName(args)" or "methodName($event)"
    std::string handlerName = ParseHandlerName(handlerExpr);

    // Get Lua methods table
    sol::table methods = m_luaState->GetValue("methods").as<sol::table>();
    sol::function handler = methods[handlerName];

    // Call Lua handler function
    handler(m_luaState->GetStateTable(), /* args */);

    // Mark state dirty (triggers re-render next frame)
    m_luaState->MarkDirty();  // Line 209
}
```

**Step 6: Lua Handler Executes** (`res/ui/state/game.lua`)
```lua
methods = {
    onStartGame = function(self)
        GameManager:StartGame()  -- C++ binding
        self.data.gameStarted = true
        -- State marked dirty by DispatchEvent()
    end
}
```

**Step 7: Next Frame Re-renders UI**
```
Game::RunFixedLoop() → reactiveUI.GetRenderedHTML()
  → IsDirty() == true → RenderWithLua() → UpdateHTML() → Render thread
```

---

## Troubleshooting

### UI not updating
- **Check dirty flag**: `m_luaState->IsDirty()` should be true after SetValue()
- **Verify HTML update**: `LoadHTML()` or `UpdateHTML()` called after state changes
- **Check frameNumber**: `m_frontBuffer.frameNumber` should increment on render thread
- **Ensure caching works**: ReactiveUI's GetRenderedHTML() returns new HTML when dirty

### Excessive re-rendering (UI flickers or logs spam)
- **Symptom**: HTMLRendererMT logs "Rendering template" multiple times per second
- **Root cause**: SetValue() being called with unchanged values
- **Solution**: ✅ FIXED - LuaUIState::SetValue() now implements change detection (Jan 8, 2026)
- **Verify fix**: UI should only re-render when values actually change
- **Expected**: FPS stable at 60 = 0 re-renders, FPS changes 60→59 = 1 re-render

### Window not appearing (macOS)
- **CRITICAL**: `glfwPollEvents()` must be called every frame
- **Location**: `Game::RunFixedLoop()` line 92
- **Why**: macOS window manager needs event processing to display window
- **Without it**: Window created but never shown, OpenGL context unusable

### FreeType crashes
- All FreeType operations must be on render thread
- Check `m_ft_library` initialization
- Verify glyph cache access is thread-safe

### Texture flickering
- Buffer swap + frameNumber increment must be atomic
- Both operations must be inside `m_bufferMutex` lock
- Verify main thread checks frameNumber before upload

### Corrupted glyphs
- Ensure `FT_LOAD_NO_HINTING` flag is used
- Bitmap pitch must be handled correctly with row-by-row copy
- Check for negative pitch (bottom-up bitmaps)

### Event handlers not firing
- Verify handlers set BEFORE `LoadHTML()` (prevents race condition)
- Check event handler extraction in TemplateParser
- Verify HTMLRendererMT hit-testing logic
- Ensure Lua methods table has correct function signatures

### Race conditions
- Never touch `m_backBuffer` from main thread
- Never call OpenGL from render thread
- Use `m_bufferMutex` for all buffer access
- Use `m_mutex` for HTML string and resize requests

---

## Debugging Checklist

### When UI doesn't render at all:
1. ✅ Window appears? → Check `glfwPollEvents()` in main loop
2. ✅ HTML loaded? → Check `InitializeUI()` logs
3. ✅ Lua state loaded? → Check `LoadStateFile()` return value
4. ✅ Template registered? → Check `RegisterTemplateWithDirectives()` logs
5. ✅ Render thread running? → Check HTMLRendererMT initialization
6. ✅ OpenGL texture created? → Check glGetError() after texture upload

### When UI renders but doesn't update:
1. ✅ Dirty flag set? → Log `m_luaState->IsDirty()` before GetRenderedHTML()
2. ✅ SetValue() called? → Log Game::TrackFPS() calls
3. ✅ GetRenderedHTML() called? → Check main loop flow
4. ✅ UpdateHTML() called? → Check if html string actually changed
5. ✅ Render thread woke up? → Log m_cv.wait() in RenderThreadLoop()
6. ✅ FrameNumber incremented? → Log m_frontBuffer.frameNumber

### When UI re-renders excessively:
1. ✅ How often? → Count "Rendering template" logs per second
2. ✅ SetValue() spam? → Log all SetValue() calls with values
3. ✅ Value unchanged? → Compare old vs new value in logs
4. ✅ Fix needed? → Implement value comparison in SetValue()

### Breakpoint locations for debugging:

**Initialization:**
- `EngineCore::Initialize()` - Engine startup
- `EngineCore::InitializeUI()` - UI system setup
- `LuaUIState::LoadStateFile()` - Lua state loading
- `ReactiveUI::RegisterTemplateWithDirectives()` - Template parsing

**Render flow:**
- `Game::Render()` - Main render call
- `ReactiveUI::GetRenderedHTML()` - Dirty check
- `ReactiveUI::RenderWithLua()` - Template evaluation
- `HTMLRendererMT::RenderThreadLoop()` - Async rendering
- `HTMLRendererMT::Render()` - Texture upload

**Event handling:**
- `HTMLRendererMT::HandleClickEvent()` - Hit-testing
- `ReactiveUI::DispatchEvent()` - Event dispatch to Lua
- Lua handler function - Game logic

**Dirty flag flow:**
- `LuaUIState::SetValue()` - Marks dirty
- `LuaUIState::IsDirty()` - Check if re-render needed
- `LuaUIState::ClearDirty()` - After render completes

---

## File Organization

```
imhotep/
├── include/systems/
│   ├── ReactiveUI.h          # Template engine & reactive system
│   ├── TemplateParser.h      # Parse directives from HTML
│   ├── LuaUIState.h          # Lua state management
│   └── HTMLRendererMT.h      # Multi-threaded renderer
├── src/systems/
│   ├── ReactiveUI.cpp
│   ├── TemplateParser.cpp
│   ├── LuaUIState.cpp
│   └── HTMLRendererMT.cpp
├── res/ui/
│   ├── templates/            # HTML templates
│   │   └── tetris.html       # Tetris UI
│   ├── styles/               # CSS stylesheets
│   │   └── tetris.css        # Tetris styles
│   ├── components/           # Reusable UI components
│   │   ├── button.html
│   │   └── tooltip.html
│   └── state/                # Lua state files
│       ├── fps.lua           # Tetris UI state
│       ├── main_menu.lua
│       └── character_screen.lua
└── docs/
    └── architecture/
        └── UI_SYSTEM.md      # This file
```

---

## Risks & Mitigations

### Risk: litehtml limitations

**Mitigation**: Can swap renderer later; keep abstraction layer

### Risk: Lua performance for large UIs

**Mitigation**: LuaJIT for JIT compilation; move hot paths to C++

### Risk: Multi-threading complexity

**Mitigation**: Well-defined thread boundaries; thorough documentation; double buffering pattern

### Risk: Over-engineering

**Mitigation**: Build incrementally; ship features as needed

### Risk: Scope creep

**Mitigation**: This plan; stick to PDX-style use cases

---

## References

### Inspirational Projects
- **Paradox UI modding**: CK3/Stellaris modding documentation (Jomini GUI)
- **Vue.js**: Reactivity and directive patterns
- **React**: Virtual DOM and reconciliation

### Technical References
- **litehtml**: CSS subset we support
- **Sol2**: Lua/C++ binding library we use
- **FreeType**: Font rendering library

### Related Documentation
- `CLAUDE.md` - Project overview and AI assistant context
- `CHANGELOG.md` - Historical changes and decisions
- `VULKAN_MIGRATION.md` - Future rendering architecture considerations

---

**Note**: Goal is NOT to build React/Vue exactly - it's a **game-appropriate** system inspired by web patterns for PDX-style complexity.

---

_Last Updated: January 13, 2026_
