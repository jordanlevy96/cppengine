**Viewport Click Feedback — Investigation & Fix**

**Status**: ROOT CAUSE IDENTIFIED AND FIXED - Awaiting Testing

**Summary**: Investigated missing click feedback for viewport UI elements. Root cause was in handler expression evaluation during v-for template rendering. Loop variables (e.g., `entity.id`) were not being substituted into handler expressions before storage, causing Lua type mismatches when handlers were invoked.

**Root Cause Found**

When template rendering processes v-for loops like:

```html
<div v-for="entity in entities" @click="selectEntity(entity.id)"></div>
```

The handler expression `selectEntity(entity.id)` was stored as-is without substituting the actual loop variable value. Later, when the click was dispatched, ReactiveUI tried to evaluate `entity.id` but `entity` didn't exist in the Lua global scope—it only existed during template rendering.

This caused:

```
[ReactiveUI] Lua handler 'selectEntity' failed: stack index 1, expected number,
received table: not a numeric type that fits exactly an integer
```

**Fixes Applied**

1. **[TemplateParser.cpp](src/systems/TemplateParser.cpp#L225-L240)** — Modified `SerializeElementForIteration` to process handler expressions:

   - Added call to `ProcessIterationInterpolations()` on event handler expressions
   - Substitutes loop variable values at render time (e.g., `selectEntity(entity.id)` → `selectEntity(1)`)
   - Handlers are now stored with concrete values instead of expressions

2. **[HTMLRendererMT.cpp](src/systems/HTMLRendererMT.cpp#L1220-L1250)** — Enhanced click/hover logging:

   - Added INFO-level logs showing which element was hit and its handler
   - Logs coordinates and button info during dispatch
   - Logs completion for better event flow traceability

3. **[ReactiveUI.cpp](src/systems/ReactiveUI.cpp#L177-L185)** — Improved error context:
   - Error messages now include event type and element ID
   - Better distinction between "methods table not found" vs "handler not found"
   - Provides handler expression context for debugging

**Event Flow (After Fix)**

- **Native input → Editor**: GLFW/window system receives clicks, forwards to editor input handlers
- **Input → ScriptManager**: Engine input events pushed into Lua EventQueue
- **Lua EventQueue → UI script**: Input handlers in [res/scripts/input.lua](res/scripts/input.lua) process events and call `htmlRenderer:HandleClickEvent(x,y,button)`
- **HTML renderer hit-test**: `HTMLRendererMT::HandleClickEvent` performs HiDPI-aware coordinate conversion and hits-tests elements in reverse z-order
- **Dispatch to ReactiveUI**: When element hit, renderer calls `ReactiveUI::DispatchEvent` with **pre-evaluated handler** (e.g., `selectEntity(1)` not `selectEntity(entity.id)`)
- **Lua handler execution**: ReactiveUI executes handler with concrete argument values → selectEntity C++ function receives proper integer ID → updates engine state → visual feedback displayed

**Changes Made**

| File                                                                         | Change                                                 | Purpose                                                      |
| ---------------------------------------------------------------------------- | ------------------------------------------------------ | ------------------------------------------------------------ |
| [src/systems/TemplateParser.cpp](src/systems/TemplateParser.cpp#L225-L240)   | Call ProcessIterationInterpolations on @event handlers | Substitute loop variable values into handlers at render time |
| [src/systems/HTMLRendererMT.cpp](src/systems/HTMLRendererMT.cpp#L1220-L1250) | Enhanced click/mouseover/mouseout logging              | Better debugging visibility into event dispatch              |
| [src/systems/ReactiveUI.cpp](src/systems/ReactiveUI.cpp#L177-L185)           | Improved error messages with event/element context     | Clearer error reporting when handler dispatch fails          |

**Reproduction Steps (Testing)**

1. Launch `imhotep-editor` from build folder
2. Click on entity items in the Scene Hierarchy sidebar (left panel)
3. Verify entity selection changes (background color updates to blue)
4. Check logs for: `[HTMLRendererMT] HIT!` → `[HTMLRendererMT] Dispatching click event` → `[ReactiveUI] Handler 'selectEntity' executed successfully`

**Expected Behavior After Fix**

- Click on entity in sidebar → element bounds hit-tested correctly
- Handler expression `selectEntity(entity.id)` stored as `selectEntity(<actual_id>)`
- ReactiveUI receives concrete integer argument → passes to C++ selectEntity function
- No type mismatch errors in logs
- Selected entity highlights in UI and inspector updates
