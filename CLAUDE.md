# CLAUDE.md - AI Assistant Context for Imhotep

> Last Updated: 2026-01-01
> For: Claude Sonnet 4.5

## Project Overview

**Imhotep** is an experimental C++ game engine using OpenGL, Lua, and more. Currently implements a fully functional Tetris game as proof-of-concept.

**Key Innovation**: Declarative, reactive UI system using HTML/CSS templates with Lua state management (Vue.js-inspired), rendered via litehtml with multi-threaded rendering.

**Current Focus**: Building declarative UI system for strategy game interfaces (Paradox Interactive-style).

---

## Architecture at a Glance

```
App (controllers/App.cpp)
├─► WindowManager (GLFW + OpenGL context)
├─► Camera (3D camera for game world)
├─► RenderSystem (3D scene rendering)
├─► HTMLRendererMT (UI rendering - MULTI-THREADED)
│   ├─► ReactiveUI (template + state management)
│   ├─► TemplateParser (v-if, v-for directives)
│   └─► LuaUIState (Lua state files)
├─► ScriptManager (Lua + Python VMs)
└─► Registry (ECS - entities & components)
```

**Critical Insight**: UI rendering happens on separate thread to avoid blocking game loop. See `docs/architecture/MULTITHREADING.md`.

---

## Dependencies & Build System

### External Dependencies

| Dependency            | Purpose         | Integration    | Install                         |
| --------------------- | --------------- | -------------- | ------------------------------- |
| **GLFW**              | Window/input    | FetchContent   | Auto-downloaded                 |
| **GLAD**              | OpenGL loader   | FetchContent   | Auto-downloaded                 |
| **FreeType**          | Font rendering  | System package | `brew install freetype` (macOS) |
| **GLM**               | Math library    | Git submodule  | In `external/`                  |
| **litehtml**          | HTML/CSS engine | Git submodule  | In `external/`                  |
| **Lua + Sol2**        | Lua scripting   | Git submodule  | In `external/`                  |
| **Python + pybind11** | Python bindings | Git submodule  | In `external/`                  |
| **yaml-cpp**          | Config parsing  | Git submodule  | In `external/`                  |
| **Dear ImGui**        | Debug UI        | Git submodule  | In `external/`                  |

### Platform-Specific Setup

**macOS**:

```bash
brew install cmake freetype
cd external && git submodule update --init --recursive
mkdir build && cd build && cmake .. && make -j8
```

**macOS (Apple Silicon - ARM64 Python Issue)**:
If CMake finds x86_64 Python instead of ARM64 Python on M1/M2 Macs, force it to use ARM64:

```bash
cd /Users/jordan/dev/cppengine/build
rm -rf *

cmake \
  -DPython3_EXECUTABLE=/opt/homebrew/bin/python3.13 \
  -DPython3_LIBRARY=/opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/lib/libpython3.13.dylib \
  -DPython3_INCLUDE_DIR=/opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/include/python3.13 \
  ..

make -j8
```

Verify ARM64 Python: `file /opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/Python` should show `arm64`.

**Ubuntu/Debian**:

```bash
sudo apt install cmake g++ libfreetype6-dev
cd external && git submodule update --init --recursive
mkdir build && cd build && cmake .. && make -j8
```

**Windows** (untested recently):

- Visual Studio 2022 (C++ Desktop Development)
- CMake
- FreeType from vcpkg or manual build

### Build Configurations

- **Debug**: `cmake -DCMAKE_BUILD_TYPE=Debug ..` (includes all logging)
- **Release**: `cmake -DCMAKE_BUILD_TYPE=Release ..` (optimized)

---

## Common Tasks & Workflows

### 1. Adding a New UI Screen

**Steps**:

1. Create Lua state file: `res/ui/state/my_screen.lua`
2. Update `App.cpp` → `Initialize()` to load your state
3. Update HTML template (currently inline in App.cpp, lines 72-200)

**Example Lua state**:

```lua
-- res/ui/state/my_screen.lua
return {
    data = {
        title = "My Screen",
        showPanel = true,
        items = {
            {name = "Item 1", value = 10},
            {name = "Item 2", value = 20}
        }
    }
}
```

**Example HTML template** (in App.cpp):

```html
<div v-if="showPanel">
  <h1>{{ title }}</h1>
  <div v-for="item in items">{{ item.name }}: {{ item.value }}</div>
</div>
```

**To trigger updates**: Mark Lua state as dirty:

```cpp
m_luaState->MarkDirty();  // Next frame will re-render
```

### 2. Adding a New Shader

**Steps**:

1. Create `res/shaders/MyShader.shader` with `#shader vertex` and `#shader fragment` sections
2. Load in C++: `auto shader = new Shader("../res/shaders/MyShader.shader");`
3. Use: `shader->Use(); shader->SetMat4("projection", projMatrix);`

**Existing shaders**:

- `Composite.shader` - HTMLRendererMT UI overlay
- `Text.shader` - Text rendering (if used)
- `UI.shader` - ImGui rendering

### 3. Debugging Multi-Threaded Renderer

**HTMLRendererMT runs on separate thread** - see `docs/architecture/MULTITHREADING.md`

**Common issues**:

- **UI not updating**: Check `m_luaState->IsDirty()` flag
- **Crashes in FreeType**: Race condition - check mutex locks
- **Texture not uploading**: Check `m_frontBuffer.frameNumber` vs `m_lastFrameNumber`

**Useful breakpoints**:

- `HTMLRendererMT::RenderThreadLoop()` - Render thread entry (src/systems/HTMLRendererMT.cpp:790)
- `HTMLRendererMT::UpdateTextureFromPixelBuffer()` - Texture upload (src/systems/HTMLRendererMT.cpp:688)
- `ReactiveUI::GetRenderedHTML()` - Dirty check (src/systems/ReactiveUI.cpp:12)

### 4. Working with Lua Scripts

**Lua is used for**:

- UI state (`res/ui/state/`)
- Game logic (see Tetris example in git history - commit f4462e2)

**Executing Lua from C++**:

```cpp
sol::state& lua = scriptManager->GetLuaState();
lua.script_file("../res/ui/state/fps.lua");
sol::table data = lua["data"];
```

**Calling C++ from Lua** (bindings in `ScriptManager.cpp`):

```cpp
lua.set_function("CreateEntity", &Registry::CreateEntity);
```

### 5. Adding ECS Components

**Pattern**:

1. Create header: `include/components/MyComponent.h`
2. Define struct: `struct MyComponent { float value; };`
3. Register in `Registry::LoadScene()`: `entity.emplace<MyComponent>(...)`
4. Create system to iterate components (optional)

**Existing components**:

- `Transform` - Position, rotation, scale, color
- `RenderComponent` - Mesh, shader, texture references
- `ScriptComponent` - Lua script reference
- `HierarchyComponent` - Parent/child relationships
- `Tween` - Animation interpolation

### 6. Adding New Dependencies

**CRITICAL**: When adding ANY new library or external dependency, update BOTH:

1. **README.md** - External Dependencies section

   - Add to appropriate category (system dependency, FetchContent, or git submodule)
   - Include installation instructions if needed

2. **REFERENCES.md** - Two sections:
   - External Libraries section (description + usage)
   - License Information section (license type)

**Example CMake patterns**:

```cmake
# System package (like FreeType)
find_package(NewLibrary REQUIRED)
target_link_libraries(core PUBLIC ${NEWLIBRARY_LIBRARIES})

# FetchContent (like Quill)
FetchContent_Declare(newlib
    GIT_REPOSITORY https://github.com/author/newlib.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(newlib)
target_link_libraries(core PUBLIC newlib::newlib)

# Git submodule (like yaml-cpp)
add_subdirectory(external/newlib)
target_link_libraries(core PUBLIC newlib)
```

---

## File Organization & Naming

### Directory Structure

```
imhotep/
├── include/          # Headers (.h)
│   ├── components/   # ECS component definitions (data only)
│   ├── controllers/  # Singletons (App, Registry, ScriptManager, WindowManager)
│   ├── systems/      # System implementations (Render, UI, Script, HTML)
│   └── util/         # Utilities (Shader, Mesh, Camera, etc.)
├── src/              # Implementations (.cpp) - mirrors include/
├── res/              # Runtime resources
│   ├── ui/           # HTML templates, Lua UI state
│   │   └── state/    # Lua state files (fps.lua, etc.)
│   ├── shaders/      # GLSL shaders
│   ├── scenes/       # YAML scene definitions
│   ├── conf/         # YAML config (settings.yaml)
│   └── scripts/      # Game Lua scripts (Tetris*.lua)
├── docs/             # Documentation
│   ├── architecture/ # Current system designs
│   └── guides/       # How-to guides
├── external/         # Third-party libraries (git submodules)
└── build/            # CMake build output (gitignored)
```

### Naming Conventions

- **Classes**: PascalCase (`HTMLRendererMT`, `ReactiveUI`, `WindowManager`)
- **Files**: Match class name (`HTMLRendererMT.h`, `HTMLRendererMT.cpp`)
- **Functions**: PascalCase (`Initialize()`, `LoadHTML()`, `GetInstance()`)
- **Member variables**: `m_` prefix (`m_frontBuffer`, `m_texture`, `m_width`)
- **Parameters/locals**: camelCase (`width`, `height`, `needsRender`)

### Suffixes with Meaning

- `-MT` = Multi-Threaded (`HTMLRendererMT`)
- `-Manager` = Singleton controller (`WindowManager`, `ScriptManager`)
- `-System` = ECS system (`RenderSystem`, `TweenSystem`, `ScriptSystem`)
- `-Component` = ECS component (sometimes omitted: `Transform` not `TransformComponent`)

### Code Documentation Style

**All headers use Doxygen-style comments** for IDE integration (VS Code, CLion, Visual Studio).

**Format:**

```cpp
/**
 * @file FileName.h
 * @brief One-line description
 */

/**
 * @brief Class/function description
 *
 * Detailed explanation if needed.
 * Can include usage examples.
 *
 * @param paramName Parameter description
 * @return Return value description
 * @note Important notes about thread safety, performance, etc.
 * @see Reference to related docs or code
 */
```

**Member variable docs:**

```cpp
int m_width = 800;  ///< Short description after declaration
```

**Required for:**

- All public API classes and functions
- Complex internal functions that need clarification
- Thread-safety critical code (document which thread owns what)

**Examples:**

- `include/util/Logger.h` - Comprehensive Doxygen docs
- `include/Camera.h` - Class and method documentation
- `include/systems/HTMLRendererMT.h` - Thread safety documentation

---

## Key Insights for Claude Code

### 1. Multi-Threading

**HTMLRendererMT runs litehtml on separate thread**. Always consider:

- Which thread am I modifying? (main game loop vs render thread)
- Do I need mutex locks? (see `m_mutex`, `m_bufferMutex`)
- Is this FreeType/litehtml code? (must be on render thread)

**Safe on main thread**:

- Reading `m_frontBuffer` (with `m_bufferMutex`)
- Calling `LoadHTML()`, `Render()`, `Resize()`
- OpenGL operations

**Safe on render thread**:

- Writing to `m_backBuffer`
- FreeType font operations
- litehtml rendering

See `docs/architecture/MULTITHREADING.md` for full details.

### 2. Separation of Concerns

Each programming language has a distinct use case and usage must remain within its domain.

- **C++**: Game engine, main loop, rendering, inputs, etc.
- **Lua**: UI state, game logic (all gameplay code)
- **Python**: Data exports, analytics and other external tooling

### 3. Resource Paths Relative to Project Root

All paths use `../res/` prefix (run from `build/` directory, cwd is `/Users/jordan/dev/cppengine/build/`):

```cpp
shader = new Shader("../res/shaders/Composite.shader");
luaState->LoadStateFile("../res/ui/state/fps.lua");
```

---

## Critical Documentation References

**Read these FIRST for work in these areas**:

| Area                 | Document                                | When to Read                                         |
| -------------------- | --------------------------------------- | ---------------------------------------------------- |
| **UI System**        | `docs/architecture/UI_SYSTEM.md`        | Adding UI features, directives, templates            |
| **Multi-threading**  | `docs/architecture/MULTITHREADING.md`   | Working on HTMLRendererMT, renderer                  |
| **Vulkan Migration** | `docs/architecture/VULKAN_MIGRATION.md` | Planning OpenGL → Vulkan migration (future research) |
| **Project History**  | `CHANGELOG.md`                          | Understanding why architecture evolved               |

---

## Decision Guide

**Ask first**:

- Architecture changes (new libraries, refactors, design patterns)
- Breaking changes to existing systems
- Adding new dependencies
- Deleting significant old code

**Proceed confidently**:

- Bug fixes, features following established patterns
- Single-file refactoring
- Documentation and comment improvements
- Tracking changes as you go using git

---

## Logging System

**Library**: Quill (v7.4.0) - High-performance async logging (~12-16μs latency)

**Usage**:

```cpp
#include "util/Logger.h"

LOG_TRACE_L1("HTMLRenderer::LoadHTML called with {} bytes", html.size());
LOG_DEBUG("Loaded {} glyphs in {}ms", count, duration);
LOG_INFO("HTMLRenderer initialized: {}x{}", width, height);
LOG_WARNING("Font fallback: {} not found, using default", fontName);
LOG_ERROR("Failed to load shader: {}", path);
LOG_CRITICAL("OpenGL context creation failed");
```

**Output**: `logs/imhotep.log` (also echoed to console)

**Initialization**: Automatic via `Logger::GetInstance()` singleton - no manual setup needed

---

## Useful Commands Reference

### Build & Run

```bash
# Full rebuild
rm -rf build && mkdir build && cd build && cmake .. && make -j8

# Incremental build
cd build && make -j8

# Run
./build/imhotep

# Clean
rm -rf build
```

### Submodules

```bash
# Initialize all submodules
cd external && git submodule update --init --recursive

# Update submodules to latest
cd external && git submodule update --remote
```

### Search & Navigation

```bash
# Find TODOs
grep -r "TODO" src/ include/ --exclude-dir=external

# Find function definition
grep -rn "void Initialize" include/

# Search for class
grep -rn "class HTMLRendererMT" include/
```

### Git History

```bash
# Recent changes
git log --oneline --since="2 weeks ago"

# File history
git log -p --follow -- path/to/file.cpp

# Find when feature was added
git log --oneline --grep="HTML"
```

---

## Project Structure Quick Reference

**Need to add a UI element?** → `res/ui/state/*.lua` + template in `res/ui/**.html` or `res/ui/styles/*.css`
**Need to render something 3D?** → Create entity with `Transform` + `RenderComponent`
**Need game logic?** → Lua script in `res/scripts/`
**Need to change window/input?** → `controllers/WindowManager.cpp`
**Need to modify rendering pipeline?** → `systems/RenderSystem.cpp`
**Need to change UI rendering?** → `systems/HTMLRendererMT.cpp` (thread-safe!)

---

## Common Pitfalls

**Threading**: Never touch `m_backBuffer` from main thread (render thread owns it). Use `m_frontBuffer` with `m_bufferMutex`.

**Abandoned approaches**: Don't suggest things we have already tried (see CHANGELOG.md).

**Paths**: Run from `build/` directory.

---

## Testing & Debugging

**No automated tests exist yet.** Testing is manual:

1. Build: `cd build && make`
2. Run: `./imhotep`
3. Play Tetris, observe UI, check console output

**Debug build**:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
lldb ./imhotep  # or gdb on Linux
```

**Common debug scenarios**:

- Manual regression tests fail: scrutinize changes
- Crash on startup: Check resource paths, shader compilation
- Build failures: Rebuild from scratch and run everything from the `build` directory

---

## Performance Notes

**Target**: 60 FPS (16.67ms per frame)

**Typical breakdown** (Dec 2025, M1 Mac):

- Game logic: 1-2ms
- 3D rendering: 2-3ms
- UI rendering (thread): 5-15ms (doesn't block!)
- UI composite: 1-2ms
- **Total main thread**: ~7ms (plenty of headroom)

**UI render budget**: HTML rendering can take 15ms because it's async. Main thread only pays for texture upload (~2ms).

---

## Version & Compatibility

**C++ Standard**: C++17
**OpenGL Version**: 3.3 Core Profile (minimum)
**CMake Version**: 3.12+ (required)

**Tested Platforms** (as of Dec 2025):

- ✅ macOS (M1/Intel) - Primary development
- ⚠️ Ubuntu 20.04+ - Should work, not recently tested
- ⚠️ Windows 10/11 - Build works, not recently tested
- ❌ Web/WASM - Not supported

---

_Last Verified: December 31, 2025_
_This file should be updated regularly and manually; prompt the user to make sure it is up to date._
