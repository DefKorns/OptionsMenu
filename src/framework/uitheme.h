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

// Geometry and palette for the split-panel "settings dialog" look. Screen
// is a fixed 1280x720 (sdl_context.cpp), so these are plain constants.
// Border/dividers are flat drawn lines (see draw_helpers.h), no textured art.
namespace UiTheme
{
    const Uint8 BgR = 0x0C, BgG = 0x0F, BgB = 0x16;
    const Uint8 BorderR = 0x2A, BorderG = 0x2C, BorderB = 0x33;
    const Uint8 AccentR = 0xF0, AccentG = 0xC4, AccentB = 0x3D; // yellow
    const Uint8 SelectedRowBgR = 0x14, SelectedRowBgG = 0x16, SelectedRowBgB = 0x1A;

    // TV overscan safe area
    const int SafeMarginX = 64;
    const int SafeMarginY = 36;
    const int BorderWidth = 3;
    const int BorderRadius = 14;
    const int BoxRadius = 8; // selected row / detail icon box / preview box
    const int ContentPadding = 16; // border to content start
    const int FrameInset = 24; // text inset for the INTERNAL command output screen

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

    // header/body divider
    const int HeaderDividerY = FrameY + HeaderH;
    const int HeaderDividerX = FrameX;
    const int HeaderDividerW = FrameW;

    // gear icon
    const int GearX = FrameX, GearY = FrameY + 4;
    const int GearSize = 32;

    // "OptionsMenu" + smaller "vX.Y.Z", right of the gear
    const int TitleFontSize = 24;
    const int VersionFontSize = 14;
    const int TitleGap = 12;
    const int VersionGap = 8;
    const int TitleX = GearX + GearSize + TitleGap;
    const int TitleY = GearY + GearSize / 2;

    // current screen's title, left-aligned next to a small accent bar
    const int SectionTitleFontSize = 28;
    const int SectionAccentBarW = 3;
    const int SectionAccentBarH = 26;
    const int SectionTitleY = HeaderDividerY + 22;
    const int SectionTitleX = FrameX + 32 + 14 + SectionAccentBarW; // ListX + accent bar + gap

    // row list (left column)
    const int ListX = FrameX + 32;
    const int ListW = 800;
    const int ListContentRightX = ListX + ListW - 40; // row box / divider right edge
    const int RowControlRightX = ListContentRightX - 16; // switch/chevron align here - matches RowTextX's 16px left inset
    const int RowFirstY = SectionTitleY + 46; // clears the section title's own text height at SectionTitleFontSize
    const int RowPitch = 36;
    const int RowTextYNudge = -4; // fine-tune vs. pure (slot-h - text-h)/2 centering
    const int RowTextX = ListX + 16;
    const int PinnedBottomMargin = 10; // gap from the footer divider to the last pinned row (e.g. Exit)

    // detail panel (right column), Modern UI only - just the preview image,
    // fixed Y independent of scroll/selection (it must not move as the list
    // scrolls). No icon box/title - nothing to show without per-command art.
    const int DetailX = ListX + ListW + 16;
    const int DetailW = (FrameX + FrameW - 16) - DetailX;
    const int PreviewBoxH = 200;
    // centered between the header and footer dividers, not anchored to the top
    const int PreviewBoxY = HeaderDividerY + ((FooterDividerY - HeaderDividerY) - PreviewBoxH) / 2;

    // toggle switch
    const int SwitchRightX = RowControlRightX;
    const int SwitchW = 40, SwitchH = 16;

    // badge cluster (A/B always shown, X shown per-row when it has a delete
    // action), now in the footer instead of the header
    const int BadgeOuterSize = 28;
    const int BadgeInnerSize = 22;
    const int BadgeBandY = FooterY + (FooterH - BadgeOuterSize) / 2;
    const int BadgeClusterRightX = FrameX + FrameW - 8;
    const int BadgeGroupGap = 24;
    const int BadgeLabelGap = 6;
    // DrawBadge's returned "next left edge" already subtracts BadgeGroupGap
    // (24px) from the last-drawn badge's own left edge; this closes the gap
    // to land the divider exactly 40px left of that badge, per hardware test
    const int BadgeDividerGapFromCluster = 16;
    const Uint32 BadgeLetterColor = 0xFF16161C; // AABBGGRR
    const Uint32 TextDimColor = 0xFF938D8D; // AABBGGRR - muted grey for chevrons/secondary text

    struct BadgeColor { Uint8 r, g, b; };
    const BadgeColor BadgeA{ 0x4C, 0xAF, 0x6E }; // green
    const BadgeColor BadgeADark{ 0x2E, 0x6B, 0x42 };
    const BadgeColor BadgeB{ 0xE1, 0x55, 0x54 }; // red
    const BadgeColor BadgeBDark{ 0x8C, 0x34, 0x33 };
    const BadgeColor BadgeX{ 0x6C, 0x8E, 0xBF }; // slate-blue
    const BadgeColor BadgeXDark{ 0x3F, 0x59, 0x7F };
    const BadgeColor BadgeY{ 0xC9, 0x7B, 0x84 }; // dusty rose, unused
    const BadgeColor BadgeYDark{ 0x8F, 0x47, 0x50 };

    // "created by CompCom" footer credit, left-aligned in the footer
    const int CreditX = FrameX;
    const int CreditY = FooterY + FooterH/2 - 8;

    // scroll chevrons - in the empty gutter left of the list, not over it
    const int ScrollX = FrameX + (ListX - FrameX) / 2 - 7;
    const int ScrollUpY = HeaderDividerY + 90;
    const int ScrollDownY = FooterDividerY - 104;

    // asset paths, relative to optionsLocation
    const char * const AssetBadgeOuter = "/images/ui/badge_outer.png";
    const char * const AssetBadgeInner = "/images/ui/badge_inner.png";
    const char * const AssetSwitchOn = "/images/ui/switch_on.png";
    const char * const AssetSwitchOff = "/images/ui/switch_off.png";
    const char * const AssetGear = "/images/ui/gear.png";
}

#endif
