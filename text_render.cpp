#include "text_render.h"
#include "scope_guard.h"

#include <glad/glad.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

//------------------------------------------------------------------------------

static const char* vertex_shader_string = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

static const char* fragment_shader_string = R"(
#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}
)";

//------------------------------------------------------------------------------

TextRender::TextRender()
: vao_(0), vbo_(0)
{
}

TextRender::~TextRender()
{
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

bool TextRender::Init()
{
    std::string errorLog;
    if (!shader_.Init(vertex_shader_string, fragment_shader_string, errorLog))
    {
        return false;
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);

    if (!textureAltas_.Init(1024, 1024))
    {
        return false;
    }

    return true;
}

void TextRender::Begin(int fbWidth, int fbHeight)
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_.Use(true);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f, static_cast<float>(fbHeight));
    glUniformMatrix4fv(shader_.GetUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(vao_);
}

void TextRender::DrawText(Font& font, 
                          const std::string& text, 
                          hb_direction_t direction, 
                          hb_script_t script, 
                          hb_language_t language,
                          float x, 
                          float y, 
                          glm::vec3 color)
{
    glUniform3f(shader_.GetUniformLocation("textColor"), color.x, color.y, color.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureAltas_.TextureID());

    // Create a hb_buffer.
    hb_buffer_t *buf = hb_buffer_create();
    auto buf_guard = scopeGuard([&buf]{ hb_buffer_destroy(buf); });
    // Put text in.
    hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);
    // Set the script, language and direction of the buffer.
    hb_buffer_set_direction(buf, direction);
    hb_buffer_set_script(buf, script);
    hb_buffer_set_language(buf, language);
    // Shape
    hb_shape(font.getHBFont(), buf, NULL, 0);
    // Get the glyph and position information.
    unsigned int glyph_count;
    hb_glyph_info_t *glyph_info    = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
    
    // Iterate over each glyph.
    for (unsigned int i = 0; i < glyph_count; i++)
    {
        hb_codepoint_t glyphid  = glyph_info[i].codepoint;
        hb_position_t x_offset  = glyph_pos[i].x_offset / 64;
        hb_position_t y_offset  = glyph_pos[i].y_offset / 64;
        hb_position_t x_advance = glyph_pos[i].x_advance / 64;
        hb_position_t y_advance = glyph_pos[i].y_advance / 64;

        Glyph g;
        if (!getGlyph(font, glyphid, g))
        {
            // TODO: error log
            break;
        }

        if (g.Size.x > 0 && g.Size.y > 0)
        {
            float glyph_x = x + g.Bearing.x + x_offset;
            float glyph_y = y - (g.Size.y - g.Bearing.y) + y_offset;
            float glyph_w = (float)g.Size.x;
            float glyph_h = (float)g.Size.y;

            float tex_x = g.TexOffset.x / (float)textureAltas_.Width();
            float tex_y = g.TexOffset.y / (float)textureAltas_.Height();
            float tex_w = glyph_w / (float)textureAltas_.Width();
            float tex_h = glyph_h / (float)textureAltas_.Height();

            // update VBO for each glyph
            float vertices[6][4] = {
                { glyph_x,           glyph_y + glyph_h, tex_x,         tex_y         },
                { glyph_x,           glyph_y,           tex_x,         tex_y + tex_h },
                { glyph_x + glyph_w, glyph_y,           tex_x + tex_w, tex_y + tex_h },

                { glyph_x,           glyph_y + glyph_h, tex_x,         tex_y         },
                { glyph_x + glyph_w, glyph_y,           tex_x + tex_w, tex_y + tex_h },
                { glyph_x + glyph_w, glyph_y + glyph_h, tex_x + tex_w, tex_y         }
            };
            // update content of VBO memory
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // advance cursors for next glyph
        x += x_advance;
        y += y_advance;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRender::End()
{
    glBindVertexArray(0);

    shader_.Use(false);
}

bool TextRender::getGlyph(Font& font, unsigned int glyph_index, Glyph& x)
{
    GlyphKey key = GlyphKey{ font.getID(), glyph_index };
    GlyphCache::iterator iter = glyphs_.find(key);
    if (iter != glyphs_.end())
    {
        x = iter->second;
        return true;
    }

    // load glyph
    FT_Face face = font.getFTFont();
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT))
    {
        return false;
    }
    if (font.synthesisItalic())
    {
        // horizontal shear
        FT_Matrix matrix;
        matrix.xx = 0x10000L;
        matrix.xy = (FT_Fixed)(0.3 * 0x10000L);
        matrix.yx = 0;
        matrix.yy = 0x10000L;
        FT_Outline_Transform(&face->glyph->outline, &matrix);
    }
    if (font.synthesisBold())
    {
        FT_Outline_Embolden(&face->glyph->outline, (FT_Pos)(font.getSize() * 0.04 * 64));
    }
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
    {
        return false;
    }

    uint16_t texOffsetX = 0, texOffsetY = 0;
    if (face->glyph->bitmap.width > 0 && face->glyph->bitmap.rows > 0)
    {
        if (!textureAltas_.AddRegion(face->glyph->bitmap.width, 
                                     face->glyph->bitmap.rows, 
                                     face->glyph->bitmap.buffer, 
                                     texOffsetX, 
                                     texOffsetY))
        {
            return false;
        }
    }

    // now store Glyph for later use
    x = Glyph {
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        glm::ivec2(texOffsetX, texOffsetY)
    };
    glyphs_.insert(GlyphCache::value_type(key, x));

    return true;
}
