/**
  * Copyright (C) 2026 DefKorns
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

#include "uitheme.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace UiTheme
{
    Uint8 BgR = 0x0C, BgG = 0x0F, BgB = 0x16;
    Uint8 BorderR = 0x2A, BorderG = 0x2C, BorderB = 0x33;
    Uint8 AccentR = 0xF0, AccentG = 0xC4, AccentB = 0x3D; // yellow
    Uint8 SelectedRowBgR = 0x14, SelectedRowBgG = 0x16, SelectedRowBgB = 0x1A;
    Uint8 ScrollArrowR = 0xFF, ScrollArrowG = 0xFF, ScrollArrowB = 0xFF;

    Uint32 BadgeLetterColor = 0xFF16161C; // AABBGGRR
    Uint32 TextColor = 0xFFFFFFFF; // AABBGGRR - primary text
    Uint32 TextDimColor = 0xFF938D8D; // AABBGGRR - muted grey for secondary text

    BadgeColor BadgeA{ 0x4C, 0xAF, 0x6E }; // green
    BadgeColor BadgeADark{ 0x2E, 0x6B, 0x42 };
    BadgeColor BadgeB{ 0xE1, 0x55, 0x54 }; // red
    BadgeColor BadgeBDark{ 0x8C, 0x34, 0x33 };
    BadgeColor BadgeX{ 0x6C, 0x8E, 0xBF }; // slate-blue
    BadgeColor BadgeXDark{ 0x3F, 0x59, 0x7F };
    BadgeColor BadgeY{ 0xC9, 0x7B, 0x84 }; // dusty rose, unused
    BadgeColor BadgeYDark{ 0x8F, 0x47, 0x50 };
    BadgeColor BadgeStart{ 0xC2, 0xA8, 0x5E }; // muted gold
    BadgeColor BadgeStartDark{ 0x74, 0x65, 0x38 };

    static bool ParseTriple(const std::string & value, Uint8 & r, Uint8 & g, Uint8 & b)
    {
        int rv, gv, bv;
        if(std::sscanf(value.c_str(), "%d,%d,%d", &rv, &gv, &bv) != 3)
            return false;
        r = static_cast<Uint8>(rv);
        g = static_cast<Uint8>(gv);
        b = static_cast<Uint8>(bv);
        return true;
    }

    static Uint8 ClampedAdd(Uint8 base, int delta)
    {
        int v = base + delta;
        return static_cast<Uint8>(v < 0 ? 0 : (v > 255 ? 255 : v));
    }

    void LoadThemeConfig(const std::string & optionsLocation)
    {
        std::ifstream in(optionsLocation + "theme.cfg");
        if(!in.is_open())
            return;

        bool bgSet = false, selectedRowBgSet = false, scrollArrowSet = false;
        std::string line;
        while(std::getline(in, line))
        {
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            size_t eq = line.find('=');
            if(eq == std::string::npos || line[0] == '#')
                continue;
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            Uint8 r, g, b;
            if(!ParseTriple(value, r, g, b))
                continue;

            if(key == "Bg") { BgR = r; BgG = g; BgB = b; bgSet = true; }
            else if(key == "Border") { BorderR = r; BorderG = g; BorderB = b; }
            else if(key == "Accent") { AccentR = r; AccentG = g; AccentB = b; }
            else if(key == "SelectedRowBg") { SelectedRowBgR = r; SelectedRowBgG = g; SelectedRowBgB = b; selectedRowBgSet = true; }
            else if(key == "ScrollArrow") { ScrollArrowR = r; ScrollArrowG = g; ScrollArrowB = b; scrollArrowSet = true; }
            else if(key == "Text") TextColor = 0xFF000000 | (b << 16) | (g << 8) | r;
            else if(key == "TextDim") TextDimColor = 0xFF000000 | (b << 16) | (g << 8) | r;
            else if(key == "BadgeLetter") BadgeLetterColor = 0xFF000000 | (b << 16) | (g << 8) | r;
            else if(key == "BadgeA") BadgeA = { r, g, b };
            else if(key == "BadgeADark") BadgeADark = { r, g, b };
            else if(key == "BadgeB") BadgeB = { r, g, b };
            else if(key == "BadgeBDark") BadgeBDark = { r, g, b };
            else if(key == "BadgeX") BadgeX = { r, g, b };
            else if(key == "BadgeXDark") BadgeXDark = { r, g, b };
            else if(key == "BadgeY") BadgeY = { r, g, b };
            else if(key == "BadgeYDark") BadgeYDark = { r, g, b };
            else if(key == "BadgeStart") BadgeStart = { r, g, b };
            else if(key == "BadgeStartDark") BadgeStartDark = { r, g, b };
        }

        // SelectedRowBg (tile/row fill - shows at rounded-corner masks, so a
        // stale default next to a customized Bg reads as a leftover old
        // color) tracks a custom Bg unless set explicitly itself, keeping
        // the same +8/+7/+4 offset as the built-in defaults
        if(bgSet && !selectedRowBgSet)
        {
            SelectedRowBgR = ClampedAdd(BgR, 8);
            SelectedRowBgG = ClampedAdd(BgG, 7);
            SelectedRowBgB = ClampedAdd(BgB, 4);
        }

        if(!scrollArrowSet)
        {
            ScrollArrowR = TextColor & 0xFF;
            ScrollArrowG = (TextColor >> 8) & 0xFF;
            ScrollArrowB = (TextColor >> 16) & 0xFF;
        }
    }
}
