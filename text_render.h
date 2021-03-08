#ifndef __TEXT_RENDER_H__
#define __TEXT_RENDER_H__

#include "shader.h"
#include "font.h"
#include "texture_atlas.h"
#include "text_run.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <map>
#include <vector>
#include <memory>

class TextRender
{
    typedef std::pair<unsigned int, unsigned int> GlyphKey;

    struct Glyph {
        glm::ivec2 Size;       // Size of glyph
        glm::ivec2 Bearing;    // Offset from horizontal layout origin to left/top of glyph
        glm::ivec2 TexOffset;  // Offset of glyph in texture atlas
        int TexIdx;            // Texture atlas index
        unsigned int TexGen;   // Texture atlas generation
    };

    typedef std::map<GlyphKey, Glyph> GlyphCache;

    typedef std::vector<std::unique_ptr<TextureAtlas>> TexVector;
    typedef std::vector<unsigned int> TexGenVector;

    ShaderProgram shader_;
    unsigned int vao_;
    unsigned int vbo_;
    TexVector tex_;
    TexGenVector texGen_;
    uint64_t texReq_;
    uint64_t texHit_;
    uint64_t texEvict_;
    GlyphCache glyphs_;
    Glyph line_;

    int maxQuadBatch_;
    int curQuadBatch_;
    float* vertices_;
    glm::vec3 lastColor_;
    unsigned int lastTexID_;

public:
    TextRender();
    ~TextRender();

    bool Init(int numTextureAltas, int maxQuadBatch);

    void Begin(int fbWidth, int fbHeight);

    void DrawText(TextRun &text,
                  float x, 
                  float y, 
                  glm::vec3 color);

    void End();

    void PrintStats();

private:
    bool getGlyph(Font& font, unsigned int glyph_index, Glyph& x);
    bool setupLineGlyph();
    bool addToTextureAtlas(uint16_t width, uint16_t height, const uint8_t *data, 
                           int &tex_idx, unsigned int &tex_gen, uint16_t &tex_x, uint16_t &tex_y);
    void setTextColor(glm::vec3 color);
    void setTexID(unsigned int texID);
    void appendQuad(float vertices[6][4]);
    void commitDraw();
};

#endif // !__TEXT_RENDER_H__
