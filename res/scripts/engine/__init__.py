"""
Engine - Unified Python bindings package for cppengine

This package provides a single import point for all C++ engine bindings.
Instead of importing each module separately:
    import app_module
    import camera
    import enums
    import glm

You can import from the engine package:
    from engine import App, Camera, InputTypes, vec3
    # or
    import engine

Usage:
    from engine import App, Registry, Window

    app = App.get_instance()
    registry = Registry.GetInstance()
    window = Window.instance
"""

try:
    # Import all C++ binding modules
    import app_module
    import camera
    import enums
    import glm as glm_module

    # Re-export commonly used classes and functions
    # From app_module
    App = app_module.App
    Config = app_module.Config
    Registry = app_module.Registry
    Window = app_module.Window
    InputEvent = app_module.InputEvent

    # From enums
    InputTypes = enums.InputTypes
    CameraDirections = enums.CameraDirections

    # From camera (module reference)
    Camera = camera.Camera

    # From glm (types)
    vec2 = glm_module.vec2
    vec3 = glm_module.vec3
    vec4 = glm_module.vec4

    # Export all for "from engine import *"
    __all__ = [
        'App', 'Config', 'Registry', 'Window', 'InputEvent',
        'InputTypes', 'CameraDirections',
        'Camera',
        'vec2', 'vec3', 'vec4'
    ]

except ImportError as e:
    import sys
    print(f"Error importing engine modules: {e}", file=sys.stderr)
    print("Make sure the Python modules are built and in your Python path", file=sys.stderr)
    raise
