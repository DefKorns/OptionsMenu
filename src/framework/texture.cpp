/**
 * Copyright (C) 2017-2018 CompCom
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#include "texture.h"
#include "font8x8_lookup.h"
#include "utf8.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <map>
#include <vector>
#include <png.h>
#include <SDL_ttf.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xff000000
#define gmask 0x00ff0000
#define bmask 0x0000ff00
#define amask 0x000000ff
#else
#define rmask 0x000000ff
#define gmask 0x0000ff00
#define bmask 0x00ff0000
#define amask 0xff000000
#endif

Texture::Texture() {}

Texture::Texture(const std::string & text, int fontSize, SDL_Renderer* renderer, int x, int y, bool centerText, const int color, bool useTTF)
{
    auto writer = (useTTF && CanRenderWithTTF(text, fontSize)) ? WriteTextTTF : WriteText;
    texture = std::shared_ptr<SDL_Texture>(writer(text, fontSize, renderer, rect.w, rect.h, color), SDL_DestroyTexture);
    rect.x = (centerText) ? x-rect.w/2 : x;
    rect.y = (centerText) ? y-rect.h/2 : y;
}

Texture::Texture(const std::string & pngFilePath, SDL_Renderer* renderer, int x, int y, bool centerImg)
{
    texture = std::shared_ptr<SDL_Texture>(LoadTexturePNG(renderer, pngFilePath, &rect), SDL_DestroyTexture);
    rect.x = (centerImg) ? x-rect.w/2 : x;
    rect.y = (centerImg) ? y-rect.h/2 : y;
}

void Texture::Draw(SDL_Renderer* renderer)
{
    if(texture.get())
        SDL_RenderCopyEx(renderer, texture.get(), NULL, &rect, 0, NULL, SDL_FLIP_NONE);
}

void Texture::Draw(SDL_Renderer* renderer, SDL_RendererFlip flip_enum)
{
    if(texture.get())
        SDL_RenderCopyEx(renderer, texture.get(), NULL, &rect, 0, NULL, flip_enum);
}

SDL_Texture * LoadTexturePNG(SDL_Renderer *renderer, std::string file, SDL_Rect * rect)
{
    SDL_Texture* texture = nullptr;

    png_image png;
    memset(&png, 0, sizeof(png));
    png.version = PNG_IMAGE_VERSION;
    png.opaque = NULL;

    if(png_image_begin_read_from_file(&png, file.c_str()) == 0)
    {
        std::cerr << "Cannot open png file: " << file << "\n";
        return nullptr;
    }
    //Set PNG Format
    png.format = PNG_FORMAT_RGBA;
    if(png.width == 0 || png.height == 0)
    {
        std::cerr << "Invalid png: " << file << "\n";
        png_image_free(&png);
        return nullptr;
    }
    if(rect)
    {
        rect->h = png.height;
        rect->w = png.width;
    }
    //Create surface
    auto surface = SDL_CreateRGBSurface(0, png.width, png.height, 32, rmask, gmask, bmask, amask);

    if(surface)
    {
        //Read png data to surface
        png_image_finish_read(&png, NULL, surface->pixels, 0, NULL);
        png_image_free(&png);
        //Convert surface to texture
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    else
    {
        std::cerr << "Error creating surface for: " << file << "\n";
        exit(1);
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

SDL_Texture * WriteText(const std::string & text, int fontSize, SDL_Renderer* renderer, int & textureWidth, int & textureHeight, const int color)
{
    if(text.size()==0)
        return nullptr;

    std::vector<unsigned int> codepoints = Utf8ToCodepoints(text);

    textureWidth = fontSize*codepoints.size();
    textureHeight = fontSize;
    auto surface = SDL_CreateRGBSurface(0, 8 * codepoints.size(), 8, 32, rmask, gmask, bmask, amask);
    uint* pixels = reinterpret_cast<uint*>(surface->pixels);
    int pitch = 8*codepoints.size();
    for (unsigned int i = 0; i < codepoints.size(); ++i)
    {
        const char * bitmap = Font8x8Glyph(codepoints[i]);
        int offset = i*8;
        for(int y = 0; y < 8; ++y)
        {
            for(int x = 0; x < 8; ++x)
            {
                pixels[x+offset+(y*pitch)] = (bitmap[y] & 1 << x) ? color : 0;
            }
        }
    }
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surface);
    return texture;
}

static std::string ttfFontPath;

// CJK font comes from the optional addon hmod if installed, else the bundled Latin-only one
void SetTTFFontPath(const std::string & optionsLocation)
{
    static const char * const cjkFontPath = "/etc/options_menu/fonts/NotoSansJP-CJK.ttf";
    ttfFontPath = std::ifstream(cjkFontPath).good() ? cjkFontPath : optionsLocation + "fonts/NotoSansJP-Latin-subset.ttf";
}

// one TTF_Font per point size actually used, opened once and kept for the process lifetime
static TTF_Font * GetTTFFont(int pointSize)
{
    static std::map<int, TTF_Font *> fonts;
    auto it = fonts.find(pointSize);
    if(it != fonts.end())
        return it->second;

    TTF_Font * font = TTF_OpenFont(ttfFontPath.c_str(), pointSize);
    fonts[pointSize] = font;
    return font;
}

SDL_Texture * WriteTextTTF(const std::string & text, int fontSize, SDL_Renderer* renderer, int & textureWidth, int & textureHeight, const int color)
{
    if(text.empty())
        return nullptr;

    TTF_Font * font = GetTTFFont(fontSize);
    if(!font)
    {
        std::cerr << "Cannot open TTF font: " << ttfFontPath << " (" << TTF_GetError() << ")\n";
        return nullptr;
    }

    // color is packed 0xAABBGGRR, same convention as WriteText/the rest of the codebase
    SDL_Color sdlColor{
        static_cast<Uint8>(color & 0xFF),
        static_cast<Uint8>((color >> 8) & 0xFF),
        static_cast<Uint8>((color >> 16) & 0xFF),
        static_cast<Uint8>((color >> 24) & 0xFF)
    };

    SDL_Surface * surface = TTF_RenderUTF8_Blended(font, text.c_str(), sdlColor);
    if(!surface)
    {
        std::cerr << "TTF_RenderUTF8_Blended failed: " << TTF_GetError() << "\n";
        return nullptr;
    }

    textureWidth = surface->w;
    textureHeight = surface->h;
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surface);
    return texture;
}

bool CanRenderWithTTF(const std::string & text, int fontSize)
{
    TTF_Font * font = GetTTFFont(fontSize);
    if(!font)
        return false;

    for(unsigned int codepoint : Utf8ToCodepoints(text))
        // TTF_GlyphIsProvided only takes a 16-bit code
        if(codepoint > 0xFFFF || !TTF_GlyphIsProvided(font, static_cast<Uint16>(codepoint)))
            return false;
    return true;
}

int MeasureTTFWidth(const std::string & text, int fontSize)
{
    TTF_Font * font = GetTTFFont(fontSize);
    if(!font || text.empty())
        return 0;
    int w = 0, h = 0;
    TTF_SizeUTF8(font, text.c_str(), &w, &h);
    return w;
}

int GetTTFLineHeight(int fontSize)
{
    TTF_Font * font = GetTTFFont(fontSize);
    return font ? TTF_FontLineSkip(font) : fontSize;
}
