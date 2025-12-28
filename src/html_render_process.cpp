#include "systems/HTMLRenderIPC.h"
#include "systems/SharedMemory.h"
#include "systems/UnixSocket.h"

#include <litehtml.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <algorithm>

// Software rendering document container for litehtml
class SoftwareRenderer : public litehtml::document_container {
public:
    struct Glyph {
        int width, height;
        int bearingX, bearingY;
        int advance;
        std::vector<uint8_t> bitmap;  // Grayscale bitmap
    };

    struct FontInfo {
        std::string name;
        int size;
        int weight;
        int ascent;
        std::map<char, Glyph> glyphs;
    };

    SoftwareRenderer(SharedFrameBuffer* buffer, uint32_t width, uint32_t height)
        : m_buffer(buffer), m_width(width), m_height(height)
    {
        // Initialize FreeType
        if (FT_Init_FreeType(&m_ft_library)) {
            std::cerr << "Failed to initialize FreeType" << std::endl;
        } else {
            std::cout << "[SoftwareRenderer] FreeType initialized" << std::endl;
        }

        // Clear buffer
        std::memset(m_buffer->pixels, 0, m_width * m_height * 4);
    }

    ~SoftwareRenderer() {
        // Clean up FreeType faces
        for (auto& facePair : m_ft_faces) {
            FT_Done_Face(facePair.second);
        }
        if (m_ft_library) {
            FT_Done_FreeType(m_ft_library);
        }
    }

    void RenderHTML(const std::string& html) {
        try {
            std::cout << "[SoftwareRenderer] Creating document from HTML (" << html.size() << " bytes)" << std::endl;
            // Create document
            m_document = litehtml::document::createFromString(html.c_str(), this);
            if (m_document) {
                std::cout << "[SoftwareRenderer] Rendering document" << std::endl;
                m_document->render(m_width);
                std::cout << "[SoftwareRenderer] Drawing to buffer" << std::endl;
                RenderToBuffer();
                std::cout << "[SoftwareRenderer] Render complete" << std::endl;
            } else {
                std::cerr << "[SoftwareRenderer] Failed to create document" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[SoftwareRenderer] Exception: " << e.what() << std::endl;
        }
    }

    void RenderToBuffer() {
        // Clear buffer
        std::memset(m_buffer->pixels, 0, m_width * m_height * 4);

        // Draw document
        litehtml::position clip(0, 0, m_width, m_height);
        m_document->draw((litehtml::uint_ptr)0, 0, 0, &clip);
    }

    // Minimal litehtml::document_container implementation
    litehtml::uint_ptr create_font(const litehtml::font_description& descr,
                                   const litehtml::document* doc,
                                   litehtml::font_metrics* fm) override {
        litehtml::uint_ptr font_id = m_next_font_id++;
        FontInfo fontInfo;
        fontInfo.name = descr.family;
        fontInfo.size = descr.size;
        fontInfo.weight = descr.weight;

        // Load font with FreeType
        std::string fontPath = "/System/Library/Fonts/Helvetica.ttc"; // macOS default

        FT_Face face = nullptr;
        auto it = m_ft_faces.find(fontPath);
        if (it != m_ft_faces.end()) {
            face = it->second;
        } else if (!FT_New_Face(m_ft_library, fontPath.c_str(), 0, &face)) {
            m_ft_faces[fontPath] = face;
            std::cout << "[SoftwareRenderer] Loaded font: " << fontPath << std::endl;
        }

        if (face) {
            FT_Set_Pixel_Sizes(face, 0, descr.size);
            fontInfo.ascent = (face->size->metrics.ascender >> 6);

            // Load ASCII characters
            for (unsigned char c = 32; c < 127; c++) {
                if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                    continue;
                }

                Glyph glyph;
                glyph.width = face->glyph->bitmap.width;
                glyph.height = face->glyph->bitmap.rows;
                glyph.bearingX = face->glyph->bitmap_left;
                glyph.bearingY = face->glyph->bitmap_top;
                glyph.advance = face->glyph->advance.x;

                // Copy bitmap data
                size_t bitmapSize = glyph.width * glyph.height;
                glyph.bitmap.resize(bitmapSize);
                if (bitmapSize > 0) {
                    std::memcpy(glyph.bitmap.data(), face->glyph->bitmap.buffer, bitmapSize);
                }

                fontInfo.glyphs[c] = glyph;
            }

            std::cout << "[SoftwareRenderer] Loaded " << fontInfo.glyphs.size() << " glyphs at size " << descr.size << std::endl;

            // Set font metrics
            if (fm) {
                fm->height = descr.size;
                fm->ascent = fontInfo.ascent;
                fm->descent = -(face->size->metrics.descender >> 6);
                fm->x_height = descr.size / 2;
                fm->draw_spaces = true;
            }
        } else {
            std::cerr << "[SoftwareRenderer] Failed to load font" << std::endl;
            fontInfo.ascent = descr.size * 0.8f;
        }

        m_fonts[font_id] = fontInfo;
        return font_id;
    }

    void delete_font(litehtml::uint_ptr hFont) override {
        m_fonts.erase(hFont);
    }

    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr hFont) override {
        auto it = m_fonts.find(hFont);
        if (it == m_fonts.end() || !text)
            return 0;

        const FontInfo& font = it->second;
        if (font.glyphs.empty())
            return 0;

        // Calculate actual text width using glyph advance values
        float width = 0;
        for (const char* c = text; *c; c++) {
            auto glyph_it = font.glyphs.find(*c);
            if (glyph_it != font.glyphs.end()) {
                width += (glyph_it->second.advance >> 6);
            }
        }
        return width;
    }

    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
                   litehtml::web_color color, const litehtml::position& pos) override {
        if (!text || !*text) return;

        auto it = m_fonts.find(hFont);
        if (it == m_fonts.end()) return;

        const FontInfo& font = it->second;
        if (font.glyphs.empty()) return;

        // Y-axis is inverted - pos.y appears at bottom, so add height to get to visual top
        // Then subtract ascent to get baseline
        float baseline_y = pos.y + pos.height - font.ascent;
        float x = pos.x;

        for (const char* c = text; *c; c++) {
            auto glyph_it = font.glyphs.find(*c);
            if (glyph_it == font.glyphs.end()) continue;

            const Glyph& glyph = glyph_it->second;

            // Calculate position
            int xpos = x + glyph.bearingX;
            // When bitmap is flipped, the formula reverses: baseline = ypos + (height - bearingY)
            // So: ypos = baseline - height + bearingY
            int ypos = baseline_y - glyph.height + glyph.bearingY;

            // Draw glyph bitmap
            DrawGlyph(glyph, xpos, ypos, color);

            // Advance to next character
            x += (glyph.advance >> 6);
        }
    }

    void DrawGlyph(const Glyph& glyph, int x, int y, litehtml::web_color color) {
        if (glyph.bitmap.empty()) return;

        // Render glyph bitmap to pixel buffer
        for (int py = 0; py < glyph.height; py++) {
            for (int px = 0; px < glyph.width; px++) {
                int screenX = x + px;
                int screenY = y + py;

                // Bounds check
                if (screenX < 0 || screenX >= (int)m_width ||
                    screenY < 0 || screenY >= (int)m_height) {
                    continue;
                }

                // Flip bitmap vertically when reading
                uint8_t alpha = glyph.bitmap[(glyph.height - 1 - py) * glyph.width + px];
                if (alpha == 0) continue;

                // Get pixel in buffer
                uint8_t* pixel = &m_buffer->pixels[(screenY * m_width + screenX) * 4];

                // Alpha blend with text color
                float a = alpha / 255.0f;
                pixel[0] = (uint8_t)(color.red * a + pixel[0] * (1.0f - a));
                pixel[1] = (uint8_t)(color.green * a + pixel[1] * (1.0f - a));
                pixel[2] = (uint8_t)(color.blue * a + pixel[2] * (1.0f - a));
                pixel[3] = std::max(pixel[3], alpha);
            }
        }
    }

    litehtml::pixel_t pt_to_px(float pt) const override {
        return (pt * 96.0f) / 72.0f;
    }

    litehtml::pixel_t get_default_font_size() const override {
        return 16;
    }

    const char* get_default_font_name() const override {
        return "Arial";
    }

    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override {}

    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override {}

    void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override {
        sz.width = 100;
        sz.height = 100;
    }

    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                   const std::string& url, const std::string& base_url) override {}

    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                        const litehtml::web_color& color) override {
        DrawRect(layer.border_box.x, layer.border_box.y,
                layer.border_box.width, layer.border_box.height, color);
    }

    void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                             const litehtml::background_layer::linear_gradient& gradient) override {
        if (!gradient.color_points.empty()) {
            DrawRect(layer.border_box.x, layer.border_box.y,
                    layer.border_box.width, layer.border_box.height,
                    gradient.color_points[0].color);
        }
    }

    void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                             const litehtml::background_layer::radial_gradient& gradient) override {
        if (!gradient.color_points.empty()) {
            DrawRect(layer.border_box.x, layer.border_box.y,
                    layer.border_box.width, layer.border_box.height,
                    gradient.color_points[0].color);
        }
    }

    void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                            const litehtml::background_layer::conic_gradient& gradient) override {
        if (!gradient.color_points.empty()) {
            DrawRect(layer.border_box.x, layer.border_box.y,
                    layer.border_box.width, layer.border_box.height,
                    gradient.color_points[0].color);
        }
    }

    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
                     const litehtml::position& draw_pos, bool root) override {
        // Draw all four borders
        if (borders.top.width > 0)
            DrawBorder(draw_pos, borders.top, 0);
        if (borders.right.width > 0)
            DrawBorder(draw_pos, borders.right, 1);
        if (borders.bottom.width > 0)
            DrawBorder(draw_pos, borders.bottom, 2);
        if (borders.left.width > 0)
            DrawBorder(draw_pos, borders.left, 3);
    }

    void set_caption(const char* caption) override {}
    void set_base_url(const char* base_url) override {}
    void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override {}
    void on_anchor_click(const char* url, const litehtml::element::ptr& el) override {}
    void on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) override {}
    void set_cursor(const char* cursor) override {}
    void transform_text(litehtml::string& text, litehtml::text_transform tt) override {}
    void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override {}
    void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override {}
    void del_clip() override {}

    void get_viewport(litehtml::position& viewport) const override {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = m_width;
        viewport.height = m_height;
    }

    std::shared_ptr<litehtml::element> create_element(const char *tag_name,
                                                      const litehtml::string_map &attributes,
                                                      const std::shared_ptr<litehtml::document> &doc) override {
        return nullptr;
    }

    void get_media_features(litehtml::media_features& media) const override {
        media.type = litehtml::media_type_screen;
        media.width = m_width;
        media.height = m_height;
        media.device_width = m_width;
        media.device_height = m_height;
        media.color = 8;
        media.monochrome = 0;
        media.color_index = 256;
        media.resolution = 96;
    }

    void get_language(litehtml::string& language, litehtml::string& culture) const override {
        language = "en";
        culture = "";
    }

private:
    void DrawRect(int x, int y, int width, int height, litehtml::web_color color) {
        // Debug first few rect draws
        static int rectCallCount = 0;
        if (rectCallCount < 5) {
            std::cout << "[SoftwareRenderer] DrawRect at (" << x << "," << y
                      << ") size=" << width << "x" << height
                      << " color=(" << (int)color.red << "," << (int)color.green
                      << "," << (int)color.blue << "," << (int)color.alpha << ")" << std::endl;
            rectCallCount++;
        }

        // Clamp to buffer bounds
        if (x < 0) { width += x; x = 0; }
        if (y < 0) { height += y; y = 0; }
        if (x >= (int)m_width || y >= (int)m_height) return;
        if (x + width > (int)m_width) width = m_width - x;
        if (y + height > (int)m_height) height = m_height - y;
        if (width <= 0 || height <= 0) return;

        uint8_t r = color.red;
        uint8_t g = color.green;
        uint8_t b = color.blue;
        uint8_t a = color.alpha;

        // Simple alpha blending
        for (int py = y; py < y + height; py++) {
            for (int px = x; px < x + width; px++) {
                uint8_t* pixel = &m_buffer->pixels[(py * m_width + px) * 4];

                if (a == 255) {
                    pixel[0] = r;
                    pixel[1] = g;
                    pixel[2] = b;
                    pixel[3] = a;
                } else {
                    // Alpha blend
                    float alpha = a / 255.0f;
                    pixel[0] = (uint8_t)(r * alpha + pixel[0] * (1.0f - alpha));
                    pixel[1] = (uint8_t)(g * alpha + pixel[1] * (1.0f - alpha));
                    pixel[2] = (uint8_t)(b * alpha + pixel[2] * (1.0f - alpha));
                    pixel[3] = std::max(pixel[3], a);
                }
            }
        }
    }

    void DrawBorder(const litehtml::position& pos, const litehtml::border& border, int side) {
        if (border.width <= 0) return;

        int x, y, w, h;
        switch(side) {
            case 0: // top
                x = pos.x; y = pos.y; w = pos.width; h = border.width;
                break;
            case 1: // right
                x = pos.x + pos.width - border.width; y = pos.y;
                w = border.width; h = pos.height;
                break;
            case 2: // bottom
                x = pos.x; y = pos.y + pos.height - border.width;
                w = pos.width; h = border.width;
                break;
            case 3: // left
                x = pos.x; y = pos.y; w = border.width; h = pos.height;
                break;
            default:
                return;
        }

        DrawRect(x, y, w, h, border.color);
    }

    SharedFrameBuffer* m_buffer;
    uint32_t m_width;
    uint32_t m_height;
    litehtml::document::ptr m_document;
    FT_Library m_ft_library = nullptr;
    std::map<std::string, FT_Face> m_ft_faces;
    std::map<litehtml::uint_ptr, FontInfo> m_fonts;
    litehtml::uint_ptr m_next_font_id = 1;
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: html_render_process <shm_name> <socket_path> <width>x<height>" << std::endl;
        return 1;
    }

    std::string shmName = argv[1];
    std::string socketPath = argv[2];
    std::string sizeStr = argv[3];

    // Parse size
    size_t xPos = sizeStr.find('x');
    if (xPos == std::string::npos) {
        std::cerr << "Invalid size format. Expected WIDTHxHEIGHT" << std::endl;
        return 1;
    }

    uint32_t width = std::stoi(sizeStr.substr(0, xPos));
    uint32_t height = std::stoi(sizeStr.substr(xPos + 1));

    try {
        std::cout << "[RenderProcess] Starting HTML render process" << std::endl;
        std::cout << "[RenderProcess] SHM: " << shmName << ", Socket: " << socketPath << std::endl;
        std::cout << "[RenderProcess] Size: " << width << "x" << height << std::endl;

        // Attach to shared memory
        SharedMemory shm(shmName, width, height, false);
        SharedFrameBuffer* buffer = shm.GetBuffer();

        // Connect to IPC socket
        UnixSocket socket;
        socket.Connect(socketPath);

        // Create renderer
        SoftwareRenderer renderer(buffer, width, height);

        // Send ready message
        socket.SendMessage(CreateReadyMessage());

        std::cout << "[RenderProcess] Ready and waiting for commands" << std::endl;

        // Main loop
        bool running = true;
        std::string currentHTML;
        bool needsRender = false;

        while (running) {
            // Check for messages (non-blocking)
            IPCMessage msg;
            if (socket.TryReceiveMessage(msg)) {
                switch (msg.header.type) {
                    case MessageType::LOAD_HTML:
                        std::cout << "[RenderProcess] Loading HTML" << std::endl;
                        currentHTML = msg.payload.loadHTML.html;
                        needsRender = true;
                        break;

                    case MessageType::RESIZE:
                        std::cout << "[RenderProcess] Resize to " << msg.payload.resize.width
                                  << "x" << msg.payload.resize.height << std::endl;
                        width = msg.payload.resize.width;
                        height = msg.payload.resize.height;
                        needsRender = true;
                        break;

                    case MessageType::MARK_DIRTY:
                        needsRender = true;
                        break;

                    case MessageType::SHUTDOWN:
                        std::cout << "[RenderProcess] Shutdown requested" << std::endl;
                        running = false;
                        break;

                    default:
                        break;
                }
            }

            // Render if needed
            if (needsRender && !currentHTML.empty()) {
                buffer->ready = 0; // Mark as being written

                renderer.RenderHTML(currentHTML);

                buffer->frameNumber++;
                buffer->ready = 1; // Mark as ready

                needsRender = false;
            }

            // Sleep to avoid busy-waiting (target 60fps for animations)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        std::cout << "[RenderProcess] Exiting" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[RenderProcess] Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
