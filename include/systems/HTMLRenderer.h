#pragma once

#include <litehtml.h>
#include <GLFW/glfw3.h>
#include <string>
#include <map>
#include "util/Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

class HTMLRenderer : public litehtml::document_container
{
public:
    static HTMLRenderer &GetInstance()
    {
        static HTMLRenderer instance;
        return instance;
    }

    HTMLRenderer(HTMLRenderer const &) = delete;
    void operator=(HTMLRenderer const &) = delete;

    void Initialize(GLFWwindow *window, int width, int height);
    void Shutdown();
    void LoadHTML(const std::string& html, const std::string& master_css = "");
    void Render();
    void Resize(int width, int height);
    void MarkDirty(); // Mark HTML as needing re-render

    // litehtml::document_container interface implementation
    litehtml::uint_ptr create_font(const litehtml::font_description& descr, const litehtml::document* doc,
                                   litehtml::font_metrics* fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
                   litehtml::web_color color, const litehtml::position& pos) override;
    litehtml::pixel_t pt_to_px(float pt) const override;
    litehtml::pixel_t get_default_font_size() const override;
    const char* get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                   const std::string& url, const std::string& base_url) override;
    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                        const litehtml::web_color& color) override;
    void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                             const litehtml::background_layer::linear_gradient& gradient) override;
    void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                             const litehtml::background_layer::radial_gradient& gradient) override;
    void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                            const litehtml::background_layer::conic_gradient& gradient) override;
    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
                     const litehtml::position& draw_pos, bool root) override;
    void set_caption(const char* caption) override;
    void set_base_url(const char* base_url) override;
    void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
    void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    void on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) override;
    void set_cursor(const char* cursor) override;
    void transform_text(litehtml::string& text, litehtml::text_transform tt) override;
    void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
    void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
    void del_clip() override;
    void get_viewport(litehtml::position& viewport) const override;
    std::shared_ptr<litehtml::element> create_element(const char *tag_name,
                                                     const litehtml::string_map &attributes,
                                                     const std::shared_ptr<litehtml::document> &doc) override;
    void get_media_features(litehtml::media_features& media) const override;
    void get_language(litehtml::string& language, litehtml::string& culture) const override;

private:
    HTMLRenderer() = default;

    GLFWwindow* m_window = nullptr;
    litehtml::document::ptr m_document;
    int m_width = 800;
    int m_height = 600;

    // Glyph information for text rendering
    struct Glyph {
        unsigned int textureID;  // ID handle of the glyph texture
        glm::ivec2   size;       // Size of glyph
        glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
        unsigned int advance;    // Horizontal offset to advance to next glyph
    };

    // Font information with FreeType
    struct FontInfo {
        std::string name;
        litehtml::pixel_t size;
        int weight;
        litehtml::pixel_t ascent;  // Font ascent for baseline calculation
        std::map<char, Glyph> glyphs; // Glyph cache for this font
    };
    std::map<litehtml::uint_ptr, FontInfo> m_fonts;
    litehtml::uint_ptr m_next_font_id = 1;

    // FreeType library
    FT_Library m_ft_library = nullptr;
    std::map<std::string, FT_Face> m_ft_faces; // Cache of loaded FreeType faces

    // Helper methods for drawing
    void DrawRect(int x, int y, int width, int height, litehtml::web_color color);
    void DrawBorder(const litehtml::position& pos, const litehtml::border& border, int side);
    void SetupRenderingResources();
    void SetupFramebuffer();
    void RenderToFramebuffer();
    void RenderFramebufferToScreen();
    void LoadFont(const std::string& fontPath, litehtml::pixel_t size, FontInfo& fontInfo);
    void RenderText(const std::string& text, float x, float y, float scale,
                   const FontInfo& font, litehtml::web_color color);

    // Rendering resources
    Shader* m_shader = nullptr;
    Shader* m_textShader = nullptr;
    Shader* m_compositeShader = nullptr; // Shader for drawing FBO texture to screen
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_textVAO = 0;
    unsigned int m_textVBO = 0;
    glm::mat4 m_projection;

    // Framebuffer for render-to-texture optimization
    unsigned int m_FBO = 0;
    unsigned int m_FBOTexture = 0;
    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;
    bool m_isDirty = true; // Flag to track if HTML needs re-rendering
};
