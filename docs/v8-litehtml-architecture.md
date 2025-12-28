# V8 + litehtml Architecture for Game Engine UI

## Overview

Integration architecture combining V8 JavaScript engine with litehtml HTML/CSS renderer.

## Component Roles

```
┌─────────────────────────────────────────────────────────┐
│                     Game Engine                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐        ┌──────────────┐             │
│  │  V8 Engine   │◄──────►│   litehtml   │             │
│  │ (JavaScript) │        │  (HTML/CSS)  │             │
│  └──────┬───────┘        └──────┬───────┘             │
│         │                       │                      │
│         │    ┌──────────────────┘                      │
│         │    │                                         │
│         ▼    ▼                                         │
│  ┌─────────────────────┐                              │
│  │  Bridge/Glue Layer  │                              │
│  │  - Expose engine to JS                             │
│  │  - Implement litehtml::document_container          │
│  │  - Handle DOM events → V8                          │
│  └─────────┬───────────┘                              │
│            │                                           │
│            ▼                                           │
│  ┌─────────────────────┐                              │
│  │  OpenGL Renderer    │                              │
│  │  - Draw text        │                              │
│  │  - Draw rectangles  │                              │
│  │  - Draw images      │                              │
│  │  - Texture atlas    │                              │
│  └─────────────────────┘                              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## Implementation Steps

### Phase 1: Integrate litehtml
1. Add litehtml as dependency
2. Implement `document_container` interface
3. Render static HTML to OpenGL texture/framebuffer
4. Test with simple HTML: buttons, divs, text

### Phase 2: Add V8
1. Embed V8 in engine
2. Create V8 context
3. Expose engine APIs to JavaScript (similar to your Lua bindings)
4. Execute JavaScript from HTML `<script>` tags

### Phase 3: Connect DOM to V8
1. Create JavaScript DOM API wrapping litehtml elements
2. Implement `document.getElementById()`, `addEventListener()`, etc.
3. Handle user input events → V8 event handlers
4. Allow JavaScript to modify DOM → trigger litehtml reflow

### Phase 4: Bridge Game Engine
1. Expose ECS registry to JavaScript
2. Allow UI scripts to query entities, components
3. Call C++ functions from JS (spawn entities, etc.)
4. Update UI from game state (health bars, scores, etc.)

## Code Structure

```
src/
├── ui/
│   ├── HTMLRenderer.h/cpp        # Main UI system
│   ├── LitehtmlContainer.h/cpp   # litehtml::document_container impl
│   ├── V8Context.h/cpp           # V8 runtime wrapper
│   ├── DOMBindings.h/cpp         # JavaScript DOM API
│   └── EngineBindings.h/cpp      # Expose engine to JavaScript
```

## Key Interfaces to Implement

### litehtml::document_container

```cpp
class LitehtmlContainer : public litehtml::document_container {
public:
    // Drawing callbacks
    void draw_background(/* params */);
    void draw_borders(/* params */);
    void draw_text(/* params */);
    void draw_list_marker(/* params */);

    // Resource loading
    void load_image(const char* src, /* params */);
    void get_image_size(const char* src, /* params */);

    // Font operations
    uint_ptr create_font(const char* faceName, int size, /* params */);
    void delete_font(uint_ptr hFont);
    int text_width(const char* text, uint_ptr hFont);

    // Layout
    void set_clip(const litehtml::position& pos);
    void del_clip();
    int get_default_font_size();

    // etc...
};
```

### V8 Engine Bindings

```cpp
// In V8Context.cpp - expose engine to JavaScript
void V8Context::exposeEngineAPI() {
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);

    // Expose registry
    global->Set(isolate, "getEntity",
        v8::FunctionTemplate::New(isolate, getEntityCallback));

    // Expose input
    global->Set(isolate, "isKeyPressed",
        v8::FunctionTemplate::New(isolate, isKeyPressedCallback));

    // etc...
}
```

### JavaScript DOM API

```javascript
// Example usage in HTML file
<script>
    // Standard DOM API
    const button = document.getElementById('startBtn');
    button.addEventListener('click', () => {
        // Call into game engine
        startGame();

        // Query ECS
        const player = getEntity('player');
        console.log('Player health:', player.getComponent('Health').value);
    });

    // Update UI from game state
    function updateHealthBar() {
        const player = getEntity('player');
        const health = player.getComponent('Health').value;
        document.getElementById('healthBar').style.width = health + '%';
    }
</script>
```

## Rendering Strategy

### Option A: Render to Texture
```cpp
// Render UI to offscreen framebuffer
FBO uiFBO;
litehtml.render(uiFBO);

// Composite UI texture over 3D scene
renderScene();
renderUITexture(uiFBO.texture);
```

### Option B: Direct Drawing
```cpp
// Render 3D scene
renderScene();

// Switch to 2D orthographic projection
setupUIProjection();

// litehtml calls your draw callbacks directly
litehtml.render();
```

## Performance Considerations

1. **Texture Atlas**: Cache fonts and UI elements in texture atlas
2. **Dirty Regions**: Only re-render changed parts of UI
3. **Batching**: Batch draw calls by shader/texture
4. **V8 Context Isolation**: Separate context per UI document if needed

## Dependencies

- **litehtml**: `git submodule add https://github.com/litehtml/litehtml.git external/litehtml`
- **V8**: Prebuilt binaries or build from source (~30 min)
- **gumbo-parser**: Included with litehtml
- **Font rendering**: FreeType (you'll need to add this)
- **Text shaping**: HarfBuzz (for complex text layout)

## Alternatives at Each Layer

| Layer | Current Choice | Alternatives |
|-------|---------------|--------------|
| JavaScript | V8 | QuickJS (lighter), Duktape |
| HTML/CSS | litehtml | Custom parser, WebKit (heavy) |
| Layout | litehtml's engine | Yoga (flexbox only) |
| Font rendering | FreeType | stb_truetype (simpler) |
| Text shaping | HarfBuzz | Skip for English-only (use kerning tables) |

## Minimal Viable Product

For quick proof-of-concept:

1. **Just litehtml** (no JS initially)
   - Load static HTML file
   - Implement basic drawing callbacks
   - See if you like the API

2. **Add mouse interaction**
   - Pass mouse events to litehtml
   - Get hover/click working
   - Fire C++ callbacks on button clicks

3. **Then add V8**
   - Embed V8
   - Execute `<script>` tags
   - Wire up event handlers

This incremental approach lets you validate each layer.

## Example: Tetris Menu

Instead of ImGui, you could have:

```html
<!-- res/ui/tetris_menu.html -->
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            background: #1a1a2e;
            color: #eee;
            font-family: Arial;
        }
        .menu {
            width: 400px;
            margin: 100px auto;
            text-align: center;
        }
        button {
            width: 200px;
            height: 50px;
            margin: 10px;
            font-size: 18px;
            background: #16213e;
            color: #eee;
            border: 2px solid #0f3460;
        }
        button:hover {
            background: #0f3460;
        }
    </style>
</head>
<body>
    <div class="menu">
        <h1>Tetris</h1>
        <button id="start">Start Game</button>
        <button id="options">Options</button>
        <button id="quit">Quit</button>
    </div>

    <script>
        document.getElementById('start').addEventListener('click', () => {
            // Call Lua function or C++ function
            startTetrisGame();
            hideMenu();
        });

        document.getElementById('quit').addEventListener('click', () => {
            quitGame();
        });
    </script>
</body>
</html>
```

Much more familiar for web developers than ImGui!
