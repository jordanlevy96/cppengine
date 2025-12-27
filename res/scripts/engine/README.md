# Engine Python Bindings

This package provides Python bindings for the cppengine C++ game engine.

## Available Modules

The bindings are organized into logical modules:

### `app_module` - Core Application & Engine
- `App` - Main application singleton
- `Config` - Configuration settings
- `Registry` - Entity registry and management
- `Window` - Window manager singleton
- `InputEvent` - Input event data structure

### `camera` - Camera System
- `Camera` - Camera class with transform and perspective controls

### `enums` - Engine Enumerations
- `InputTypes` - Input event types (KEY, CLICK, CURSOR, RESIZE, SCROLL)
- `CameraDirections` - Camera movement directions (FORWARD, BACK, LEFT, RIGHT)

### `glm` - Math Types
- `vec2` - 2D vector (x, y)
- `vec3` - 3D vector (x, y, z)
- `vec4` - 4D vector (x, y, z, w)

## Usage

### Option 1: Import individual modules (current)
```python
import app_module
import camera
import enums
import glm

app = app_module.App.get_instance()
window = app_module.Window.instance
```

### Option 2: Use the unified engine package (recommended)
```python
from engine import App, Window, InputTypes, vec3

app = App.get_instance()
window = Window.instance

if event.type == InputTypes.KEY:
    handle_key(event.input)
```

### Option 3: Import the engine module
```python
import engine

app = engine.App.get_instance()
window = engine.Window.instance
```

## Example: Input Handler

```python
from engine import App, Window, Camera, InputTypes, CameraDirections

window = Window.instance
camera_rotate_flag = False

def handle_input(event):
    global camera_rotate_flag

    if event.type == InputTypes.KEY:
        key = event.input

        if key == "ESCAPE":
            window.CloseWindow()
        elif key == "SPACE":
            camera_rotate_flag = not camera_rotate_flag
        elif key == "W":
            app = App.get_instance()
            app.camera.Move(CameraDirections.FORWARD, app.delta)

    elif event.type == InputTypes.CURSOR and camera_rotate_flag:
        app = App.get_instance()
        app.camera.RotateByMouse(event.input.x, event.input.y)
```

## Building

The Python modules are built automatically by CMake when `SCRIPTING_LANG` is set to `PYTHON`:

```bash
cmake -DSCRIPTING_LANG=PYTHON ..
make
```

The built modules (.so files on Linux/Mac, .pyd on Windows) will be in the build directory.

## Game Logic

All game-specific logic should be implemented in Python scripts, not in C++ bindings.
The C++ bindings provide only the core engine functionality.

To create a game:
1. Create Python scripts in `res/scripts/`
2. Import the engine bindings
3. Implement your game logic using the provided APIs
4. Register your scripts with the engine

Example game script structure:
```
res/scripts/
  ├── engine/           # Engine bindings package
  │   └── __init__.py
  ├── init.py          # Initialization script (loaded by engine)
  ├── input.py         # Input handling
  └── your_game/       # Your game logic
      ├── __init__.py
      ├── entities.py
      └── systems.py
```
