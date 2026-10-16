/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#ifndef BADGE_H_
#define BADGE_H_

#include "color.h"
#include "texture.h"

#include <string>
#include <vector>

struct Badge
{
    Texture letter;
    Texture label;
    Color rim;
    Color fill;
    bool pill;
};

class BadgePainter
{
public:
    BadgePainter(const std::string & optionsLocation, SDL_Renderer * renderer);

    Badge Make(const std::string & letter, const std::string & label, Color rim, Color fill) const;
    Badge MakePill(const std::string & keys, const std::string & label) const;
    int Width(const Badge & badge) const;
    int Draw(Badge & badge, int rightEdgeX, int y);
    void DrawCentered(const std::vector<Badge *> & badges, int centerX, int y);

private:
    int MarkWidth(const Badge & badge) const;

    SDL_Renderer * renderer_;
    Texture outer_;
    Texture inner_;
};

#endif
