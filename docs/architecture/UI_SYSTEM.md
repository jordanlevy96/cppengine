# Declarative UI System - Master Plan

## Vision

Build a web-based declarative UI system for complex, data-driven game interfaces.

## Architecture Overview

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
│  - Incremental updates          │
│  - Font caching                 │
│  - Dirty region rendering       │
└─────────────────────────────────┘
```

---

## Phase 1: Foundation

**Goal**: Basic reactive templating with change detection

### Implemented

- ✅ `ReactiveUI` - Template engine with change detection
- ✅ `HTMLRendererMT` - Multi-threaded rendering + font caching
- ✅ `TemplateParser` - Parses v-if, v-for, {{ }} directives
- ✅ `LuaUIState` - Loads UI state from .lua files
- ✅ Tetris UI - Working example with all Phase 1 features

### Example (Working in Tetris UI)

```lua
-- ui/character_list.lua
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
<!-- ui/character_list.html -->
<div v-if="showList">
  <h2>Characters</h2>
  <div v-for="char in characters">{{ char.name }} (Age: {{ char.age }})</div>
</div>
```

---

## Phase 2: Interactivity (PLANNED)

**Goal**: User interaction and dynamic updates

### Planned Features

1. **Event Handling**

   - `@click` - Click handlers
   - `@hover` - Mouse hover
   - Event propagation

2. **Two-Way Binding**

   - `v-model` - Form inputs
   - Sync C++ game state ↔ Lua ↔ UI

3. **Tooltip System**
   - `v-tooltip` - Rich nested tooltips (critical for PDX games)
   - Lazy evaluation (only compute when shown)

### Phase 2 Success Criteria

```lua
return {
    data = { selectedChar = nil },
    methods = {
        selectCharacter = function(self, char)
            self.selectedChar = char
        end
    }
}
```

```html
<div
  v-for="char in characters"
  @click="selectCharacter(char)"
  v-tooltip="char.getTooltip()"
>
  {{ char.name }}
</div>
```

---

## Phase 3: Performance & Scale (PLANNED)

**Goal**: Handle complex UIs with thousands of elements

### Planned Features

- Virtual DOM / diffing algorithm
- List virtualization (render only visible items)
- Lazy rendering for off-screen content
- Dirty rectangle rendering

**Target**: 10,000+ UI elements at 60 FPS

---

## Phase 4: Modding & Polish (PLANNED)

**Goal**: Production-ready for game release

### Planned Features

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
- **Already integrated**: You have Sol2 working

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

### Change Detection Strategy

- **Dirty checking**: Compare values on update (simple, works)
- **Later**: Proxy-based reactivity (Vue 3 style)
- **Later**: Fine-grained reactivity (Solid.js style)

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
│   ├── components/           # Reusable UI components
│   │   ├── button.html
│   │   └── tooltip.html
│   ├── screens/              # Game screens
│   │   ├── main_menu.html
│   │   ├── character_screen.html
│   │   └── diplomacy.html
│   └── state/                # Lua state files
│       ├── main_menu.lua
│       └── character_screen.lua
└── docs/
    └── UI_SYSTEM_PLAN.md     # This file
```

---

## Risks & Mitigations

### Risk: litehtml limitations

**Mitigation**: Can swap renderer later; keep abstraction layer

### Risk: Lua performance for large UIs

**Mitigation**: LuaJIT for JIT compilation; move hot paths to C++

### Risk: Over-engineering

**Mitigation**: Build incrementally; ship features as needed

### Risk: Scope creep

**Mitigation**: This plan; stick to PDX-style use cases

---

## References

- **Paradox UI modding**: CK3/Stellaris modding documentation
- **Vue.js**: Reactivity and directive patterns
- **React**: Virtual DOM and reconciliation
- **litehtml**: CSS subset we support
- **Sol2**: Lua/C++ binding library we use

**Note**: Goal is NOT to build React/Vue exactly - it's a **game-appropriate** system inspired by web patterns for PDX-style complexity.
