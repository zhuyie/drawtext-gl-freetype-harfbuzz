#include "texture_atlas.h"
#include <glad/glad.h>
#include <cassert>
#include <cstdlib>

TextureAtlas::TextureAtlas()
: width_(0), height_(0), data_(nullptr), texture_(0)
{
}

TextureAtlas::~TextureAtlas()
{
    free(data_);
    glDeleteTextures(1, &texture_);
}

bool TextureAtlas::Init(uint16_t width, uint16_t height)
{
    assert(width > 0);
    assert(height > 0);

    width_ = width;
    height_ = height;
    
    binPacker_.Init(width, height);

    data_ = (uint8_t*)calloc(width * height * 1, sizeof(uint8_t));
    if (data_ == nullptr)
    {
        return false;
    }

    // generate texture
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        width_,
        height_,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

bool TextureAtlas::AddRegion(uint16_t width, uint16_t height, const uint8_t *data, uint16_t &x, uint16_t &y)
{
    assert(width > 0);
    assert(height > 0);
    assert(data != nullptr);

    assert(width_ > 0);
    assert(height_ > 0);
    assert(data_ != nullptr);
    assert(texture_ != 0);

    binpack::Rect r = binPacker_.Insert(width, height);
    if (r.height <= 0)
    {
        return false;
    }

    for (uint16_t i = 0; i < height; i++)
    {
        memcpy(data_ + ((r.y + i) * width_ + r.x), data + i * width, width);
    }

    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, width, height, GL_RED, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    x = r.x;
    y = r.y;
    
    return true;
}

void TextureAtlas::Clear()
{
    assert(width_ > 0);
    assert(height_ > 0);
    assert(data_ != nullptr);
    assert(texture_ != 0);

    binPacker_.Init(width_, height_);

    memset(data_, 0, width_ * height_ * 1 * sizeof(uint8_t));

    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RED, GL_UNSIGNED_BYTE, data_);
    glBindTexture(GL_TEXTURE_2D, 0);
}
