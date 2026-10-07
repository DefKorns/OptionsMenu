/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "badge.h"
#include "draw_helpers.h"
#include "uitheme.h"

namespace
{
    constexpr int BadgeFontSize = 16;
    constexpr int PillFontSize = 14;
    constexpr int PillPadX = 10;
}

BadgePainter::BadgePainter(const std::string & optionsLocation, SDL_Renderer * renderer)
    : renderer_(renderer)
    , outer_(optionsLocation + UiTheme::AssetBadgeOuter, renderer)
    , inner_(optionsLocation + UiTheme::AssetBadgeInner, renderer)
{
}

Badge BadgePainter::Make(const std::string & letter, const std::string & label, Color rim, Color fill) const
{
    return Badge{ Texture(letter, BadgeFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::BadgeLetter), true),
                  Texture(label, BadgeFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::Text), true), rim, fill, false };
}

Badge BadgePainter::MakePill(const std::string & keys, const std::string & label) const
{
    return Badge{ Texture(keys, PillFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::BadgeLetter), true),
                  Texture(label, BadgeFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::Text), true), UiTheme::BadgeStartDark, UiTheme::BadgeStart, true };
}

int BadgePainter::MarkWidth(const Badge & badge) const
{
    return badge.pill ? badge.letter.rect.w + 2 * PillPadX : UiTheme::BadgeOuterSize;
}

int BadgePainter::Width(const Badge & badge) const
{
    return MarkWidth(badge) + UiTheme::BadgeLabelGap + badge.label.rect.w;
}

int BadgePainter::Draw(Badge & badge, int rightEdgeX, int y)
{
    const int x = rightEdgeX - Width(badge);
    const int markW = MarkWidth(badge);
    const int border = (UiTheme::BadgeOuterSize - UiTheme::BadgeInnerSize) / 2;
    if(badge.pill)
    {
        const SDL_Rect pillRect{ x, y, markW, UiTheme::BadgeOuterSize };
        DrawRoundedFillRect(renderer_, pillRect, badge.rim, pillRect.h / 2);
        const SDL_Rect fillRect{ x + border, y + border, markW - 2 * border, UiTheme::BadgeInnerSize };
        DrawRoundedFillRect(renderer_, fillRect, badge.fill, fillRect.h / 2);
        badge.letter.rect.x = x + (markW - badge.letter.rect.w) / 2;
        badge.letter.rect.y = y + (UiTheme::BadgeOuterSize - badge.letter.rect.h) / 2;
    }
    else
    {
        outer_.rect = { x, y, UiTheme::BadgeOuterSize, UiTheme::BadgeOuterSize };
        SetColorMod(outer_.texture.get(), badge.rim);
        outer_.Draw(renderer_);
        inner_.rect = { x + border, y + border, UiTheme::BadgeInnerSize, UiTheme::BadgeInnerSize };
        SetColorMod(inner_.texture.get(), badge.fill);
        inner_.Draw(renderer_);
        badge.letter.rect.x = x + (UiTheme::BadgeOuterSize - badge.letter.rect.w) / 2 + 1;
        badge.letter.rect.y = y + (UiTheme::BadgeOuterSize - badge.letter.rect.h) / 2 - 1;
    }
    badge.letter.Draw(renderer_);
    badge.label.rect.x = x + markW + UiTheme::BadgeLabelGap;
    badge.label.rect.y = y + (UiTheme::BadgeOuterSize - badge.label.rect.h) / 2;
    badge.label.Draw(renderer_);
    return x - UiTheme::BadgeGroupGap;
}

void BadgePainter::DrawCentered(const std::vector<Badge *> & badges, int centerX, int y)
{
    int total = 0;
    for(const Badge * badge : badges)
        total += Width(*badge);
    if(!badges.empty())
        total += static_cast<int>(badges.size() - 1) * UiTheme::BadgeGroupGap;
    int rightEdge = centerX + total / 2;
    for(auto it = badges.rbegin(); it != badges.rend(); ++it)
        rightEdge = Draw(**it, rightEdge, y);
}
