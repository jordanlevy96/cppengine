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
  - Added MULTITHREADING.md and UI_SYSTEM.md architecture docs

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

_Last Updated: December 31, 2025_
