#ifndef __FONT_H__
#define __FONT_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

//------------------------------------------------------------------------------

class Font
{
    unsigned int ID_;
    FT_Face ftFont_;
    hb_font_t* hbFont_;
    float fontSize_;
    float contentScale_;
    bool bold_;
    bool italic_;
    bool initOK_;

public:
    Font(FT_Library ftLib, const char* fontFile, float fontSize, float contentScale, bool bold, bool italic);
    ~Font();
    
    bool Ok() { return initOK_; }
    
    unsigned int getID() const { return ID_; }
    FT_Face getFTFont() const { return ftFont_; }
    hb_font_t* getHBFont() const { return hbFont_; }
    float getSize() const { return fontSize_; }
    float getContentScale() const { return contentScale_; }
    bool getBold() const { return bold_; }
    bool getItalic() const { return italic_; }
    bool synthesisBold() const
    { 
        return (bold_ && !(ftFont_->style_flags & FT_STYLE_FLAG_BOLD));
    }
    bool synthesisItalic() const
    {
        return (italic_ && !(ftFont_->style_flags & FT_STYLE_FLAG_ITALIC));
    }

private:
    void init(FT_Library ftLib, const char* fontFile, float fontSize, float contentScale, bool bold, bool italic);
    unsigned int genID();
};

//------------------------------------------------------------------------------

#endif // !__FONT_H__
