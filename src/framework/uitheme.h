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

#ifndef UITHEME_H_
#define UITHEME_H_

#include "color.h"

#include <SDL.h>
#include <string>

namespace UiTheme
{
    void LoadThemeConfig(const std::string & optionsLocation);

    extern Color Bg;
    extern Color Border;
    extern Color Accent;
    extern Color SelectedRowBg;
    extern Color Text;
    extern Color TextDim;
    extern Color ScrollArrow;
    extern Color BadgeLetter;
    extern Color BadgeA;
    extern Color BadgeADark;
    extern Color BadgeB;
    extern Color BadgeBDark;
    extern Color BadgeX;
    extern Color BadgeXDark;
    extern Color BadgeY;
    extern Color BadgeYDark;
    extern Color BadgeStart;
    extern Color BadgeStartDark;
    extern Color CheckboxOn;
    extern Color CheckboxOff;

    const int SafeMarginX = 64;
    const int SafeMarginY = 36;
    const int BorderWidth = 3;
    const int BorderRadius = 14;
    const int BoxRadius = 8;
    const int ContentPadding = 16;
    const int FrameInset = 24;

    const SDL_Rect FrameRect{ SafeMarginX, SafeMarginY, 1280 - 2*SafeMarginX, 720 - 2*SafeMarginY };
    const int FrameX = FrameRect.x + ContentPadding;
    const int FrameY = FrameRect.y + ContentPadding;
    const int FrameW = FrameRect.w - 2*ContentPadding;

    const int HeaderH = 64;
    const int FooterGap = 16;
    const int FooterH = 50;
    const int BodyH = FrameRect.h - 2*ContentPadding - FooterGap - FooterH;
    const int FooterY = FrameY + BodyH + FooterGap;
    const int FooterDividerY = FooterY - FooterGap/2;
    const SDL_Rect OuterRect{ FrameRect.x, FrameRect.y, FrameRect.w, (FooterY+FooterH+ContentPadding) - FrameRect.y };

    const int HeaderDividerY = FrameY + HeaderH;
    const int HeaderDividerX = FrameX;
    const int HeaderDividerW = FrameW;

    const int GearX = FrameX, GearY = FrameY + 4;
    const int GearSize = 32;

    const int TitleFontSize = 24;
    const int VersionFontSize = 14;
    const int TitleGap = 12;
    const int VersionGap = 8;
    const int TitleX = GearX + GearSize + TitleGap;
    const int TitleY = GearY + GearSize / 2;

    const int SectionTitleFontSize = 28;
    const int SectionAccentBarW = 3;
    const int SectionAccentBarH = 26;
    const int SectionTitleY = HeaderDividerY + 22;
    const int SectionTitleX = FrameX + 32 + 14 + SectionAccentBarW;

    const int ListX = FrameX + 32;
    const int ListW = 800;
    const int ListContentRightX = ListX + ListW - 40;
    const int RowControlRightX = ListContentRightX - 16;
    const int RowFirstY = SectionTitleY + 46;
    const int RowPitch = 36;
    const int RowTextYNudge = -4;
    const int RowTextX = ListX + 16;
    const int ListBottomMargin = 10;

    const int DetailX = ListX + ListW + 16;
    const int DetailW = (FrameX + FrameW - 16) - DetailX;
    const int PreviewBoxH = 200;
    const int PreviewBoxY = HeaderDividerY + ((FooterDividerY - HeaderDividerY) - PreviewBoxH) / 2;

    const int SwitchRightX = RowControlRightX;
    const int SwitchW = 40, SwitchH = 16;

    const int BadgeOuterSize = 28;
    const int BadgeInnerSize = 22;
    const int BadgeBandY = FooterY + (FooterH - BadgeOuterSize) / 2;
    const int BadgeClusterRightX = FrameX + FrameW - 8;
    const int BadgeGroupGap = 24;
    const int BadgeLabelGap = 6;
    const int BadgeDividerGapFromCluster = 16;

    const int CreditX = FrameX;
    const int CreditY = FooterY + FooterH/2 - 8;

    const int ScrollX = FrameX + (ListX - FrameX) / 2 - 7;
    const int ScrollUpY = HeaderDividerY + 90;
    const int ScrollDownY = FooterDividerY - 104;

    const char * const AssetBadgeOuter = "/images/ui/badge_outer.png";
    const char * const AssetBadgeInner = "/images/ui/badge_inner.png";
    const char * const AssetSwitchOn = "/images/ui/switch_on.png";
    const char * const AssetSwitchOff = "/images/ui/switch_off.png";
    const char * const AssetGear = "/images/ui/gear.png";
    const char * const AssetChevronRight = "/images/ui/chevron_right.png";
    const char * const AssetChevronUp = "/images/ui/chevron_up.png";
    const char * const AssetCheckboxOn = "/images/ui/checkbox_on.png";
    const char * const AssetCheckboxOff = "/images/ui/checkbox_off.png";
}

#endif
