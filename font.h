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
    FT_Face ftFont_;
    hb_font_t* hbFont_;
    float fontSize_;
    float contentScale_;
    bool initOK_;

public:
    Font(FT_Library ftLib, const char* fontFile, float fontSize, float contentScale)
    : ftFont_(NULL), hbFont_(NULL), fontSize_(0), contentScale_(0), initOK_(false)
    {
        init(ftLib, fontFile, fontSize, contentScale);
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
    
    FT_Face getFTFont() { return ftFont_; }
    hb_font_t* getHBFont() { return hbFont_; }
    float getSize() { return fontSize_; }
    float getContentScale() { return contentScale_; }
    
private:
    void init(FT_Library ftLib, const char* fontFile, float fontSize, float contentScale)
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
            0,                         // same as character height
            fontSize*contentScale*64,  // char_height in 1/64th of points
            logic_dpi_x,               // horizontal device resolution
            logic_dpi_y                // vertical device resolution
        );

        hbFont_ = hb_ft_font_create_referenced(ftFont_);

        ftFont_guard.dismiss();
        fontSize_ = fontSize;
        contentScale_ = contentScale;
        initOK_ = true;
        return;
    }
};

//------------------------------------------------------------------------------

#endif // !__FONT_H__
