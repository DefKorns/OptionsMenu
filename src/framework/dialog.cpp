/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "dialog.h"
#include "draw_helpers.h"
#include "uitheme.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace
{
    constexpr Uint8 BackdropAlpha = 0xB0;
    constexpr int ScreenW = 1280;
    constexpr int ScreenH = 720;
    constexpr int MinPanelW = 520;
    constexpr int TopPad = 36;
    constexpr int PanelBorderW = 2;
}

void Dialog::DrawBackdrop(SDL_Renderer * renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, BackdropAlpha);
    const SDL_Rect screen{ 0, 0, ScreenW, ScreenH };
    SDL_RenderFillRect(renderer, &screen);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

ScreenSnapshot::ScreenSnapshot(SDL_Renderer * renderer)
    : renderer_(renderer)
{
    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    if(width <= 0 || height <= 0)
        return;
    const int pitch = width * 4;
    std::vector<Uint8> pixels(static_cast<size_t>(pitch) * height);
    if(SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, pixels.data(), pitch) != 0)
        return;
    SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    if(!texture)
        return;
    SDL_UpdateTexture(texture, nullptr, pixels.data(), pitch);
    texture_.reset(texture, SDL_DestroyTexture);
}

void ScreenSnapshot::Draw() const
{
    if(texture_)
        SDL_RenderCopy(renderer_, texture_.get(), nullptr, nullptr);
}

SDL_Rect Dialog::PanelRect(int width, int height)
{
    const int bodyCenterY = (UiTheme::HeaderDividerY + UiTheme::FooterDividerY) / 2;
    return SDL_Rect{ ScreenW / 2 - width / 2, bodyCenterY - height / 2, width, height };
}

void Dialog::DrawPanel(SDL_Renderer * renderer, const SDL_Rect & panel)
{
    DrawRoundedFillRect(renderer, panel, UiTheme::Bg, UiTheme::BorderRadius);
    DrawStrokeRect(renderer, panel, UiTheme::Accent, PanelBorderW, UiTheme::BorderRadius);
}

ConfirmDialog::ConfirmDialog(SDL_Renderer * renderer, BadgePainter & painter, const std::string & title, Badge confirm, Badge cancel)
    : renderer_(renderer)
    , painter_(&painter)
    , title_(title, Dialog::TitleFontSize, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true)
    , confirm_(std::move(confirm))
    , cancel_(std::move(cancel))
{
    const int badgesW = painter.Width(cancel_) + UiTheme::BadgeGroupGap + painter.Width(confirm_);
    const int width = std::max(MinPanelW, std::max(title_.rect.w, badgesW) + 2 * Dialog::PadX);
    const int height = TopPad + title_.rect.h + Dialog::BadgeRowGap + UiTheme::BadgeOuterSize + Dialog::BottomPad;
    panel_ = Dialog::PanelRect(width, height);
    title_.rect.x = panel_.x + (panel_.w - title_.rect.w) / 2;
    title_.rect.y = panel_.y + TopPad;
}

void ConfirmDialog::Draw()
{
    Dialog::DrawBackdrop(renderer_);
    Dialog::DrawPanel(renderer_, panel_);
    title_.Draw(renderer_);
    painter_->DrawCentered({ &cancel_, &confirm_ }, panel_.x + panel_.w / 2, title_.rect.y + title_.rect.h + Dialog::BadgeRowGap);
}
