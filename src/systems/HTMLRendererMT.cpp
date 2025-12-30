#include <glad/glad.h>
#include "systems/HTMLRendererMT.h"

#include <iostream>
#include <cstring>
#include <algorithm>
#include <chrono>

// Software rendering document container for litehtml
class HTMLRendererMT::SoftwareRenderer : public litehtml::document_container
{
public:
    struct Glyph
    {
        int width, height;
        int bearingX, bearingY;
        int advance;
        std::vector<uint8_t> bitmap; // Grayscale bitmap
    };

    struct FontInfo
    {
        std::string name;
        int size;
        int weight;
        int ascent;
        std::map<char, Glyph> glyphs;
    };

    SoftwareRenderer(FrameBuffer *buffer)
        : m_buffer(buffer)
    {
        // Initialize FreeType
        if (FT_Init_FreeType(&m_ft_library))
        {
            std::cerr << "[SoftwareRenderer] Failed to initialize FreeType" << std::endl;
        }
        else
        {
            std::cout << "[SoftwareRenderer] FreeType initialized" << std::endl;
        }
    }

    ~SoftwareRenderer()
    {
        // Clean up FreeType faces
        for (auto &facePair : m_ft_faces)
        {
            FT_Done_Face(facePair.second);
        }
        if (m_ft_library)
        {
            FT_Done_FreeType(m_ft_library);
        }
    }

    void RenderHTML(const std::string &html)
    {
        try
        {
            std::cout << "[SoftwareRenderer] Creating document from HTML (" << html.size() << " bytes)" << std::endl;

            // Create document
            m_document = litehtml::document::createFromString(html.c_str(), this);
            if (m_document)
            {
                std::cout << "[SoftwareRenderer] Rendering document" << std::endl;
                m_document->render(m_buffer->width);
                std::cout << "[SoftwareRenderer] Drawing to buffer" << std::endl;
                RenderToBuffer();
                std::cout << "[SoftwareRenderer] Render complete" << std::endl;
            }
            else
            {
                std::cerr << "[SoftwareRenderer] Failed to create document" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[SoftwareRenderer] Exception: " << e.what() << std::endl;
        }
    }

    void RenderToBuffer()
    {
        // Clear buffer
        std::memset(m_buffer->pixels.data(), 0, m_buffer->pixels.size());

        // Draw document
        litehtml::position clip(0, 0, m_buffer->width, m_buffer->height);
        m_document->draw((litehtml::uint_ptr)0, 0, 0, &clip);
    }

    // Minimal litehtml::document_container implementation
    litehtml::uint_ptr create_font(const litehtml::font_description &descr,
                                   const litehtml::document *doc,
                                   litehtml::font_metrics *fm) override
    {
        // Check if we already have this font cached (by name + size)
        std::string fontKey = descr.family + "_" + std::to_string(descr.size);
        auto cachedFont = m_fontCache.find(fontKey);
        if (cachedFont != m_fontCache.end())
        {
            litehtml::uint_ptr font_id = m_next_font_id++;
            m_fonts[font_id] = cachedFont->second;

            // Set font metrics
            if (fm)
            {
                fm->height = descr.size;
                fm->ascent = cachedFont->second.ascent;
                fm->descent = -(descr.size - cachedFont->second.ascent);
                fm->x_height = descr.size / 2;
                fm->draw_spaces = true;
            }

            return font_id;
        }

        litehtml::uint_ptr font_id = m_next_font_id++;
        FontInfo fontInfo;
        fontInfo.name = descr.family;
        fontInfo.size = descr.size;
        fontInfo.weight = descr.weight;

        // Load font with FreeType - use Menlo for monospace
        std::string fontPath = "/System/Library/Fonts/Menlo.ttc"; // macOS monospace default

        FT_Face face = nullptr;
        auto it = m_ft_faces.find(fontPath);
        if (it != m_ft_faces.end())
        {
            face = it->second;
        }
        else if (!FT_New_Face(m_ft_library, fontPath.c_str(), 0, &face))
        {
            m_ft_faces[fontPath] = face;
            std::cout << "[SoftwareRenderer] Loaded font face: " << fontPath << std::endl;
        }

        if (face)
        {
            auto glyphLoadStart = std::chrono::high_resolution_clock::now();

            FT_Set_Pixel_Sizes(face, 0, descr.size);
            fontInfo.ascent = (face->size->metrics.ascender >> 6);

            // Load ASCII characters
            for (unsigned char c = 32; c < 127; c++)
            {
                if (FT_Load_Char(face, c, FT_LOAD_RENDER))
                {
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
                if (bitmapSize > 0)
                {
                    std::memcpy(glyph.bitmap.data(), face->glyph->bitmap.buffer, bitmapSize);
                }

                fontInfo.glyphs[c] = glyph;
            }

            auto glyphLoadEnd = std::chrono::high_resolution_clock::now();
            auto glyphLoadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(glyphLoadEnd - glyphLoadStart).count();
            std::cout << "[SoftwareRenderer] Loaded " << fontInfo.glyphs.size() << " glyphs at size " << descr.size << " in " << glyphLoadDuration << "ms" << std::endl;

            // Set font metrics
            if (fm)
            {
                fm->height = descr.size;
                fm->ascent = fontInfo.ascent;
                fm->descent = -(face->size->metrics.descender >> 6);
                fm->x_height = descr.size / 2;
                fm->draw_spaces = true;
            }
        }
        else
        {
            std::cerr << "[SoftwareRenderer] Failed to load font" << std::endl;
            fontInfo.ascent = descr.size * 0.8f;
        }

        m_fonts[font_id] = fontInfo;
        m_fontCache[fontKey] = fontInfo;  // Cache for reuse
        return font_id;
    }

    void delete_font(litehtml::uint_ptr hFont) override
    {
        m_fonts.erase(hFont);
    }

    litehtml::pixel_t text_width(const char *text, litehtml::uint_ptr hFont) override
    {
        auto it = m_fonts.find(hFont);
        if (it == m_fonts.end() || !text)
            return 0;

        const FontInfo &font = it->second;
        if (font.glyphs.empty())
            return 0;

        // Calculate actual text width using glyph advance values
        float width = 0;
        for (const char *c = text; *c; c++)
        {
            auto glyph_it = font.glyphs.find(*c);
            if (glyph_it != font.glyphs.end())
            {
                width += (glyph_it->second.advance >> 6);
            }
        }
        return width;
    }

    void draw_text(litehtml::uint_ptr hdc, const char *text, litehtml::uint_ptr hFont,
                   litehtml::web_color color, const litehtml::position &pos) override
    {
        if (!text || !*text)
            return;

        auto it = m_fonts.find(hFont);
        if (it == m_fonts.end())
            return;

        const FontInfo &font = it->second;
        if (font.glyphs.empty())
            return;

        // Y-axis is inverted - pos.y appears at bottom, so add height to get to visual top
        // Then subtract ascent to get baseline
        float baseline_y = pos.y + pos.height - font.ascent;
        float x = pos.x;

        for (const char *c = text; *c; c++)
        {
            auto glyph_it = font.glyphs.find(*c);
            if (glyph_it == font.glyphs.end())
                continue;

            const Glyph &glyph = glyph_it->second;

            // Calculate position
            int xpos = x + glyph.bearingX;
            int ypos = baseline_y - glyph.height + glyph.bearingY;

            // Draw glyph bitmap
            DrawGlyph(glyph, xpos, ypos, color);

            // Advance to next character
            x += (glyph.advance >> 6);
        }
    }

    void DrawGlyph(const Glyph &glyph, int x, int y, litehtml::web_color color)
    {
        if (glyph.bitmap.empty())
            return;

        // Render glyph bitmap to pixel buffer
        for (int py = 0; py < glyph.height; py++)
        {
            for (int px = 0; px < glyph.width; px++)
            {
                int screenX = x + px;
                int screenY = y + py;

                // Bounds check
                if (screenX < 0 || screenX >= (int)m_buffer->width ||
                    screenY < 0 || screenY >= (int)m_buffer->height)
                {
                    continue;
                }

                // Flip bitmap vertically when reading
                uint8_t alpha = glyph.bitmap[(glyph.height - 1 - py) * glyph.width + px];
                if (alpha == 0)
                    continue;

                // Get pixel in buffer
                uint8_t *pixel = &m_buffer->pixels[(screenY * m_buffer->width + screenX) * 4];

                // Alpha blend with text color
                float a = alpha / 255.0f;
                pixel[0] = (uint8_t)(color.red * a + pixel[0] * (1.0f - a));
                pixel[1] = (uint8_t)(color.green * a + pixel[1] * (1.0f - a));
                pixel[2] = (uint8_t)(color.blue * a + pixel[2] * (1.0f - a));
                pixel[3] = std::max(pixel[3], alpha);
            }
        }
    }

    litehtml::pixel_t pt_to_px(float pt) const override
    {
        return (pt * 96.0f) / 72.0f;
    }

    litehtml::pixel_t get_default_font_size() const override
    {
        return 16;
    }

    const char *get_default_font_name() const override
    {
        return "Arial";
    }

    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker &marker) override {}

    void load_image(const char *src, const char *baseurl, bool redraw_on_ready) override {}

    void get_image_size(const char *src, const char *baseurl, litehtml::size &sz) override
    {
        sz.width = 100;
        sz.height = 100;
    }

    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                    const std::string &url, const std::string &base_url) override {}

    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                         const litehtml::web_color &color) override
    {
        DrawRect(layer.border_box.x, layer.border_box.y,
                 layer.border_box.width, layer.border_box.height, color);
    }

    void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                              const litehtml::background_layer::linear_gradient &gradient) override
    {
        if (!gradient.color_points.empty())
        {
            DrawRect(layer.border_box.x, layer.border_box.y,
                     layer.border_box.width, layer.border_box.height,
                     gradient.color_points[0].color);
        }
    }

    void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                              const litehtml::background_layer::radial_gradient &gradient) override
    {
        if (!gradient.color_points.empty())
        {
            DrawRect(layer.border_box.x, layer.border_box.y,
                     layer.border_box.width, layer.border_box.height,
                     gradient.color_points[0].color);
        }
    }

    void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                             const litehtml::background_layer::conic_gradient &gradient) override
    {
        if (!gradient.color_points.empty())
        {
            DrawRect(layer.border_box.x, layer.border_box.y,
                     layer.border_box.width, layer.border_box.height,
                     gradient.color_points[0].color);
        }
    }

    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders &borders,
                      const litehtml::position &draw_pos, bool root) override
    {
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

    void set_caption(const char *caption) override {}
    void set_base_url(const char *base_url) override {}
    void link(const std::shared_ptr<litehtml::document> &doc, const litehtml::element::ptr &el) override {}
    void on_anchor_click(const char *url, const litehtml::element::ptr &el) override {}
    void on_mouse_event(const litehtml::element::ptr &el, litehtml::mouse_event event) override {}
    void set_cursor(const char *cursor) override {}
    void transform_text(litehtml::string &text, litehtml::text_transform tt) override {}
    void import_css(litehtml::string &text, const litehtml::string &url, litehtml::string &baseurl) override {}
    void set_clip(const litehtml::position &pos, const litehtml::border_radiuses &bdr_radius) override {}
    void del_clip() override {}

    void get_viewport(litehtml::position &viewport) const override
    {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = m_buffer->width;
        viewport.height = m_buffer->height;
    }

    std::shared_ptr<litehtml::element> create_element(const char *tag_name,
                                                      const litehtml::string_map &attributes,
                                                      const std::shared_ptr<litehtml::document> &doc) override
    {
        return nullptr;
    }

    void get_media_features(litehtml::media_features &media) const override
    {
        media.type = litehtml::media_type_screen;
        media.width = m_buffer->width;
        media.height = m_buffer->height;
        media.device_width = m_buffer->width;
        media.device_height = m_buffer->height;
        media.color = 8;
        media.monochrome = 0;
        media.color_index = 256;
        media.resolution = 96;
    }

    void get_language(litehtml::string &language, litehtml::string &culture) const override
    {
        language = "en";
        culture = "";
    }

private:
    void DrawRect(int x, int y, int width, int height, litehtml::web_color color)
    {
        // Clamp to buffer bounds
        if (x < 0)
        {
            width += x;
            x = 0;
        }
        if (y < 0)
        {
            height += y;
            y = 0;
        }
        if (x >= (int)m_buffer->width || y >= (int)m_buffer->height)
            return;
        if (x + width > (int)m_buffer->width)
            width = m_buffer->width - x;
        if (y + height > (int)m_buffer->height)
            height = m_buffer->height - y;
        if (width <= 0 || height <= 0)
            return;

        uint8_t r = color.red;
        uint8_t g = color.green;
        uint8_t b = color.blue;
        uint8_t a = color.alpha;

        // Simple alpha blending
        for (int py = y; py < y + height; py++)
        {
            for (int px = x; px < x + width; px++)
            {
                uint8_t *pixel = &m_buffer->pixels[(py * m_buffer->width + px) * 4];

                if (a == 255)
                {
                    pixel[0] = r;
                    pixel[1] = g;
                    pixel[2] = b;
                    pixel[3] = a;
                }
                else
                {
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

    void DrawBorder(const litehtml::position &pos, const litehtml::border &border, int side)
    {
        if (border.width <= 0)
            return;

        int x, y, w, h;
        switch (side)
        {
        case 0: // top
            x = pos.x;
            y = pos.y;
            w = pos.width;
            h = border.width;
            break;
        case 1: // right
            x = pos.x + pos.width - border.width;
            y = pos.y;
            w = border.width;
            h = pos.height;
            break;
        case 2: // bottom
            x = pos.x;
            y = pos.y + pos.height - border.width;
            w = pos.width;
            h = border.width;
            break;
        case 3: // left
            x = pos.x;
            y = pos.y;
            w = border.width;
            h = pos.height;
            break;
        default:
            return;
        }

        DrawRect(x, y, w, h, border.color);
    }

    FrameBuffer *m_buffer;
    FT_Library m_ft_library = nullptr;
    std::map<std::string, FT_Face> m_ft_faces;
    std::map<litehtml::uint_ptr, FontInfo> m_fonts;
    std::map<std::string, FontInfo> m_fontCache;  // Cache fonts by "family_size"
    litehtml::uint_ptr m_next_font_id = 1;

    // IMPORTANT: m_document must be declared AFTER m_fonts/m_fontCache
    // because its destructor calls delete_font() which needs those maps
    litehtml::document::ptr m_document;
};

// ============================================================================
// HTMLRendererMT Implementation
// ============================================================================

void HTMLRendererMT::Initialize(GLFWwindow *window, int width, int height)
{
    m_window = window;
    m_width = width;
    m_height = height;

    std::cout << "[HTMLRendererMT] Initializing multi-threaded HTML renderer" << std::endl;

    try
    {
        // Initialize buffers
        m_frontBuffer.width = width;
        m_frontBuffer.height = height;
        m_frontBuffer.frameNumber = 0;
        m_frontBuffer.pixels.resize(width * height * 4, 0);

        m_backBuffer.width = width;
        m_backBuffer.height = height;
        m_backBuffer.frameNumber = 0;
        m_backBuffer.pixels.resize(width * height * 4, 0);

        // Set up OpenGL resources
        SetupGL();

        m_projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);

        // Start render thread
        m_running = true;
        m_renderThread = std::make_unique<std::thread>(&HTMLRendererMT::RenderThreadLoop, this);

        std::cout << "[HTMLRendererMT] Initialization complete" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[HTMLRendererMT] Initialization failed: " << e.what() << std::endl;
        Shutdown();
        throw;
    }
}

void HTMLRendererMT::SetupGL()
{
    // Create composite shader
    m_compositeShader = new Shader("../res/shaders/Composite.shader");

    // Create texture for uploading pixels
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create quad for rendering texture (screen space coordinates)
    float quadVertices[] = {
        // positions (screen space)        // texCoords
        0.0f, 0.0f, 0.0f, 1.0f,                      // top-left
        0.0f, (float)m_height, 0.0f, 0.0f,           // bottom-left
        (float)m_width, (float)m_height, 1.0f, 0.0f, // bottom-right

        0.0f, 0.0f, 0.0f, 1.0f,                      // top-left
        (float)m_width, (float)m_height, 1.0f, 0.0f, // bottom-right
        (float)m_width, 0.0f, 1.0f, 1.0f             // top-right
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void HTMLRendererMT::LoadHTML(const std::string &html)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "[HTMLRendererMT] Loading HTML (" << html.size() << " bytes)" << std::endl;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingHTML = html;
        m_hasNewHTML = true;
    }
    m_cv.notify_one();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[HTMLRendererMT] LoadHTML took " << duration << "ms" << std::endl;
}

void HTMLRendererMT::UpdateHTML(const std::string &html)
{
    // Same as LoadHTML
    LoadHTML(html);
}

void HTMLRendererMT::Render()
{
    if (!m_compositeShader)
        return;

    // Check if new frame is ready
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_frontBuffer.frameNumber != m_lastFrameNumber)
        {
            std::cout << "[HTMLRendererMT] Uploading new frame " << m_frontBuffer.frameNumber << std::endl;
            UpdateTextureFromPixelBuffer();
            m_lastFrameNumber = m_frontBuffer.frameNumber;
        }
    }

    // Set up OpenGL state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use composite shader
    m_compositeShader->Use();
    m_compositeShader->SetMat4("projection", m_projection);
    m_compositeShader->SetInt("screenTexture", 0);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Draw quad with texture
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Cleanup
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void HTMLRendererMT::UpdateTextureFromPixelBuffer()
{
    // Upload pixels to texture
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_frontBuffer.width, m_frontBuffer.height,
                    GL_RGBA, GL_UNSIGNED_BYTE, m_frontBuffer.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HTMLRendererMT::Resize(int width, int height)
{
    std::cout << "[HTMLRendererMT] Resize to " << width << "x" << height << std::endl;

    m_width = width;
    m_height = height;

    // Update projection matrix
    m_projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);

    // Notify render thread
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_newWidth = width;
        m_newHeight = height;
        m_needsResize = true;
    }
    m_cv.notify_one();

    // Recreate texture
    if (m_texture)
    {
        glDeleteTextures(1, &m_texture);
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Update quad vertices for new size
    float quadVertices[] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, (float)m_height, 0.0f, 0.0f,
        (float)m_width, (float)m_height, 1.0f, 0.0f,

        0.0f, 0.0f, 0.0f, 1.0f,
        (float)m_width, (float)m_height, 1.0f, 0.0f,
        (float)m_width, 0.0f, 1.0f, 1.0f};

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void HTMLRendererMT::Shutdown()
{
    if (m_isShutdown)
        return; // Already shut down
    m_isShutdown = true;

    std::cout << "[HTMLRendererMT] Shutting down" << std::endl;

    // Stop render thread
    if (m_renderThread)
    {
        m_running = false;
        m_cv.notify_one();
        if (m_renderThread->joinable())
        {
            m_renderThread->join();
        }
        m_renderThread.reset();
    }

    // Clean up OpenGL resources
    if (m_texture)
    {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    if (m_quadVAO)
    {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO)
    {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    if (m_compositeShader)
    {
        delete m_compositeShader;
        m_compositeShader = nullptr;
    }

    std::cout << "[HTMLRendererMT] Shutdown complete" << std::endl;
}

void HTMLRendererMT::RenderThreadLoop()
{
    std::cout << "[RenderThread] Starting" << std::endl;

    // Create renderer
    SoftwareRenderer renderer(&m_backBuffer);

    std::string currentHTML;
    bool needsRender = false;

    while (m_running)
    {
        // Wait for work
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]
                      { return !m_running || m_hasNewHTML || m_needsResize; });

            if (!m_running)
                break;

            // Handle resize
            if (m_needsResize)
            {
                m_backBuffer.width = m_newWidth;
                m_backBuffer.height = m_newHeight;
                m_backBuffer.pixels.resize(m_newWidth * m_newHeight * 4, 0);
                m_needsResize = false;
                needsRender = !currentHTML.empty();
            }

            // Handle new HTML
            if (m_hasNewHTML)
            {
                currentHTML = m_pendingHTML;
                m_hasNewHTML = false;
                needsRender = true;
            }
        }

        // Render if needed
        if (needsRender && !currentHTML.empty())
        {
            auto renderStart = std::chrono::high_resolution_clock::now();
            std::cout << "[RenderThread] Rendering HTML" << std::endl;

            renderer.RenderHTML(currentHTML);

            auto renderEnd = std::chrono::high_resolution_clock::now();
            auto renderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(renderEnd - renderStart).count();

            // Swap buffers
            {
                std::lock_guard<std::mutex> lock(m_bufferMutex);
                std::swap(m_frontBuffer, m_backBuffer);
                m_frontBuffer.frameNumber++;
            }

            needsRender = false;
            auto totalEnd = std::chrono::high_resolution_clock::now();
            auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - renderStart).count();
            std::cout << "[RenderThread] Render complete (render: " << renderDuration << "ms, total: " << totalDuration << "ms)" << std::endl;
        }
    }

    std::cout << "[RenderThread] Exiting" << std::endl;
}
