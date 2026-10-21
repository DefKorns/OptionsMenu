/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
  */

#include "list_screen.h"
#include "draw_helpers.h"
#include "text_layout.h"
#include "uitheme.h"
#include "utf8.h"

#include <algorithm>

namespace
{
    constexpr int CreditFontSize = 16;
    constexpr int SectionAccentGap = 14;
    constexpr int FooterDividerInset = 10;
    constexpr int FooterDividerW = 2;
    constexpr int SelectedRowStrokeW = 2;
    constexpr int RowSeparatorGap = 3;

    constexpr int PickerChevronGap = 10;
    constexpr int PickerHintChevronGap = 4;
    constexpr int PickerHintFontSize = 16;

    constexpr int NoteFontSize = 15;
    constexpr int NoteLinePitch = 20;
    constexpr int NoteIconOffsetX = 26;
    constexpr int NoteIconSize = 18;
    constexpr int NoteTextOffsetX = 46;
    constexpr int NotePadRight = 16;
}

ScreenChrome::ScreenChrome(const std::string & optionsLocation, SDL_Renderer * renderer, const std::string & appTitle,
                           const std::string & version, const std::string & credit)
    : renderer_(renderer)
    , gear_(optionsLocation + UiTheme::AssetGear, renderer, UiTheme::GearX, UiTheme::GearY)
    , appTitle_(appTitle, UiTheme::TitleFontSize, renderer, UiTheme::TitleX, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true)
    , credit_(credit, CreditFontSize, renderer, UiTheme::CreditX, UiTheme::CreditY, false, ToAbgr(UiTheme::Text), true)
{
    appTitle_.rect.y -= appTitle_.rect.h / 2;
    appVersion_ = Texture(version, UiTheme::VersionFontSize, renderer, appTitle_.rect.x + appTitle_.rect.w + UiTheme::VersionGap, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true);
    appVersion_.rect.y -= appVersion_.rect.h / 2;
}

void ScreenChrome::SetSectionTitle(const std::string & title)
{
    sectionTitle_ = Texture(title, UiTheme::SectionTitleFontSize, renderer_, UiTheme::SectionTitleX, UiTheme::SectionTitleY, false, ToAbgr(UiTheme::Text), true);
}

const SDL_Rect & ScreenChrome::SectionTitleRect() const
{
    return sectionTitle_.rect;
}

void ScreenChrome::DrawFrame()
{
    DrawStrokeRect(renderer_, UiTheme::OuterRect, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BorderRadius);
    gear_.Draw(renderer_);
    appTitle_.Draw(renderer_);
    appVersion_.Draw(renderer_);
    DrawHLine(renderer_, UiTheme::HeaderDividerX, UiTheme::HeaderDividerX + UiTheme::HeaderDividerW, UiTheme::HeaderDividerY, UiTheme::Border, UiTheme::BorderWidth);
    DrawHLine(renderer_, UiTheme::OuterRect.x, UiTheme::OuterRect.x + UiTheme::OuterRect.w, UiTheme::FooterDividerY, UiTheme::Border, UiTheme::BorderWidth);
    credit_.Draw(renderer_);
}

void ScreenChrome::Draw()
{
    DrawFrame();
    if(sectionTitle_.texture)
    {
        const int accentBarY = sectionTitle_.rect.y + (sectionTitle_.rect.h - UiTheme::SectionAccentBarH) / 2;
        DrawFillRect(renderer_, { UiTheme::SectionTitleX - UiTheme::SectionAccentBarW - SectionAccentGap, accentBarY, UiTheme::SectionAccentBarW, UiTheme::SectionAccentBarH }, UiTheme::Accent);
        sectionTitle_.Draw(renderer_);
    }
}

void DrawFooterDivider(SDL_Renderer * renderer, int rightEdge)
{
    const int dividerX = rightEdge - UiTheme::BadgeDividerGapFromCluster;
    DrawVLine(renderer, dividerX, UiTheme::FooterY + FooterDividerInset, UiTheme::FooterY + UiTheme::FooterH - FooterDividerInset, UiTheme::Border, FooterDividerW);
}

SDL_Rect DrawListRow(SDL_Renderer * renderer, int index, int rightX, int rowHeight, bool selected)
{
    const SDL_Rect rowRect{ UiTheme::ListX, UiTheme::RowFirstY + index * UiTheme::RowPitch, rightX - UiTheme::ListX, rowHeight };
    if(selected)
    {
        DrawRoundedFillRect(renderer, rowRect, UiTheme::SelectedRowBg, UiTheme::BoxRadius);
        DrawStrokeRect(renderer, rowRect, UiTheme::Accent, SelectedRowStrokeW, UiTheme::BoxRadius);
    }
    else
        DrawHLine(renderer, rowRect.x, rowRect.x + rowRect.w, rowRect.y + rowRect.h + RowSeparatorGap, UiTheme::Border, 1);
    return rowRect;
}

ScrollArrows::ScrollArrows(const std::string & optionsLocation, SDL_Renderer * renderer)
    : renderer_(renderer)
    , up_(optionsLocation + UiTheme::AssetChevronUp, renderer, UiTheme::ScrollX, UiTheme::ScrollUpY)
{
    SetColorMod(up_.texture.get(), UiTheme::ScrollArrow);
    up_.rect.x -= up_.rect.w / 2;
    down_ = up_;
    down_.rect.y = UiTheme::ScrollDownY;
}

void ScrollArrows::Draw(bool moreAbove, bool moreBelow)
{
    if(moreAbove)
        up_.Draw(renderer_);
    if(moreBelow)
        down_.Draw(renderer_, SDL_FLIP_VERTICAL);
}

ValuePicker::ValuePicker(const std::string & optionsLocation, SDL_Renderer * renderer)
    : renderer_(renderer)
    , chevron_(optionsLocation + UiTheme::AssetChevronRight, renderer)
{
    SetColorMod(chevron_.texture.get(), UiTheme::Accent);
}

void ValuePicker::SetHintLabel(const std::string & label)
{
    hint_ = Texture(label, PickerHintFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::Text), true);
}

int ValuePicker::Width(const Texture & value) const
{
    return value.rect.w + 2 * (chevron_.rect.w + PickerChevronGap);
}

void ValuePicker::DrawValue(Texture & value, int rightX, int midY, bool selected)
{
    chevron_.rect.y = midY - chevron_.rect.h / 2;
    if(selected)
    {
        chevron_.rect.x = rightX - chevron_.rect.w;
        chevron_.Draw(renderer_);
        rightX = chevron_.rect.x - PickerChevronGap;
    }
    value.rect.x = rightX - value.rect.w;
    value.rect.y = midY - value.rect.h / 2;
    value.Draw(renderer_);
    if(selected)
    {
        chevron_.rect.x = value.rect.x - PickerChevronGap - chevron_.rect.w;
        chevron_.Draw(renderer_, SDL_FLIP_HORIZONTAL);
    }
}

int ValuePicker::DrawHint(int rightEdgeX, int y)
{
    const int midY = y + UiTheme::BadgeOuterSize / 2;
    const int x = rightEdgeX - (2 * chevron_.rect.w + PickerHintChevronGap + UiTheme::BadgeLabelGap + hint_.rect.w);
    chevron_.rect.x = x;
    chevron_.rect.y = midY - chevron_.rect.h / 2;
    chevron_.Draw(renderer_, SDL_FLIP_HORIZONTAL);
    chevron_.rect.x = x + chevron_.rect.w + PickerHintChevronGap;
    chevron_.Draw(renderer_);
    hint_.rect.x = chevron_.rect.x + chevron_.rect.w + UiTheme::BadgeLabelGap;
    hint_.rect.y = midY - hint_.rect.h / 2;
    hint_.Draw(renderer_);
    return x - UiTheme::BadgeGroupGap;
}

NoteBox::NoteBox(SDL_Renderer * renderer, SDL_Rect rect)
    : renderer_(renderer)
    , rect_(rect)
{
}

void NoteBox::SetText(const std::string & text)
{
    const int textW = rect_.w - NotePadRight - NoteTextOffsetX;
    std::vector<std::string> wrapped = WrapToWidth(text, NoteFontSize, textW);
    const size_t maxLines = static_cast<size_t>(std::max(1, rect_.h / NoteLinePitch));
    if(wrapped.size() > maxLines)
    {
        wrapped.resize(maxLines);
        std::string & last = wrapped.back();
        int n = Utf8Length(last);
        while(n > 0 && TextWidth(TruncateUtf8(last, n) + "...", NoteFontSize) > textW)
            --n;
        last = TruncateUtf8(last, n) + "...";
    }
    lines_ = MakeLines(wrapped, NoteFontSize, renderer_, UiTheme::TextDim);
}

void NoteBox::Draw()
{
    DrawStrokeRect(renderer_, rect_, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BoxRadius);
    const int iconCX = rect_.x + NoteIconOffsetX;
    const int centerY = rect_.y + rect_.h / 2;
    DrawStrokeRect(renderer_, { iconCX - NoteIconSize / 2, centerY - NoteIconSize / 2, NoteIconSize, NoteIconSize }, UiTheme::TextDim, 2, NoteIconSize / 2);
    DrawFillRect(renderer_, { iconCX - 1, centerY - 5, 2, 2 }, UiTheme::TextDim);
    DrawFillRect(renderer_, { iconCX - 1, centerY - 1, 2, 6 }, UiTheme::TextDim);
    int y = centerY - static_cast<int>(lines_.size()) * NoteLinePitch / 2;
    for(Texture & line : lines_)
    {
        line.rect.x = rect_.x + NoteTextOffsetX;
        line.rect.y = y + (NoteLinePitch - line.rect.h) / 2;
        line.Draw(renderer_);
        y += NoteLinePitch;
    }
}
