# Changelog

All notable changes to Imhotep documented in chronological order.

---

## [2023-10] - Project Inception

### October 9, 2023
- **Initial Commit** - Project created
- Set up basic C++ OpenGL framework

### October 11-25, 2023
- **Basic Rendering Pipeline** - Triangle → rectangle → textures
- **GameManager & WindowManager** - Singleton pattern for core systems
- **Cross-platform builds** - macOS, Windows (WSL), Linux support
- **Mathematics & Transformations** - GLM integration for translate/rotate/scale
- **Input System** - Keyboard and mouse handling foundation

### October 30 - November 7, 2023
- **Camera Implementation** - Working 3D camera controls
- Started 3D object loading (OBJ files)
- Rendered Stanford bunny model successfully
- Rendered multiple 3D cubes

---

## [2023-11] - 3D Rendering & UI

### November 2023
- Renamed Model class to Mesh for clarity
- More robust camera controls (WASD + mouse)
- Added scroll wheel zooming using FOV
- Fixed M1 Mac-specific rendering bug with glDrawElements
- Fixed camera start position
- Started lighting shader implementation

### Late November 2023
- **Lua Scripting Integration** - First working Lua script
- Experimented with Nuklear GUI (ultimately abandoned)
- **Dear ImGui Integration** - Basic ImGui working
- Refactored ImGui into UI class
- Linux build compatibility

---

## [2023-12] - Lighting & ECS Foundation

### December 2023
- Cleanup and refactoring of codebase
- Generalized handling of shader uniforms
- **Lighting System** - Implemented lighting on 3D bunny model
- **YAML Parsing** - Added configuration file support
- **Entity Component System (ECS) Refactor** - Switched to shared_ptr-based registry
- Linux compatibility fixes

---

## [2024-01 to 2024-05] - Tetris Development Begins

### January 2024
- Cleaned up entity management
- Created tetriminos.yaml configuration
- Started Tetrimino rendering work
- **Working Tetrimino** - First tetris piece rendered
- Created basic class diagram for architecture
- Added new config settings
- Fixed specular lighting

### February - May 2024
- **Input Handling in Lua** - Moved input logic from C++ to Lua scripts
- Refactored tetriminos as proper entities
- Created CompositeEntity and ResourceManager
- **Rotation System** - Simple tetrimino rotations working
- Moved Transform functionality to TransformUtils utility
- Removed Emitter component (unused)
- Component cleanup and refactoring

---

## [2024-05 to 2025-07] - Scripting Systems & Game Logic

### May 2024
- **ScriptSystem** - Basic implementation for running Lua scripts per entity
- Set up example scripts
- Created TetrisGrid Lua class
- Major cleanup and refactoring

### July 2025
- Windows debugging session and path fixes
- Tetris grid rendering and camera alignment
- **Colored Tetriminos** - Spawned pieces in Lua with colors
- Tetrimino movement controls
- **Tween System** - Animation/interpolation implementation
- Enforced 16:9 aspect ratio

### July 26, 2025
- **Sparse Set Optimization** - Replaced std::map with custom SparseSet for ECS
  - Improved cache performance for component iteration
  - Removed shared_ptr overhead from entity management
- Added entity list to SparseSet for fast iteration
- Fixed ScriptComponent deconstruction issues
- Fixed Tetrimino rendering and movement after ECS refactor
- **Gravity System** - Added gravity to falling tetriminos
- **Python Scripting** - Added Python as scripting option alongside Lua
  - Built Python modules from C++ using pybind11
  - Made Sol2 (Lua) and pybind11 mutually exclusive builds
- Cleaned up Python/Lua input handling
- Fedora build support

---

## [2025-12] - Major Architecture Evolution

### December 24-25, 2025
- Updated all dependencies
- Claude Code cleanup session
- **Lua-First Approach** - Moved ALL Tetris game logic to Lua
  - Game is now 100% Lua-scripted
  - C++ engine just provides rendering/ECS infrastructure

### December 27, 2025
- **Unified Scripting** - Python and Lua can coexist (no longer mutually exclusive)
- Organized Tetris Lua code into Object-Oriented model
  - Created TetrisGrid.lua, Tetrimino.lua, TetrisConstants.lua
- Fixed collision detection and grid snapping issues
- **FPS Tracking** - Added FPS counter to main game loop

### December 28, 2025
- **HTML Rendering Experiment** - Started multi-process HTML rendering
  - Explored using litehtml for UI rendering
  - Integrated FreeType for font rendering
  - Initial implementation used separate process for rendering
- Fixed character positioning bug in text rendering

### December 30, 2025
- **Architecture Change: Multi-threaded Rendering**
  - Switched from multi-process to multi-threaded approach (HTMLRendererMT)
  - Implemented ReactiveUI system with Vue.js-style directives
  - Created TemplateParser for v-if, v-for, {{ }} interpolation
  - Added LuaUIState for reactive state management
  - Cleaner architecture with shared memory instead of IPC
- **Tetris UI Complete**
  - Added startup screen with "Press SPACE to start"
  - Implemented side panels showing game stats
  - Created game over screen with restart functionality

### December 31, 2025
- Fixed game over screen input handling
- Added restart and return-to-menu functionality
- **Quill Logging System** - Replaced std::cout with async logging
  - Optimized compilation by separating Logger header/implementation
  - Fixed macro redefinition warnings
  - Resolved Python linking errors (arm64 architecture)
- **Documentation Overhaul**
  - Created CLAUDE.md for AI assistant context
  - Created comprehensive CHANGELOG.md
  - Reorganized docs/ directory (deleted outdated V8/multiprocess docs)
  - Added comprehensive UI_SYSTEM.md architecture documentation

---

## Abandoned Experiments

### V8 JavaScript Engine (December 2025)
- Explored using V8 for UI scripting (see archived docs)
- **Decision**: Switched to Lua for consistency
  - Lua already used for game logic
  - V8 has ~20MB overhead vs Lua ~200KB
  - Simpler integration with Sol2 (already in use)

### Multi-Process HTML Rendering (December 28, 2025)
- Initially implemented HTML rendering in separate process with IPC
- **Decision**: Switched to multi-threaded approach
  - Simpler architecture (no IPC complexity)
  - Shared memory instead of serialization
  - Sufficient performance for game UI needs
  - Easier debugging

---

## [2026-01] - Interactive UI & Quality Improvements

### January 1, 2026
- **External HTML/CSS Templates**
  - Moved HTML templates from inline code in App.cpp to external files
  - Created `res/ui/templates/tetris.html` for template separation
  - Created `res/ui/styles/tetris.css` for stylesheet separation
  - ReactiveUI now loads templates from files via LoadTemplateFromFile()
  - Improved maintainability and separation of concerns

- **Web-Based UI Event Handling**
  - Implemented `@event` directive parsing in TemplateParser
  - Added support for `@click`, `@mouseover`, and other event directives
  - Auto-generate `data-event-id` attributes for interactive elements
  - Created event handler mapping system (elementId → {eventType → handlerExpr})
  - ReactiveUI DispatchEvent() method for calling Lua event handlers
  - Support for event objects with coordinates, button data, element ID
  - Auto-mark state dirty after event execution for UI updates

- **Interactive UI Elements**
  - Added clickable buttons with Lua event handlers
  - Implemented `@click` directives on START GAME, RESTART, and MAIN MENU buttons
  - Created Lua event handlers in fps.lua (onStartGame, onRestart, onMainMenu)
  - Added button hover effects (green glow, background highlight)
  - UI now supports both mouse clicks and keyboard shortcuts
  - Fixed race condition by setting event handlers before LoadHTML()

- **Input System Integration**
  - Extended WindowManager InputEvent to support position data
  - Click events now include vec3(x, y, button) for mouse button tracking
  - Added HTMLRendererMT Lua bindings (HandleClickEvent, UpdateHoverState)
  - Created OnClick and OnCursorMove handlers in input.lua
  - Full event flow: GLFW → WindowManager → Lua → HTMLRendererMT → ReactiveUI

- **Rendering Fixes**
  - Fixed HTML rendering coordinate system and text positioning
  - Corrected viewport transformation for proper UI alignment

- **Platform Support**
  - Added ARM64 Mac build instructions to CLAUDE.md and README
  - Documented Python 3.13 ARM64 integration for Apple Silicon
  - Added CMake command examples for forcing ARM64 Python on M1/M2 Macs

### January 2, 2026
- **FreeType Glyph Rendering Fix**
  - Fixed corrupted glyph rendering (especially 'E' character)
  - Added FT_LOAD_NO_HINTING flag to prevent aggressive hinting corruption
  - Implemented proper bitmap pitch handling with row-by-row copy
  - Added support for negative pitch (bottom-up bitmaps)
  - Added pixel mode validation for grayscale bitmaps
  - Significantly improved text rendering quality

- **Documentation Cleanup**
  - Removed obsolete BUILD_NOTES.md
  - Updated CLAUDE.md with latest architecture details
  - Improved HTML/CSS template documentation
  - Added event handling system documentation
  - Consolidated MULTITHREADING.md into comprehensive UI_SYSTEM.md

### January 3, 2026
- **Editor Phase 1 Implementation**
  - Created standalone `imhotep-editor` executable
  - Basic HTML/CSS UI with three-panel layout (Scene Hierarchy, Viewport, Inspector)
  - ESC key to quit editor
  - Dark theme UI (VS Code-inspired)
  - Proper HiDPI/Retina display support
  - Logger initialization fix (resolved segfault at startup)

### January 8, 2026
- **Architecture Refactoring**
  - Split monolithic `App` class into `EngineCore` (common init) + `Game` (game loop)
  - Shared initialization between Game and Editor, reduced code duplication
  - New files: `EngineCore.h`, `EngineCore.cpp`, `Game.h`, `Game.cpp`

- **UI Event Coordinate Scaling Fix**
  - Fixed button clicks not working on Retina/HiDPI displays
  - Added coordinate scaling in `HandleClickEvent()` and `UpdateHoverState()`
  - Buttons and interactive elements now work correctly on all display types

- **Rendering Fixes**
  - Fixed viewport and camera projection to use framebuffer size
  - Window resize now correctly updates camera projection matrix

### January 12, 2026
- **HierarchySystem for Transform Pipeline**
  - Added `HierarchySystem` for proper parent-child transform composition
  - Computes world matrices using matrix multiplication (fixes incorrect position composition)
  - Supports multi-level hierarchies (grandparents, etc.)
  - Clear system boundaries: scripts modify local transforms, HierarchySystem computes world transforms
  - New files: `HierarchySystem.h`, `HierarchySystem.cpp`

- **Input Event Queue Improvements**
  - Updated input event handling for better UI reactivity
  - Transform pipeline updates for smoother animations

### January 13, 2026
- **Documentation Cleanup**
  - Removed "current status" and "future work" references from all documentation
  - Removed timeline estimates from implementation phases
  - Updated all documentation dates

---

_Last Updated: January 13, 2026_
