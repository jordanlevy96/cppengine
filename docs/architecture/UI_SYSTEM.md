# Declarative UI System - Master Plan

## Vision
Build a PDX-style (Crusader Kings, Stellaris) declarative UI system for complex, data-driven game interfaces.

## Why Not ImGui
- ImGui is code-based (procedural), not declarative
- Unmaintainable for complex UIs (character screens, diplomacy, tech trees)
- Hard for designers/modders to work with
- PDX games need data binding, templates, and reactive updates

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

## Phase 1: Foundation (CURRENT)
**Goal**: Basic reactive templating with change detection

### What We've Built
- ✅ `ReactiveUI` - Simple template substitution with `{{key}}` placeholders
- ✅ Change detection - Only re-renders when values actually change
- ✅ `HTMLRendererMT` - Multi-threaded HTML rendering with font caching

### What We're Building Next
1. **Template Directive Parser**
   - Parse `v-if`, `v-for`, `v-bind`, `@click` from HTML
   - Build Abstract Syntax Tree (AST) from templates

2. **Lua Integration**
   - Load UI state from `.lua` files
   - Reactive property system (auto-track dependencies)
   - Computed properties (derived values)

3. **Basic Directives**
   - `v-if` - Conditional rendering
   - `v-for` - List rendering
   - `{{ }}` - Text interpolation

### Deliverables
- [ ] Parse directives from HTML templates
- [ ] Lua state files that define reactive data
- [ ] v-if directive working
- [ ] v-for directive working
- [ ] Example: Character list that shows/hides based on state

### Success Criteria
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
    <div v-for="char in characters">
        {{ char.name }} (Age: {{ char.age }})
    </div>
</div>
```

---

## Phase 2: Interactivity
**Goal**: User interaction and dynamic updates

### Features
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

### Deliverables
- [ ] @click directive working
- [ ] Methods in Lua state files
- [ ] Tooltip system with nested content
- [ ] Example: Clickable character that shows tooltip

### Success Criteria
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
<div v-for="char in characters"
     @click="selectCharacter(char)"
     v-tooltip="char.getTooltip()">
    {{ char.name }}
</div>
```

---

## Phase 3: Performance & Scale
**Goal**: Handle complex UIs with thousands of elements

### Features
1. **Virtual DOM / Diffing**
   - Don't re-render entire tree
   - Diff old vs new state
   - Only update changed nodes

2. **List Virtualization**
   - Only render visible items
   - Critical for lists of 1000+ elements

3. **Lazy Rendering**
   - Don't render off-screen tabs/panels
   - Deferred loading

4. **Dirty Rectangle Rendering**
   - Only re-rasterize changed regions
   - GPU-accelerated compositing

### Deliverables
- [ ] Virtual DOM implementation
- [ ] Diff algorithm (reconciliation)
- [ ] Virtual list component
- [ ] Performance benchmarks (render 1000+ items smoothly)

### Success Criteria
- Render list of 10,000 characters at 60 FPS
- Update single character without re-rendering entire UI
- Lazy tabs don't compute until shown

---

## Phase 4: Modding & Polish
**Goal**: Production-ready system for game release

### Features
1. **Component System**
   - Reusable UI components
   - Props and slots
   - Component library (buttons, panels, etc.)

2. **Modding Support**
   - Hot reload templates
   - Override base templates
   - Mod load order

3. **Developer Tools**
   - UI inspector (like browser DevTools)
   - State debugger
   - Performance profiler

4. **Advanced CSS**
   - Animations
   - Transitions
   - Advanced layouts (grid, flexbox improvements)

### Deliverables
- [ ] Component registration system
- [ ] Mod loading infrastructure
- [ ] Hot reload for development
- [ ] UI inspector tool

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

## Current Status

### Completed
- ✅ Basic ReactiveUI with change detection
- ✅ Multi-threaded HTML renderer
- ✅ Font caching
- ✅ Double-free bug fixes
- ✅ Template substitution (`{{key}}`)

### In Progress
- 🔨 Template directive parser
- 🔨 Lua state integration
- 🔨 v-if, v-for directives

### Next Up
- Template AST builder
- Lua VM for UI state
- Directive implementations

---

## File Organization

```
cppengine/
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

## Success Metrics

By end of Phase 2, you should be able to:
- Define UI in HTML templates
- Manage state in Lua files
- Click elements and update game state
- Show rich tooltips on hover
- Build a simple character screen

By end of Phase 3, you should be able to:
- Render complex UIs (1000+ elements) smoothly
- Update individual elements without full re-render
- Handle PDX-scale complexity (province maps, diplomacy, etc.)

By end of Phase 4, you should be able to:
- Ship a moddable game
- Let UI designers work independently
- Support community content creation

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

---

## Notes for Future Context

When resuming work on this system:

1. **Check Phase 1 deliverables** - What's done?
2. **Read `ReactiveUI.h/cpp`** - Current implementation
3. **Look at test files** - `res/ui/` for examples
4. **Run the engine** - See what actually works
5. **Continue from current phase** - Don't restart

The goal is NOT to build React/Vue exactly - it's to build a **game-appropriate** system inspired by web patterns that works well for PDX-style complexity.
