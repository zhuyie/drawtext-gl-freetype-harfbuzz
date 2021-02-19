#ifndef __TEXT_RENDER_H__
#define __TEXT_RENDER_H__

#include "shader.h"
#include "font.h"
#include "texture_atlas.h"

#include <hb.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <map>

class TextRender
{
    typedef std::pair<unsigned int, unsigned int> GlyphKey;

    struct Glyph {
        glm::ivec2 Size;       // Size of glyph
        glm::ivec2 Bearing;    // Offset from horizontal layout origin to left/top of glyph
        glm::ivec2 TexOffset;  // Offset of glyph in texture atalas
    };

    typedef std::map<GlyphKey, Glyph> GlyphCache;

    ShaderProgram shader_;
    unsigned int vao_;
    unsigned int vbo_;
    TextureAtlas textureAltas_;
    GlyphCache glyphs_;

public:
    TextRender();
    ~TextRender();

    bool Init();

    void Begin(int fbWidth, int fbHeight);

    void DrawText(Font& font, 
                  const std::string& text, 
                  hb_direction_t direction, 
                  hb_script_t script, 
                  hb_language_t language,
                  float x, 
                  float y, 
                  glm::vec3 color);

    void End();

private:
    bool getGlyph(Font& font, unsigned int glyph_index, Glyph& x);
};

#endif // !__TEXT_RENDER_H__
