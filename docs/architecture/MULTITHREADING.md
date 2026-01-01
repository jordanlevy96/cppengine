# Multi-Threaded HTML Rendering Architecture

**File**: `include/systems/HTMLRendererMT.h`, `src/systems/HTMLRendererMT.cpp`
**Last Updated**: December 30, 2025

## Overview

HTMLRendererMT implements a multi-threaded HTML/CSS rendering system that prevents UI rendering from blocking the main game loop. HTML is rendered on a dedicated background thread using litehtml + FreeType, then composited onto the screen via OpenGL textures.

## Why Multi-Threading?

HTML rendering can be slow, causing frame drops at 60 FPS. IPC would introduce too much overhead compared to multi-threading.

**Solution**: Render HTML on background thread, swap buffers when ready, composite on main thread. Main loop never blocks.

## Architecture Diagram

```
Main Thread                    Render Thread
─────────────                  ─────────────
App::Run()                     RenderThreadLoop()
  │                                 │
  ├─ Update game                    │
  ├─ ReactiveUI::GetRenderedHTML()  │
  │   ├─ Check if dirty             │
  │   └─ Returns cached HTML        │
  │                                 │
  ├─ HTMLRendererMT::Render()       │
  │   ├─ Check m_frontBuffer        │
  │   └─ Upload to GL texture       │
  │                                 │
  └─ Composite UI overlay           │
                                    ├─ Wait for new HTML (m_cv)
                                    ├─ Render HTML → m_backBuffer
                                    │   ├─ litehtml layout
                                    │   ├─ FreeType rasterization
                                    │   └─ Software rendering
                                    ├─ Swap buffers (lock m_bufferMutex)
                                    └─ Increment frame number
```

## Key Components

### Double Buffering

Two `FrameBuffer` structures prevent race conditions:

```cpp
struct FrameBuffer {
    uint32_t width;
    uint32_t height;
    uint32_t frameNumber;   // Incremented on each render
    std::vector<uint8_t> pixels;  // RGBA format
};

FrameBuffer m_frontBuffer;  // Read by main thread
FrameBuffer m_backBuffer;   // Written by render thread
```

**Flow**:

1. Render thread writes to `m_backBuffer`
2. When complete, lock `m_bufferMutex` and `std::swap(m_frontBuffer, m_backBuffer)`
3. Increment `m_frontBuffer.frameNumber`
4. Main thread detects new frame number, uploads to GL texture

### Thread Synchronization

- `m_mutex` - Protects HTML string and resize requests
- `m_bufferMutex` - Protects buffer swap (short critical section)
- `m_cv` - Wakes render thread on new HTML/resize
- `m_running` - Atomic shutdown flag

### Render Thread Lifecycle

**Pattern**: Wait for work → render HTML → swap buffers → repeat

1. `m_cv.wait()` blocks until new HTML or resize
2. Render to `m_backBuffer` (outside critical section)
3. Swap buffers under `m_bufferMutex`
4. Increment `frameNumber` to signal main thread

See `HTMLRendererMT::RenderThreadLoop()` (src/systems/HTMLRendererMT.cpp:790) for full implementation.

### SoftwareRenderer

Inner class running on render thread, implements litehtml `document_container`:

- Font management (FreeType + glyph cache)
- Text rasterization (alpha blending)
- Layout (litehtml HTML/CSS parsing)
- Software rendering to pixel buffer

**Why not OpenGL on render thread?** Context sharing is complex; FreeType + pixel blitting is fast enough for UI.

## Thread Safety

**Safe operations**:

- Main thread: reads `m_frontBuffer` (with `m_bufferMutex`), GL operations
- Render thread: writes `m_backBuffer`, FreeType operations

**Critical**: Never touch `m_backBuffer` from main thread. Never OpenGL from render thread.

**Pattern for texture upload**:

```cpp
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    if (m_frontBuffer.frameNumber != m_lastFrameNumber) {
        glTexSubImage2D(..., m_frontBuffer.pixels.data());  // ✅ Safe
        m_lastFrameNumber = m_frontBuffer.frameNumber;
    }
}
```

## Performance (M1 Mac, Dec 2025)

- HTML rendering: 5-15ms (on render thread, doesn't block)
- Texture upload: 1-2ms (main thread cost)
- Composite: <1ms
- **Total main thread**: ~2ms per frame

**Optimization**: ReactiveUI dirty flag prevents re-renders unless Lua state changes.

## Common Issues

**UI not updating**: Check `m_luaState->IsDirty()` flag, verify `LoadHTML()` called, check `m_frontBuffer.frameNumber` increments.

**FreeType crash**: All FreeType ops must be on render thread. Check `m_ft_library` initialization.

**Texture flickering**: Buffer swap + frameNumber increment must be atomic (both inside `m_bufferMutex`).

## Integration

HTMLRendererMT integrates with ReactiveUI → TemplateParser → LuaUIState → litehtml + FreeType → OpenGL composite.

See `docs/architecture/UI_SYSTEM.md` for reactive UI details.

---

_Last Verified: December 31, 2025_
