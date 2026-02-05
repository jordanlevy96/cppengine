"""
Python initialization - works in both development and bundled modes.
"""
import sys
import os

# Import engine bindings (PYTHONPATH already set by C++)
import engine

# Add scripts directory if not already in path
scripts_path = os.path.join(engine.getResourcePath(), 'scripts')
if scripts_path not in sys.path:
    sys.path.insert(0, scripts_path)

print(f"[Python] Initialized. Bundle mode: {engine.isInstalledBundle()}")
