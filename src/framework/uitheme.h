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

#include <SDL.h>

// Geometry and palette for the "settings dialog" look. Screen is a fixed
// 1280x720 (sdl_context.cpp), so these are plain constants.
namespace UiTheme
{
    const Uint8 BgR = 0x0A, BgG = 0x0A, BgB = 0x0C;

    // TV overscan safe area
    const int SafeMarginX = 48;
    const int SafeMarginY = 36;

    const SDL_Rect FrameRect{ SafeMarginX, SafeMarginY, 1280 - 2*SafeMarginX, 720 - 2*SafeMarginY };
    const int FrameInset = 24; // frame.png's 9-slice corner size

    // selection highlight bar
    const int HighlightInsetLR = 8;
    const int HighlightX = FrameRect.x + 16;
    const int HighlightW = (FrameRect.x + FrameRect.w - FrameInset) - HighlightX;
    const int HighlightH = 20;
    const Uint8 HighlightR = 0x3F, HighlightG = 0x59, HighlightB = 0x7F;

    // right edge for switches/highlight on rows with a PREVIEW_IMAGE - fixed,
    // not derived from PREVIEW_IMAGE_X, so it doesn't move as that's tuned
    const int ControlColumnRightX = 760;
    const int HighlightWWithPreview = ControlColumnRightX - HighlightX;

    // row layout
    const int RowFirstY = FrameRect.y + 236;
    const int RowPitch = 22;
    const int RowTextX = FrameRect.x + 40;
    const int DisplayItemCount = 16; // rows visible at once, both UI styles

    // badge cluster (A/B always shown, X shown per-row when it has a delete action)
    const int BadgeOuterSize = 28;
    const int BadgeInnerSize = 22;
    const int BadgeBandY = FrameRect.y + 22;
    const int BadgeClusterRightX = FrameRect.x + FrameRect.w - 24;
    const int BadgeGroupGap = 24;
    const int BadgeLabelGap = 6;
    const Uint32 BadgeLetterColor = 0xFF16161C; // AABBGGRR

    struct BadgeColor { Uint8 r, g, b; };
    const BadgeColor BadgeA{ 0xE0, 0xA4, 0x58 }; // amber
    const BadgeColor BadgeADark{ 0xA0, 0x6D, 0x2E };
    const BadgeColor BadgeB{ 0x4F, 0xB0, 0xA5 }; // teal
    const BadgeColor BadgeBDark{ 0x2C, 0x72, 0x68 };
    const BadgeColor BadgeX{ 0x6C, 0x8E, 0xBF }; // slate-blue
    const BadgeColor BadgeXDark{ 0x3F, 0x59, 0x7F };
    const BadgeColor BadgeY{ 0xC9, 0x7B, 0x84 }; // dusty rose, unused
    const BadgeColor BadgeYDark{ 0x8F, 0x47, 0x50 };

    // header/body divider
    const int HeaderDividerY = FrameRect.y + 76;
    const int HeaderDividerH = 2;
    const int HeaderDividerX = FrameRect.x + FrameInset;
    const int HeaderDividerW = FrameRect.w - 2 * FrameInset;

    // gear icon
    const int GearX = FrameRect.x + 32, GearY = FrameRect.y + 22;
    const int GearSize = 32;

    // "OptionsMenu" + smaller "vX.Y.Z", right of the gear
    const int TitleFontSize = 24;
    const int VersionFontSize = 14;
    const int TitleGap = 12;
    const int VersionGap = 8;
    const int TitleX = GearX + GearSize + TitleGap;
    const int TitleY = GearY + GearSize / 2;

    // current screen's title, centered below the divider
    const int SectionTitleFontSize = 28;
    const int SectionTitleY = FrameRect.y + 116;
    const int SectionTitleCenterX = FrameRect.x + FrameRect.w / 2;

    // toggle switch, right-aligned to the control column
    const int SwitchRightX = ControlColumnRightX;
    const int SwitchW = 40, SwitchH = 16;

    // "created by CompCom" footer credit, right-aligned inside the frame
    const int CreditRightX = (FrameRect.x + FrameRect.w - FrameInset) - 24;
    const int CreditY = FrameRect.y + FrameRect.h - 28;

    // scroll chevrons
    const int ScrollX = SectionTitleCenterX - 8;
    const int ScrollUpY = FrameRect.y + 216;
    const int ScrollDownY = RowFirstY + DisplayItemCount * RowPitch + 8;

    // asset paths, relative to optionsLocation
    const char * const AssetFrame = "/images/ui/frame.png";
    const char * const AssetHighlight = "/images/ui/highlight.png";
    const char * const AssetBadgeOuter = "/images/ui/badge_outer.png";
    const char * const AssetBadgeInner = "/images/ui/badge_inner.png";
    const char * const AssetSwitchOn = "/images/ui/switch_on.png";
    const char * const AssetSwitchOff = "/images/ui/switch_off.png";
    const char * const AssetGear = "/images/ui/gear.png";
}

#endif
