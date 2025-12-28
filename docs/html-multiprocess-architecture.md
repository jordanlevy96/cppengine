# Multi-Process HTML Rendering Architecture

## Overview

To support CSS/JS animations without blocking the main game loop, we use a separate process for HTML rendering that communicates via shared memory.

## Architecture

```
┌─────────────────────────┐         ┌──────────────────────────┐
│   Main Game Process     │         │  HTML Render Process     │
│                         │         │                          │
│  ┌─────────────────┐   │         │  ┌──────────────────┐   │
│  │  Game Loop      │   │         │  │  Render Loop     │   │
│  │  (120 FPS)      │   │         │  │  (60 FPS)        │   │
│  └────────┬────────┘   │         │  └────────┬─────────┘   │
│           │            │         │           │             │
│           v            │         │           v             │
│  ┌─────────────────┐   │         │  ┌──────────────────┐   │
│  │  Upload Texture │◄──┼─────────┼──│  Render to       │   │
│  │  from Shared    │   │ Shared  │  │  Pixel Buffer    │   │
│  │  Memory         │   │ Memory  │  │                  │   │
│  └─────────────────┘   │         │  └──────────────────┘   │
│           │            │         │           ▲             │
│           v            │         │           │             │
│  ┌─────────────────┐   │         │  ┌────────┴─────────┐   │
│  │  Composite to   │   │         │  │  litehtml +      │   │
│  │  Screen         │   │         │  │  FreeType        │   │
│  └─────────────────┘   │         │  └──────────────────┘   │
│           │            │         │                          │
│           │            │  IPC    │                          │
│           └────────────┼────────►│  ┌──────────────────┐   │
│     (LoadHTML,         │ Socket  │  │  Command Handler │   │
│      Resize, etc.)     │         │  └──────────────────┘   │
└─────────────────────────┘         └──────────────────────────┘
```

## Shared Memory Layout

```cpp
struct SharedFrameBuffer {
    // Metadata
    uint32_t width;
    uint32_t height;
    uint32_t frameNumber;
    volatile uint8_t ready;  // 0 = being written, 1 = ready to read
    uint8_t padding[3];

    // Pixel data (RGBA, bottom-left origin for OpenGL)
    uint8_t pixels[width * height * 4];
};
```

## IPC Messages (Unix Domain Socket)

**Main → Render Process:**
- `LOAD_HTML`: Load new HTML content
- `RESIZE`: Update viewport size
- `SHUTDOWN`: Gracefully terminate render process
- `MARK_DIRTY`: Force re-render (for future dynamic content updates)

**Render → Main Process:**
- `READY`: New frame available
- `ERROR`: Rendering error occurred

## Process Lifecycle

### Startup
1. Main process creates shared memory region
2. Main process spawns render process with shm name as argument
3. Render process attaches to shared memory
4. Render process sends READY message
5. Main process sends initial LOAD_HTML message

### Runtime
1. Render process continuously renders HTML to shared buffer (60fps)
2. Main process checks `ready` flag each frame
3. If ready: copy pixels to OpenGL texture
4. Main process can send messages to update HTML/size

### Shutdown
1. Main process sends SHUTDOWN message
2. Render process cleans up and exits
3. Main process unlinks shared memory

## Performance Characteristics

**Benefits:**
- HTML rendering happens in parallel on separate CPU core
- Main game loop never blocked by HTML rendering
- Supports smooth CSS animations without frame drops
- Future: Can add JavaScript engine without impacting game performance

**Costs:**
- Memory copy from shared buffer to GL texture (~2-5ms for 1920x1080 RGBA)
- Process overhead (~8MB per process)
- IPC latency (~0.1ms for messages)

**Expected Performance:**
- Game loop: 120 FPS (unchanged)
- HTML updates: 60 FPS (independent)
- Total overhead: <10ms per frame for large UIs

## Implementation Plan

### Phase 1: Basic Multi-Process Setup
- [ ] Create shared memory management (SHM creation, mapping)
- [ ] Implement render process skeleton
- [ ] Set up Unix domain socket IPC
- [ ] Basic message passing (LOAD_HTML, SHUTDOWN)

### Phase 2: Rendering Pipeline
- [ ] Move litehtml rendering to render process
- [ ] Implement pixel buffer rendering (software rendering or FBO)
- [ ] Implement texture upload in main process
- [ ] Synchronization (ready flag, double buffering)

### Phase 3: Advanced Features
- [ ] CSS animation support (time-based re-rendering)
- [ ] Integrate V8 JavaScript engine
- [ ] Interactive events (mouse clicks, keyboard input)
- [ ] Multiple HTML windows/layers

## Alternative Approaches Considered

1. **Threading instead of processes**: Rejected because:
   - V8 requires separate process for isolation
   - Process crash won't take down game
   - Easier memory management

2. **GPU-based HTML rendering**: Rejected because:
   - Requires WebGL or similar in render process
   - More complex, less portable
   - Can add later if needed

3. **Chromium Embedded Framework (CEF)**: Considered but:
   - Very large dependency (~200MB)
   - Overkill for simple game UI
   - litehtml + V8 gives us more control
