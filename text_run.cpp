#include "text_run.h"
#include "scope_guard.h"
#include <cassert>

TextRun::TextRun(Font &font, 
                 const std::string &text,
                 hb_direction_t direction, 
                 hb_script_t script, 
                 hb_language_t language,
                 bool underline)
: font_(font), text_(text), direction_(direction), script_(script), language_(language), underline_(underline)
{
    setDirty();
}

TextRun::~TextRun()
{
}

size_t TextRun::GetGlyphCount()
{
    doLayout();
    return glyphs_.size();
}

void TextRun::GetGlyph(size_t index, GlyphInfo &info)
{
    doLayout();
    assert(index < glyphs_.size());
    info = glyphs_[index];
}

void TextRun::setDirty()
{
    dirty_ = true;
}

void TextRun::doLayout()
{
    if (!dirty_)
        return;

    glyphs_.clear();
    
    // Create a hb_buffer.
    hb_buffer_t *buf = hb_buffer_create();
    auto buf_guard = scopeGuard([&buf]{ hb_buffer_destroy(buf); });
    // Put text in.
    hb_buffer_add_utf8(buf, text_.c_str(), -1, 0, -1);
    // Set the script, language and direction of the buffer.
    hb_buffer_set_direction(buf, direction_);
    hb_buffer_set_script(buf, script_);
    hb_buffer_set_language(buf, language_);
    // Shape
    hb_shape(font_.getHBFont(), buf, NULL, 0);
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

        glyphs_.push_back(GlyphInfo { glyphid, x_offset, y_offset, x_advance, y_advance });
    }

    dirty_ = false;
}
