#pragma once

#include "font.h"

#include <hb.h>

#include <string>
#include <vector>

class TextRun
{
public:
    struct GlyphInfo {
        hb_codepoint_t glyphid;
        hb_position_t x_offset;
        hb_position_t y_offset;
        hb_position_t x_advance;
        hb_position_t y_advance;
    };

private:
    Font &font_;
    std::string text_;
    hb_direction_t direction_; 
    hb_script_t script_; 
    hb_language_t language_;
    std::vector<GlyphInfo> glyphs_;
    bool dirty_;

public:
    TextRun(Font &font, 
            const std::string &text,
            hb_direction_t direction, 
            hb_script_t script, 
            hb_language_t language);
    ~TextRun();

    Font& GetFont() const { return font_; }

    size_t GetGlyphCount();
    void GetGlyph(size_t index, GlyphInfo &info);

private:
    void setDirty();
    void doLayout();
};
