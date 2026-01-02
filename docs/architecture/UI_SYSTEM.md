# Declarative UI System - Architecture & Implementation

**Last Updated**: January 2, 2026

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
Main Thread                    Render Thread
─────────────                  ─────────────
App::Run()                     RenderThreadLoop()
  │                                 │
  ├─ Update game                    │
  ├─ ReactiveUI::GetRenderedHTML()  │
  │   ├─ Check if dirty             │
  │   └─ Returns cached HTML        │
  │                                 │
  ├─ HTMLRendererMT::Render()       │
  │   ├─ Check m_frontBuffer        │
  │   └─ Upload to GL texture       │
  │                                 │
  └─ Composite UI overlay           │
                                    ├─ Wait for new HTML (m_cv)
                                    ├─ Render HTML → m_backBuffer
                                    │   ├─ litehtml layout
                                    │   ├─ FreeType rasterization
                                    │   └─ Software rendering
                                    ├─ Swap buffers (lock m_bufferMutex)
                                    └─ Increment frame number
```

**Why Multi-Threading?** HTML rendering can take 5-15ms, causing frame drops at 60 FPS. IPC would introduce too much overhead. Solution: Render HTML on background thread, swap buffers when ready, composite on main thread. Main loop never blocks.

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

### Phase 1: Foundation ✅ COMPLETE

**Goal**: Basic reactive templating with change detection

**Implemented**:
- ✅ `ReactiveUI` - Template engine with change detection
- ✅ `HTMLRendererMT` - Multi-threaded rendering + font caching
- ✅ `TemplateParser` - Parses v-if, v-for, {{ }} directives
- ✅ `LuaUIState` - Loads UI state from .lua files
- ✅ External HTML/CSS files (res/ui/templates/, res/ui/styles/)
- ✅ Tetris UI - Working example with all Phase 1 features

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

### Phase 2: Interactivity 🟡 IN PROGRESS

**Goal**: User interaction and dynamic updates

**Implemented Features**:

1. **Event Handling** ✅
   - `@click` - Click handlers with Lua event methods
   - `@mouseover` - Mouse hover support (infrastructure ready)
   - Event handler extraction and dispatching via TemplateParser
   - Full event flow: GLFW → WindowManager → Lua → HTMLRendererMT → ReactiveUI
   - Event objects with coordinates, button data, element ID
   - Auto-mark state dirty after event execution

2. **Lua Event Methods** ✅
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

**In Progress / Planned**:

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

### Phase 3: Performance & Scale 📋 PLANNED

**Goal**: Handle complex UIs with thousands of elements

**Planned Features**:
- Virtual DOM / diffing algorithm
- List virtualization (render only visible items)
- Lazy rendering for off-screen content
- Dirty rectangle rendering

**Target**: 10,000+ UI elements at 60 FPS

### Phase 4: Modding & Polish 📋 PLANNED

**Goal**: Production-ready for game release

**Planned Features**:
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

## Troubleshooting

### UI not updating
- Check `m_luaState->IsDirty()` flag
- Verify `LoadHTML()` called after state changes
- Check `m_frontBuffer.frameNumber` increments
- Ensure ReactiveUI's GetRenderedHTML() returns new HTML

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
│   │   └── tetris.html       # Tetris UI (working example)
│   ├── styles/               # CSS stylesheets
│   │   └── tetris.css        # Tetris styles (working example)
│   ├── components/           # Reusable UI components (planned)
│   │   ├── button.html
│   │   └── tooltip.html
│   └── state/                # Lua state files
│       ├── fps.lua           # Tetris UI state (working example)
│       ├── main_menu.lua     # (planned)
│       └── character_screen.lua  # (planned)
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

_Last Verified: January 2, 2026_
