#ifndef __FONT_H__
#define __FONT_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

#include "scope_guard.h"

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
    Font(FT_Library ftLib, const char* fontFile, float fontSize, float contentScale, bool bold, bool italic)
    : ftFont_(NULL), hbFont_(NULL), fontSize_(0), contentScale_(0), bold_(false), italic_(false), initOK_(false)
    {
        init(ftLib, fontFile, fontSize, contentScale, bold, italic);
    }
    ~Font()
    {
        if (hbFont_)
        {
            hb_font_destroy(hbFont_);
            hbFont_ = NULL;
        }
        if (ftFont_)
        {
            FT_Done_Face(ftFont_);
            ftFont_ = NULL;
        }
    }
    bool initOK() { return initOK_; }
    
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
    void init(FT_Library ftLib, const char* fontFile, float fontSize, float contentScale, bool bold, bool italic)
    {
        if (FT_New_Face(ftLib, fontFile, 0, &ftFont_))
        {
            return;
        }
        auto ftFont_guard = scopeGuard([this]{ FT_Done_Face(ftFont_); ftFont_ = NULL; });

    #if defined(_WIN32)
        const int logic_dpi_x = 96;
        const int logic_dpi_y = 96;
    #elif defined(__APPLE__)
        const int logic_dpi_x = 72;
        const int logic_dpi_y = 72;
    #else
        #error "not implemented"
    #endif
        FT_Set_Char_Size(
            ftFont_, 
            0,                                       // same as character height
            (FT_F26Dot6)(fontSize*contentScale*64),  // char_height in 1/64th of points
            logic_dpi_x,                             // horizontal device resolution
            logic_dpi_y                              // vertical device resolution
        );

        hbFont_ = hb_ft_font_create_referenced(ftFont_);

        ftFont_guard.dismiss();
        ID_ = genID();
        fontSize_ = fontSize;
        contentScale_ = contentScale;
        bold_ = bold;
        italic_ = italic;
        initOK_ = true;
        return;
    }
    unsigned int genID()
    {
        static unsigned int s_ID = 0;
        return (++s_ID);
    }
};

//------------------------------------------------------------------------------

#endif // !__FONT_H__
