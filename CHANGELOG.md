# Changelog

All notable changes to cppengine documented in chronological order.

---

## [2023-10] - Project Inception

### October 9, 2023
- **Initial Commit** - Project created
- Set up basic C++ OpenGL framework

### October 11-17, 2023
- Added GameManager singleton pattern
- Achieved working build on macOS
- Fixed CMake for WSL (Windows Subsystem for Linux)
- Created GLFW window ("Hello window!")
- Rendered first triangle
- Created WindowManager class and refactoring
- Refactored triangle into GameObject class and Renderer
- Set up GLM dependency for mathematics
- Rendered rectangle using Element Buffer Objects (EBO)
- Loaded and rendered textures

### October 19-25, 2023
- Rendered two textures simultaneously
- Implemented basic transformations (translate, rotate, scale)
- Refactored input system for camera support
- Windows build support (cross-platform development)

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
- Windows debugging session
- Fixed relative path references
- Spawned tetris grid and aligned camera
- **Colored Tetriminos** - Spawned colored pieces in Lua
- Tetrimino movement controls working
- **Tween System** - Simple animation/interpolation implementation
- Enforced 16:9 aspect ratio

---

## [2025-07] - ECS Optimization & Python Support

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
- Polish and bug fixes

---

## Current Status (December 31, 2025)

**Architecture**:
- Entity Component System (ECS) with custom SparseSet implementation
- Multi-threaded HTML/CSS UI rendering (HTMLRendererMT)
- Reactive UI system with Lua state management
- Dual scripting: Lua (game logic + UI) and Python (data analysis)

**Complete Features**:
- 3D rendering with OpenGL 3.3+
- Camera system with FPS controls
- Lighting system (Phong shading)
- Texture loading and rendering
- YAML-based configuration
- Lua + Python scripting integration
- ECS with components: Transform, RenderComponent, ScriptComponent, HierarchyComponent, Tween
- FreeType font rendering
- HTML/CSS UI with reactive templates
- Working Tetris game (100% Lua-scripted)

**Technologies**:
- C++17
- OpenGL 3.3+ (Core Profile)
- CMake build system
- Libraries: GLFW, GLAD, GLM, FreeType, litehtml, Sol2, pybind11, yaml-cpp, Dear ImGui

**Key Innovations**:
- Declarative UI using HTML/CSS rendered via litehtml
- Vue.js-inspired reactive templates (v-if, v-for, interpolation)
- Multi-threaded rendering to prevent UI from blocking game loop
- Lua-driven game logic with hot-reloadable scripts

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

## Version History

**Note**: This project does not currently use semantic versioning. Versions listed here are retroactive organization based on major milestones.

- **v0.1** (Oct 2023) - Basic rendering engine
- **v0.2** (Nov 2023) - 3D rendering, camera, lighting
- **v0.3** (Dec 2023) - ECS architecture, YAML config
- **v0.4** (Jan-May 2024) - Tetris game foundation
- **v0.5** (Jul 2025) - Sparse set optimization, Python support
- **v0.6** (Dec 2025) - HTML UI system, reactive templates
- **Current** (Dec 31, 2025) - Fully functional Tetris with modern UI

---

_Last Updated: December 31, 2025_
