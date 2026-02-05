# Event Test UI - Usage Guide

## Overview

This test UI demonstrates all the new event system features added to Imhotep's reactive UI system:

- **Fixed double-click issue**: Buttons now only fire once per click (not on both press and release)
- **Mouse enter/leave events**: `@mouseenter` and `@mouseleave` directives for hover detection
- **Mouse down/up events**: `@mousedown` and `@mouseup` for fine-grained press control
- **v-table directive**: Automatic table generation from Lua data structures

## Files

- `templates/event_test.html` - HTML template with event examples
- `styles/event_test.css` - Styling for the test UI
- `state/event_test.lua` - Lua state with event handlers and test data

## How to Use

### Option 1: Load via EngineCore (Recommended)

Modify your initialization code to load the event test UI:

```cpp
// In Game.cpp or your initialization code
#include "systems/ReactiveUI.h"
#include "systems/LuaUIState.h"
#include "systems/HTMLRendererMT.h"

// Load the test UI
auto luaState = std::make_shared<LuaUIState>();
luaState->LoadStateFile("../res/ui/state/event_test.lua");

ReactiveUI& ui = ReactiveUI::GetInstance();
ui.BindLuaState(luaState);

std::string uiTemplate = ReactiveUI::LoadTemplateFromFiles(
    "../res/ui/templates/event_test.html",
    "../res/ui/styles/event_test.css"
);

ui.RegisterTemplateWithDirectives("event_test", uiTemplate);

// Update HTML renderer
HTMLRendererMT& htmlRenderer = HTMLRendererMT::GetInstance();
htmlRenderer.SetEventHandlers(ui.GetEventHandlers());
htmlRenderer.LoadHTML(ui.GetRenderedHTML());
```

### Option 2: Quick Test in Existing Scene

If you already have a UI loaded, you can swap it at runtime:

```cpp
// Save reference to current state
auto testState = std::make_shared<LuaUIState>();
testState->LoadStateFile("../res/ui/state/event_test.lua");

ReactiveUI::GetInstance().BindLuaState(testState);
// ... (continue with template loading as above)
```

## Test Sections

### 1. Click Events (Fixed Double-Click)
**Purpose**: Verify that buttons only fire once per click, not twice.

**Test**:
- Click the "Click Me!" button
- Observe the counter increments by 1 per click (not 2)
- Check event log shows single CLICK event per user action

### 2. Mouse Enter/Leave Events
**Purpose**: Test new `@mouseenter` and `@mouseleave` event handlers.

**Test**:
- Hover over Box 1 and Box 2 (uses new events)
- Hover over Box 3 (uses legacy @mouseover/@mouseout for comparison)
- Observe status changes and event log entries
- Verify enter/leave events fire correctly when crossing boundaries

### 3. Mouse Down/Up Events
**Purpose**: Test separate press and release event handling.

**Test**:
- Press and hold the purple button
- Observe "PRESSING..." state while holding
- Release the button
- Check press duration calculation in milliseconds
- Verify MOUSEDOWN fires on press, MOUSEUP fires on release

### 4. v-table Directive
**Purpose**: Test automatic table generation from Lua data.

**Test**:
- Observe two auto-generated tables: Player Stats and Performance Metrics
- Click "Refresh Data" button
- Verify tables update with new random data
- Check that table structure (headers + rows) renders correctly

**Data Format**:
```lua
tableData = {
    columns = {"Col1", "Col2", "Col3"},  -- or "headers"
    rows = {                              -- or "data"
        {val1, val2, val3},
        {val4, val5, val6}
    }
}
```

### 5. Combined Events
**Purpose**: Test multiple event types on a single element.

**Test**:
- Click the interactive card (increments click counter)
- Hover over card (changes status to "Hovering...")
- Press and hold on card (shows "Pressed!" and increments press counter)
- Verify all counters update independently

### 6. Event Log
**Purpose**: Live event tracking for debugging.

**Test**:
- All events from sections 1-5 appear in the log with timestamps
- Events are shown in reverse chronological order (newest first)
- "Clear Log" button resets the log

## Event Handler API

### New Event Types

```html
<!-- Mouse enter/leave (recommended for hover) -->
<div @mouseenter="onHoverStart" @mouseleave="onHoverEnd"></div>

<!-- Mouse down/up (for press/release tracking) -->
<button @mousedown="onPressStart" @mouseup="onPressEnd"></button>

<!-- Legacy events (still supported) -->
<div @click="onClick" @mouseover="onOver" @mouseout="onOut"></div>
```

### Lua Event Handler Format

```lua
methods = {
    -- No arguments
    onClick = function(self)
        self.data.clicks = self.data.clicks + 1
    end,

    -- With string argument
    onHover = function(self, itemName)
        print("Hovered: " .. itemName)
    end,

    -- With event object
    onMouseDown = function(self, event)
        print("Pressed at: " .. event.x .. ", " .. event.y)
    end
}
```

### v-table Usage

```html
<!-- Basic usage -->
<table v-table="myData"></table>

<!-- With CSS classes -->
<table v-table="statsTable" class="styled-table striped"></table>

<!-- Container element (generates <table> inside) -->
<div v-table="playerData" class="table-wrapper"></div>
```

## Expected Output

When running the test UI, you should see:

1. **Console logs**: Event handlers print to console with `[EventTest]` prefix
2. **Visual feedback**: Hover effects, color changes, status updates
3. **Counter updates**: Click counts, hover counts, press counts increment
4. **Event log**: Timestamped log of all events at the bottom
5. **Table updates**: Random data generation when clicking "Refresh Data"

## Troubleshooting

### Double-click still occurring
- Check that WindowManager.cpp only processes GLFW_PRESS events
- Verify no duplicate event handlers registered

### Hover events not firing
- Ensure `UpdateHoverState()` is called in main loop
- Check that HTMLRendererMT is properly initialized
- Verify cursor position is being tracked

### v-table not rendering
- Check Lua data structure format (must have columns/headers and rows/data)
- Verify table data is a valid Lua table
- Look for TemplateParser errors in logs

### Events fire but handlers don't execute
- Verify Lua state methods table exists
- Check function names match exactly (case-sensitive)
- Look for Lua errors in console output

## Performance Notes

- **Event logging**: Event log grows unbounded - use "Clear Log" periodically
- **Table updates**: Refreshing large tables may cause frame drops (current test data is small)
- **Hover events**: Called frequently during mouse movement (optimized with state change detection)

## Integration with Existing UI

To add these events to your existing UI templates:

```html
<!-- Add to your existing templates -->
<button @click="existingHandler"
        @mouseenter="onHoverStart"
        @mouseleave="onHoverEnd">
    My Button
</button>

<!-- Use v-table instead of manual table markup -->
<table v-table="data.myTableData" class="my-styles"></table>
```

No C++ code changes needed - just update your HTML templates and Lua state!

## Further Reading

- `docs/architecture/UI_SYSTEM.md` - Full UI system documentation
- `CLAUDE.md` - Project overview and coding guidelines
- `include/systems/ReactiveUI.h` - Event system API reference
- `include/systems/TemplateParser.h` - Directive parsing documentation

---

**Last Updated**: January 22, 2026
**Author**: Claude (via jordanlevy96)
**Status**: Tested and working
