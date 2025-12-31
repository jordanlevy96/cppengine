# Multi-Threaded HTML Rendering Architecture

**File**: `include/systems/HTMLRendererMT.h`, `src/systems/HTMLRendererMT.cpp`
**Last Updated**: December 30, 2025

## Overview

HTMLRendererMT implements a multi-threaded HTML/CSS rendering system that prevents UI rendering from blocking the main game loop. HTML is rendered on a dedicated background thread using litehtml + FreeType, then composited onto the screen via OpenGL textures.

## Why Multi-Threading?

**Problem**: litehtml rendering (especially with FreeType font rasterization) can take 10-50ms for complex layouts. This would cause frame drops in a 60 FPS game loop.

**Solution**: Render HTML on background thread, swap buffers when ready, composite on main thread. Main game loop never blocks.

### Alternative Approaches Considered

1. **Multi-Process** (attempted Dec 28) - Separate process with IPC
   - ❌ Too complex (Unix sockets, shared memory, serialization)
   - ❌ Harder to debug (two processes)
   - ✅ Better isolation

2. **Single-Threaded** (original) - Render HTML in main loop
   - ❌ Blocks game loop during rendering
   - ❌ Frame drops when UI updates
   - ✅ Simple architecture

3. **Multi-Threaded** (current) - Background thread with double buffering
   - ✅ Non-blocking main loop
   - ✅ Simpler than multi-process
   - ✅ Shared memory without IPC
   - ✅ Easier debugging

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

**Mutexes**:
- `m_mutex` - Protects shared HTML string and resize requests
- `m_bufferMutex` - Protects frame buffer swap (very short critical section)

**Condition Variable**:
- `m_cv` - Wakes render thread when new HTML arrives or resize needed

**Atomic**:
- `m_running` - Clean shutdown signal (no lock needed)

### Render Thread Lifecycle

**Startup** (`Initialize()`):
```cpp
m_running = true;
m_renderThread = std::make_unique<std::thread>(&HTMLRendererMT::RenderThreadLoop, this);
```

**Loop** (`RenderThreadLoop()`):
```cpp
while (m_running) {
    // Wait for work
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this] { return !m_running || m_hasNewHTML || m_needsResize; });

    if (!m_running) break;

    // Handle resize
    if (m_needsResize) {
        m_backBuffer.resize(...);
        m_needsResize = false;
    }

    // Handle new HTML
    if (m_hasNewHTML) {
        currentHTML = m_pendingHTML;
        m_hasNewHTML = false;
    }
    lock.unlock();

    // Render (outside critical section)
    renderer.RenderHTML(currentHTML);

    // Swap buffers
    {
        std::lock_guard<std::mutex> bufferLock(m_bufferMutex);
        std::swap(m_frontBuffer, m_backBuffer);
        m_frontBuffer.frameNumber++;
    }
}
```

**Shutdown** (`Shutdown()`):
```cpp
m_running = false;
m_cv.notify_one();
if (m_renderThread->joinable()) {
    m_renderThread->join();  // Wait for thread to finish
}
```

### SoftwareRenderer (Inner Class)

Runs on render thread, implements litehtml `document_container` interface:

- **Font Management**: Loads FreeType fonts, caches glyphs
- **Text Rendering**: Rasterizes text with alpha blending
- **Layout**: Uses litehtml for HTML/CSS parsing and layout
- **Drawing**: Software rendering to pixel buffer (no GL on render thread)

**Why software rendering?**
- OpenGL contexts are not easily thread-safe
- FreeType + simple pixel blitting is fast enough for UI
- Avoids complex GL context sharing

## Thread Safety Guarantees

### What's Safe

1. **Main thread** reads `m_frontBuffer` (protected by `m_bufferMutex`)
2. **Render thread** writes to `m_backBuffer` (exclusive ownership)
3. **Main thread** uploads texture from `m_frontBuffer` (only after frame number change)

### Critical Sections

1. **LoadHTML()** (main thread):
   ```cpp
   {
       std::lock_guard<std::mutex> lock(m_mutex);
       m_pendingHTML = html;
       m_hasNewHTML = true;
   }
   m_cv.notify_one();  // Wake render thread
   ```

2. **Buffer Swap** (render thread):
   ```cpp
   {
       std::lock_guard<std::mutex> lock(m_bufferMutex);
       std::swap(m_frontBuffer, m_backBuffer);
       m_frontBuffer.frameNumber++;
   }
   ```

3. **Texture Upload** (main thread):
   ```cpp
   {
       std::lock_guard<std::mutex> lock(m_bufferMutex);
       if (m_frontBuffer.frameNumber != m_lastFrameNumber) {
           UpdateTextureFromPixelBuffer();
           m_lastFrameNumber = m_frontBuffer.frameNumber;
       }
   }
   ```

### Potential Race Conditions (Avoided)

❌ **Don't do this**:
```cpp
// BAD: Reading m_backBuffer from main thread
glTexSubImage2D(..., m_backBuffer.pixels.data());  // RACE CONDITION!
```

✅ **Correct approach**:
```cpp
// GOOD: Only read m_frontBuffer (protected by mutex)
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    glTexSubImage2D(..., m_frontBuffer.pixels.data());
}
```

## Performance Characteristics

### Timings (Dec 2025 on M1 Mac)

- **HTML Rendering**: 5-15ms (varies with complexity)
- **Texture Upload**: 1-2ms (glTexSubImage2D)
- **Composite**: <1ms (single quad draw)
- **Main Thread Impact**: ~2ms (only upload + composite)

### Optimization: When to Update

ReactiveUI checks dirty flag before calling `LoadHTML()`:
```cpp
if (m_luaState && m_luaState->IsDirty()) {
    RenderWithLua();              // Generate new HTML
    htmlRenderer->LoadHTML(...);   // Trigger background render
    m_luaState->ClearDirty();
}
```

**Result**: HTML only re-renders when Lua state changes (e.g., score update, game state change).

## Common Issues & Debugging

### Issue: UI Not Updating

**Symptoms**: HTML changes but screen doesn't update

**Debug**:
1. Check if `m_luaState->IsDirty()` is returning true
2. Verify `LoadHTML()` is being called
3. Check render thread is running: `m_running == true`
4. Look for frame number changes: `m_frontBuffer.frameNumber`

**Fix**: Usually dirty flag not set after Lua state change.

### Issue: Crash in FreeType

**Symptoms**: Segfault in `FT_Load_Char` or similar

**Debug**:
1. Check if `m_ft_library` is initialized
2. Verify fonts are loaded on render thread (not main thread)
3. Check for race conditions in font cache access

**Fix**: Ensure all FreeType operations happen on render thread.

### Issue: Texture Flickering

**Symptoms**: UI flashes or shows old frames

**Debug**:
1. Check if buffer swap is atomic (should be inside mutex)
2. Verify frame number increments correctly
3. Check `m_lastFrameNumber` tracking

**Fix**: Ensure `m_bufferMutex` protects entire swap + increment.

## Integration with ReactiveUI

HTMLRendererMT works with ReactiveUI system:

```
ReactiveUI → TemplateParser → LuaUIState
     ↓ (generates HTML)
HTMLRendererMT → SoftwareRenderer → litehtml + FreeType
     ↓ (pixel buffer)
OpenGL → Composite Shader → Screen
```

See `docs/architecture/UI_SYSTEM.md` for reactive UI details.

## Future Improvements

1. **Partial Updates**: Only re-render changed regions (dirty rectangles)
2. **Triple Buffering**: Reduce latency with 3 buffers instead of 2
3. **GPU Rendering**: Use OpenGL on render thread (context sharing)
4. **Vulkan Backend**: For better multi-threading support

## References

- Implementation: `src/systems/HTMLRendererMT.cpp`
- Header: `include/systems/HTMLRendererMT.h`
- Reactive UI: `docs/architecture/UI_SYSTEM.md`
- CHANGELOG: Decision to switch from multi-process (Dec 30, 2025)

---

_Last Verified: December 31, 2025_
