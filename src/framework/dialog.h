/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#ifndef DIALOG_H_
#define DIALOG_H_

#include "badge.h"

#include <memory>
#include <string>

namespace Dialog
{
    constexpr int PadX = 48;
    constexpr int TitleFontSize = 24;
    constexpr int BadgeRowGap = 36;
    constexpr int BottomPad = 32;

    void DrawBackdrop(SDL_Renderer * renderer);
    SDL_Rect PanelRect(int width, int height);
    void DrawPanel(SDL_Renderer * renderer, const SDL_Rect & panel);
}

class ScreenSnapshot
{
public:
    explicit ScreenSnapshot(SDL_Renderer * renderer);
    void Draw() const;

private:
    SDL_Renderer * renderer_;
    std::shared_ptr<SDL_Texture> texture_;
};

class ConfirmDialog
{
public:
    ConfirmDialog(SDL_Renderer * renderer, BadgePainter & painter, const std::string & title, Badge confirm, Badge cancel);
    void Draw();

private:
    SDL_Renderer * renderer_;
    BadgePainter * painter_;
    Texture title_;
    Badge confirm_;
    Badge cancel_;
    SDL_Rect panel_;
};

#endif
