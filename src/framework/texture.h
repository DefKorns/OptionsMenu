/**
  * Copyright (C) 2017-2018 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <memory>
#include <SDL.h>

struct Texture
{
    std::shared_ptr<SDL_Texture> texture;
    SDL_Rect rect{0, 0, 0, 0}; // LoadTexturePNG leaves this untouched on failure, so it must default to zero, not garbage
    Texture();
    Texture(const std::string & text, int fontSize, SDL_Renderer* renderer, int x = 0, int y = 0, bool centerText = false, const int color = 0xFFFFFFFF, bool useTTF = false);
    Texture(const std::string & pngFilePath, SDL_Renderer* renderer, int x = 0, int y = 0, bool centerImg = false);
    void Draw(SDL_Renderer* renderer);
    void Draw(SDL_Renderer* renderer, SDL_RendererFlip flip_enum);
};

struct Sprite
{
    std::shared_ptr<SDL_Texture> texture;
    SDL_Rect sRect, dRect;
    void Draw(SDL_Renderer * renderer);
};

SDL_Texture * LoadTexturePNG(SDL_Renderer *renderer, std::string file, SDL_Rect * rect = nullptr);
SDL_Texture * WriteText(const std::string & text, int fontSize, SDL_Renderer* renderer, int & textureWidth, int & textureHeight, const int color = 0xFFFFFF);
SDL_Texture * WriteTextTTF(const std::string & text, int fontSize, SDL_Renderer* renderer, int & textureWidth, int & textureHeight, const int color = 0xFFFFFF);

// call before any useTTF=true Texture, once at startup
void SetTTFFontPath(const std::string & optionsLocation);

// false if the loaded font is missing a glyph in text - fall back to font8x8
bool CanRenderWithTTF(const std::string & text, int fontSize);

// proportional width, unlike font8x8's fixed fontSize/char
int MeasureTTFWidth(const std::string & text, int fontSize);

// line-to-line spacing, unlike font8x8's fixed-height rows
int GetTTFLineHeight(int fontSize);

#endif
