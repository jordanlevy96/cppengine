/**
 * @file HTMLRenderer.cpp
 * @brief LEGACY: Single-threaded HTML renderer (superseded by HTMLRendererMT)
 * @lines ~835
 *
 * Purpose: Original litehtml+FreeType renderer running on main thread.
 * Kept for reference but no longer used in production code.
 *
 * Key functions:
 * - Initialize() - Setup GL resources (line 5, ~20 lines)
 * - LoadHTML() - Parse and render HTML+CSS (line 113, ~20 lines)
 * - Render() - Draw to screen (line 133, ~15 lines)
 * - draw_text() - FreeType text rasterization (line 258, ~30 lines)
 * - draw_solid_fill() - Fill rectangles (line 322, ~10 lines)
 * - draw_borders() - Border rendering (line 440, ~100 lines)
 *
 * Limitations (why HTMLRendererMT replaced it):
 * - Blocks main thread during render (~5-15ms)
 * - No async rendering
 * - FreeType calls on main thread (stutters)
 *
 * Migration status: Fully replaced by HTMLRendererMT in all modes (Game, Editor)
 * Removal: TODO - can be deleted once HTMLRendererMT is proven stable
 *
 * @deprecated Use HTMLRendererMT instead
 */

#include <glad/glad.h>
#include "systems/HTMLRenderer.h"
#include <iostream>

void HTMLRenderer::Initialize(GLFWwindow *window, int width, int height)
{
    m_window = window;
    m_width = width;
    m_height = height;

    // Initialize FreeType
    if (FT_Init_FreeType(&m_ft_library))
    {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    SetupRenderingResources();

    // Set up orthographic projection for 2D rendering
    m_projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);

    std::cout << "HTMLRenderer initialized with size: " << width << "x" << height << std::endl;
}

void HTMLRenderer::SetupRenderingResources()
{
    // Create shaders
    m_shader = new Shader("../res/shaders/UI.shader");
    m_textShader = new Shader("../res/shaders/Text.shader");
    m_compositeShader = new Shader("../res/shaders/Composite.shader");

    // Create VAO and VBO for rectangle rendering
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Allocate buffer (we'll update it per draw call)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create VAO and VBO for text rendering
    glGenVertexArrays(1, &m_textVAO);
    glGenBuffers(1, &m_textVBO);

    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    // Vertex attributes: position (2) + texcoords (2)
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Configure texture parameters for glyph rendering
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    // Set up framebuffer for render-to-texture
    SetupFramebuffer();
}

void HTMLRenderer::Shutdown()
{
    m_document.reset();

    // Clean up OpenGL resources
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_textVAO) glDeleteVertexArrays(1, &m_textVAO);
    if (m_textVBO) glDeleteBuffers(1, &m_textVBO);
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_shader) delete m_shader;
    if (m_textShader) delete m_textShader;
    if (m_compositeShader) delete m_compositeShader;

    // Clean up framebuffer
    if (m_FBOTexture) glDeleteTextures(1, &m_FBOTexture);
    if (m_FBO) glDeleteFramebuffers(1, &m_FBO);

    // Clean up glyph textures
    for (auto& fontPair : m_fonts)
    {
        for (auto& glyphPair : fontPair.second.glyphs)
        {
            glDeleteTextures(1, &glyphPair.second.textureID);
        }
    }

    // Clean up FreeType
    for (auto& facePair : m_ft_faces)
    {
        FT_Done_Face(facePair.second);
    }
    if (m_ft_library)
    {
        FT_Done_FreeType(m_ft_library);
    }

    std::cout << "HTMLRenderer shutdown" << std::endl;
}

void HTMLRenderer::LoadHTML(const std::string& html, const std::string& master_css)
{
    // Create litehtml document
    // Use default master CSS if none provided
    const std::string& styles = master_css.empty() ? litehtml::master_css : master_css;
    m_document = litehtml::document::createFromString(html.c_str(), this, styles);

    if (m_document)
    {
        // Render the document with current viewport size
        m_document->render(m_width);
        m_isDirty = true; // Mark as needing re-render to FBO
        std::cout << "HTML document loaded and rendered" << std::endl;
    }
    else
    {
        std::cerr << "Failed to create HTML document" << std::endl;
    }
}

void HTMLRenderer::Render()
{
    if (!m_document || !m_shader) return;

    // Only re-render to FBO if content has changed
    if (m_isDirty)
    {
        RenderToFramebuffer();
        m_isDirty = false;
    }

    // Composite the cached FBO texture to screen (single draw call)
    RenderFramebufferToScreen();
}

void HTMLRenderer::Resize(int width, int height)
{
    m_width = width;
    m_height = height;

    // Update projection matrix
    m_projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f);

    if (m_document)
    {
        m_document->render(m_width);
        m_isDirty = true; // Need to re-render at new size
    }

    // Recreate framebuffer at new size
    if (m_FBOTexture) glDeleteTextures(1, &m_FBOTexture);
    if (m_FBO) glDeleteFramebuffers(1, &m_FBO);
    SetupFramebuffer();
}

void HTMLRenderer::MarkDirty()
{
    m_isDirty = true;
}

// ============================================================================
// litehtml::document_container interface implementation
// ============================================================================

litehtml::uint_ptr HTMLRenderer::create_font(const litehtml::font_description& descr, const litehtml::document* doc,
                                             litehtml::font_metrics* fm)
{
    litehtml::uint_ptr font_id = m_next_font_id++;
    FontInfo fontInfo;
    fontInfo.name = descr.family;
    fontInfo.size = descr.size;
    fontInfo.weight = descr.weight;

    // Load font with FreeType
    std::string fontPath = "/System/Library/Fonts/Helvetica.ttc"; // macOS default

    // Get the FreeType face to extract accurate metrics
    FT_Face face = nullptr;
    auto it = m_ft_faces.find(fontPath);
    if (it != m_ft_faces.end())
    {
        face = it->second;
    }
    else if (!FT_New_Face(m_ft_library, fontPath.c_str(), 0, &face))
    {
        m_ft_faces[fontPath] = face;
    }

    if (face)
    {
        FT_Set_Pixel_Sizes(face, 0, descr.size);

        // Store ascent for baseline calculation
        fontInfo.ascent = (face->size->metrics.ascender >> 6);

        // Set accurate font metrics from FreeType
        if (fm)
        {
            fm->height = descr.size;
            fm->ascent = fontInfo.ascent;
            fm->descent = -(face->size->metrics.descender >> 6); // Make positive
            fm->x_height = descr.size / 2;
            fm->draw_spaces = true;
        }
    }
    else
    {
        fontInfo.ascent = descr.size * 0.8f; // fallback
    }

    LoadFont(fontPath, descr.size, fontInfo);
    m_fonts[font_id] = fontInfo;

    return font_id;
}

void HTMLRenderer::delete_font(litehtml::uint_ptr hFont)
{
    m_fonts.erase(hFont);
}

litehtml::pixel_t HTMLRenderer::text_width(const char* text, litehtml::uint_ptr hFont)
{
    auto it = m_fonts.find(hFont);
    if (it == m_fonts.end() || !text)
        return 0;

    const FontInfo& font = it->second;
    if (font.glyphs.empty())
        return 0;

    // Calculate actual text width using glyph advance values
    float width = 0;
    for (const char* c = text; *c; c++)
    {
        auto glyph_it = font.glyphs.find(*c);
        if (glyph_it != font.glyphs.end())
        {
            // Advance is in 1/64th pixels, convert to pixels
            width += (glyph_it->second.advance >> 6);
        }
    }
    return width;
}

void HTMLRenderer::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
                             litehtml::web_color color, const litehtml::position& pos)
{
    auto it = m_fonts.find(hFont);
    if (it == m_fonts.end() || !text)
        return;

    const FontInfo& font = it->second;
    if (font.glyphs.empty())
        return;

    // pos.y is the top of the text box, add the ascent to get baseline position
    float baseline_y = pos.y + font.ascent;

    RenderText(text, pos.x, baseline_y, 1.0f, font, color);
}

litehtml::pixel_t HTMLRenderer::pt_to_px(float pt) const
{
    // Standard conversion: 1pt = 1.333px at 96 DPI
    return (pt * 96.0f) / 72.0f;
}

litehtml::pixel_t HTMLRenderer::get_default_font_size() const
{
    return 16; // Standard browser default
}

const char* HTMLRenderer::get_default_font_name() const
{
    return "Arial";
}

void HTMLRenderer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker)
{
    // Draw simple bullet points or numbers
    if (marker.marker_type == litehtml::list_style_type_circle ||
        marker.marker_type == litehtml::list_style_type_disc)
    {
        // Draw a small circle/disc
        DrawRect(marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color);
    }
}

void HTMLRenderer::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    // Image loading will be implemented later
    std::cout << "Image load requested: " << src << std::endl;
}

void HTMLRenderer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    // Return default image size for now
    sz.width = 100;
    sz.height = 100;
}

void HTMLRenderer::draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                             const std::string& url, const std::string& base_url)
{
    // Image rendering will be implemented later
    std::cout << "Draw image: " << url << std::endl;
}

void HTMLRenderer::draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                                  const litehtml::web_color& color)
{
    // Draw solid color background
    if (color.alpha > 0)
    {
        DrawRect(layer.border_box.x, layer.border_box.y,
                layer.border_box.width, layer.border_box.height,
                color);
    }
}

void HTMLRenderer::draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                                       const litehtml::background_layer::linear_gradient& gradient)
{
    // Linear gradient rendering will be implemented later
    // For now, just fill with the first color
    if (!gradient.color_points.empty())
    {
        DrawRect(layer.border_box.x, layer.border_box.y,
                layer.border_box.width, layer.border_box.height,
                gradient.color_points[0].color);
    }
}

void HTMLRenderer::draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                                       const litehtml::background_layer::radial_gradient& gradient)
{
    // Radial gradient rendering will be implemented later
    // For now, just fill with the first color
    if (!gradient.color_points.empty())
    {
        DrawRect(layer.border_box.x, layer.border_box.y,
                layer.border_box.width, layer.border_box.height,
                gradient.color_points[0].color);
    }
}

void HTMLRenderer::draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                                      const litehtml::background_layer::conic_gradient& gradient)
{
    // Conic gradient rendering will be implemented later
    // For now, just fill with the first color
    if (!gradient.color_points.empty())
    {
        DrawRect(layer.border_box.x, layer.border_box.y,
                layer.border_box.width, layer.border_box.height,
                gradient.color_points[0].color);
    }
}

void HTMLRenderer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
                               const litehtml::position& draw_pos, bool root)
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

void HTMLRenderer::set_caption(const char* caption)
{
    // Could update window title here if desired
}

void HTMLRenderer::set_base_url(const char* base_url)
{
    // Store base URL for relative path resolution
}

void HTMLRenderer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el)
{
    // Called when stylesheet is linked
}

void HTMLRenderer::on_anchor_click(const char* url, const litehtml::element::ptr& el)
{
    std::cout << "Anchor clicked: " << url << std::endl;
}

void HTMLRenderer::on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event)
{
    // Handle mouse enter/leave events for hover effects
}

void HTMLRenderer::set_cursor(const char* cursor)
{
    // Could change mouse cursor here
}

void HTMLRenderer::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
    // Transform text according to CSS text-transform property
    switch(tt)
    {
        case litehtml::text_transform_uppercase:
            for(auto& c : text) c = toupper(c);
            break;
        case litehtml::text_transform_lowercase:
            for(auto& c : text) c = tolower(c);
            break;
        case litehtml::text_transform_capitalize:
            // Capitalize first letter of each word
            {
                bool new_word = true;
                for(auto& c : text) {
                    if (isspace(c)) {
                        new_word = true;
                    } else if (new_word) {
                        c = toupper(c);
                        new_word = false;
                    }
                }
            }
            break;
        default:
            break;
    }
}

void HTMLRenderer::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{
    // Import external CSS - not implemented yet
}

void HTMLRenderer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius)
{
    // Set clipping region for overflow handling
    glEnable(GL_SCISSOR_TEST);
    glScissor(pos.x, m_height - pos.y - pos.height, pos.width, pos.height);
}

void HTMLRenderer::del_clip()
{
    glDisable(GL_SCISSOR_TEST);
}

void HTMLRenderer::get_viewport(litehtml::position& viewport) const
{
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_width;
    viewport.height = m_height;
}

std::shared_ptr<litehtml::element> HTMLRenderer::create_element(const char *tag_name,
                                                               const litehtml::string_map &attributes,
                                                               const std::shared_ptr<litehtml::document> &doc)
{
    // Return nullptr to use default element creation
    return nullptr;
}

void HTMLRenderer::get_media_features(litehtml::media_features& media) const
{
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

void HTMLRenderer::get_language(litehtml::string& language, litehtml::string& culture) const
{
    language = "en";
    culture = "US";
}

// ============================================================================
// Helper methods
// ============================================================================

void HTMLRenderer::LoadFont(const std::string& fontPath, litehtml::pixel_t size, FontInfo& fontInfo)
{
    // Check if we already have this face loaded
    FT_Face face;
    auto it = m_ft_faces.find(fontPath);
    if (it != m_ft_faces.end())
    {
        face = it->second;
    }
    else
    {
        // Load font face
        if (FT_New_Face(m_ft_library, fontPath.c_str(), 0, &face))
        {
            std::cerr << "ERROR::FREETYPE: Failed to load font: " << fontPath << std::endl;
            // Try to load a system font as fallback
            std::string fallback = "/System/Library/Fonts/Helvetica.ttc"; // macOS
            if (FT_New_Face(m_ft_library, fallback.c_str(), 0, &face))
            {
                std::cerr << "ERROR::FREETYPE: Failed to load fallback font" << std::endl;
                return;
            }
            std::cout << "Using fallback font: " << fallback << std::endl;
        }
        m_ft_faces[fontPath] = face;
    }

    // Set font size
    FT_Set_Pixel_Sizes(face, 0, size);

    // Load ASCII characters (32-127)
    for (unsigned char c = 32; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph " << c << std::endl;
            continue;
        }

        // Generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store glyph for later use
        Glyph glyph = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        fontInfo.glyphs[c] = glyph;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void HTMLRenderer::RenderText(const std::string& text, float x, float y, float scale,
                              const FontInfo& font, litehtml::web_color color)
{
    if (!m_textShader) return;

    // Activate corresponding render state
    m_textShader->Use();
    glm::vec4 textColor(color.red / 255.0f, color.green / 255.0f,
                       color.blue / 255.0f, color.alpha / 255.0f);
    m_textShader->SetVec4("textColor", textColor);
    m_textShader->SetMat4("projection", m_projection);
    m_textShader->SetInt("text", 0); // Set texture uniform to use texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_textVAO);

    // Iterate through all characters
    for (auto c : text)
    {
        auto it = font.glyphs.find(c);
        if (it == font.glyphs.end())
            continue;

        const Glyph& glyph = it->second;

        // Calculate position
        // x position: current x + horizontal bearing (offset from origin to left of glyph)
        float xpos = x + glyph.bearing.x * scale;
        // y position: baseline - bearing.y (bearing.y is offset from baseline to top of glyph)
        float ypos = y - glyph.bearing.y * scale;

        float w = glyph.size.x * scale;
        float h = glyph.size.y * scale;

        // Update VBO for each character
        // Flip texture coordinates vertically (FreeType uses top-left origin)
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos,     ypos,       0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 0.0f },

            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f }
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, glyph.textureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (glyph.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HTMLRenderer::DrawRect(int x, int y, int width, int height, litehtml::web_color color)
{
    if (!m_shader) return;

    // Define rectangle vertices (two triangles)
    float vertices[] = {
        // First triangle
        (float)x,         (float)y,           // Top-left
        (float)(x+width), (float)y,           // Top-right
        (float)x,         (float)(y+height),  // Bottom-left
        // Second triangle
        (float)(x+width), (float)y,           // Top-right
        (float)(x+width), (float)(y+height),  // Bottom-right
        (float)x,         (float)(y+height)   // Bottom-left
    };

    // Set color uniform (convert from 0-255 to 0.0-1.0)
    glm::vec4 colorVec(
        color.red / 255.0f,
        color.green / 255.0f,
        color.blue / 255.0f,
        color.alpha / 255.0f
    );
    m_shader->SetVec4("color", colorVec);

    // Upload vertices and draw
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void HTMLRenderer::DrawBorder(const litehtml::position& pos, const litehtml::border& border, int side)
{
    if (border.width <= 0) return;

    // Calculate border rectangle based on side
    int x, y, w, h;
    switch(side)
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

void HTMLRenderer::SetupFramebuffer()
{
    // Create framebuffer
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // Create texture to render to
    glGenTextures(1, &m_FBOTexture);
    glBindTexture(GL_TEXTURE_2D, m_FBOTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FBOTexture, 0);

    // Check framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
    }

    // Unbind texture and framebuffer
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create quad for compositing FBO texture to screen (in screen space coordinates)
    // This covers the entire viewport area
    float quadVertices[] = {
        // positions (screen space)        // texCoords
        0.0f,           0.0f,               0.0f, 1.0f,  // top-left
        0.0f,           (float)m_height,    0.0f, 0.0f,  // bottom-left
        (float)m_width, (float)m_height,    1.0f, 0.0f,  // bottom-right

        0.0f,           0.0f,               0.0f, 1.0f,  // top-left
        (float)m_width, (float)m_height,    1.0f, 0.0f,  // bottom-right
        (float)m_width, 0.0f,               1.0f, 1.0f   // top-right
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "Framebuffer set up: " << m_width << "x" << m_height << std::endl;
}

void HTMLRenderer::RenderToFramebuffer()
{
    static int renderCount = 0;
    std::cout << "[HTML] Re-rendering to FBO (render #" << ++renderCount << ")" << std::endl;

    // Save current viewport and clear color
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Bind framebuffer and render HTML to it
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_width, m_height);

    // Clear the framebuffer with transparent color
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up OpenGL state for 2D rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use shader and set projection matrix
    m_shader->Use();
    m_shader->SetMat4("projection", m_projection);

    // Draw the document (will call our draw callbacks)
    litehtml::position clip(0, 0, m_width, m_height);
    m_document->draw((litehtml::uint_ptr)0, 0, 0, &clip);

    // Unbind framebuffer and restore viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void HTMLRenderer::RenderFramebufferToScreen()
{
    if (!m_compositeShader || !m_FBOTexture) return;

    // Set up OpenGL state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use composite shader
    m_compositeShader->Use();
    m_compositeShader->SetMat4("projection", m_projection);
    m_compositeShader->SetInt("screenTexture", 0);

    // Bind FBO texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_FBOTexture);

    // Draw quad with FBO texture
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Cleanup
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
