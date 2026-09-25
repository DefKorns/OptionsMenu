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
#include <map>
#include <string>

namespace UiTheme
{
    Color Bg{ 0x0C, 0x0F, 0x16 };
    Color Border{ 0x2A, 0x2C, 0x33 };
    Color Accent{ 0xF0, 0xC4, 0x3D }; // yellow
    Color SelectedRowBg{ 0x14, 0x16, 0x1A };
    Color Text{ 0xFF, 0xFF, 0xFF };
    Color TextDim{ 0x8D, 0x8D, 0x93 };
    Color ScrollArrow{ 0xFF, 0xFF, 0xFF };
    Color BadgeLetter{ 0x1C, 0x16, 0x16 };
    Color BadgeA{ 0x4C, 0xAF, 0x6E };
    Color BadgeADark{ 0x2E, 0x6B, 0x42 };
    Color BadgeB{ 0xE1, 0x55, 0x54 };
    Color BadgeBDark{ 0x8C, 0x34, 0x33 };
    Color BadgeX{ 0x6C, 0x8E, 0xBF };
    Color BadgeXDark{ 0x3F, 0x59, 0x7F };
    Color BadgeY{ 0xC9, 0x7B, 0x84 };
    Color BadgeYDark{ 0x8F, 0x47, 0x50 };
    Color BadgeStart{ 0xC2, 0xA8, 0x5E };
    Color BadgeStartDark{ 0x74, 0x65, 0x38 };

    namespace
    {
        struct ParsedColor
        {
            Color color;
            bool valid;
        };

        // "R,G,B", each 0-255
        ParsedColor ParseRgb(const std::string & value)
        {
            int r = 0, g = 0, b = 0;
            if(std::sscanf(value.c_str(), "%d,%d,%d", &r, &g, &b) != 3)
                return { {}, false };
            return { { static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b) }, true };
        }

        Uint8 ClampedAdd(Uint8 base, int delta)
        {
            const int v = base + delta;
            return static_cast<Uint8>(v < 0 ? 0 : (v > 255 ? 255 : v));
        }

        const std::map<std::string, Color *> & ColorKeys()
        {
            static const std::map<std::string, Color *> keys{
                { "Bg", &Bg }, { "Border", &Border }, { "Accent", &Accent },
                { "SelectedRowBg", &SelectedRowBg }, { "Text", &Text }, { "TextDim", &TextDim },
                { "ScrollArrow", &ScrollArrow }, { "BadgeLetter", &BadgeLetter },
                { "BadgeA", &BadgeA }, { "BadgeADark", &BadgeADark },
                { "BadgeB", &BadgeB }, { "BadgeBDark", &BadgeBDark },
                { "BadgeX", &BadgeX }, { "BadgeXDark", &BadgeXDark },
                { "BadgeY", &BadgeY }, { "BadgeYDark", &BadgeYDark },
                { "BadgeStart", &BadgeStart }, { "BadgeStartDark", &BadgeStartDark },
            };
            return keys;
        }
    }

    void LoadThemeConfig(const std::string & optionsLocation)
    {
        std::ifstream in(optionsLocation + "theme.cfg");
        if(!in.is_open())
            return;

        std::map<std::string, bool> isSet;
        std::string line;
        while(std::getline(in, line))
        {
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            const size_t eq = line.find('=');
            if(eq == std::string::npos || line[0] == '#')
                continue;
            const auto key = ColorKeys().find(line.substr(0, eq));
            const ParsedColor parsed = ParseRgb(line.substr(eq + 1));
            if(key == ColorKeys().end() || !parsed.valid)
                continue;
            *key->second = parsed.color;
            isSet[key->first] = true;
        }

        // SelectedRowBg (tile/row fill - shows at rounded-corner masks, so a
        // stale default next to a customized Bg reads as a leftover old
        // color) tracks a custom Bg unless set explicitly itself, keeping
        // the same +8/+7/+4 offset as the built-in defaults
        if(isSet["Bg"] && !isSet["SelectedRowBg"])
            SelectedRowBg = { ClampedAdd(Bg.r, 8), ClampedAdd(Bg.g, 7), ClampedAdd(Bg.b, 4) };

        if(!isSet["ScrollArrow"])
            ScrollArrow = Text;
    }
}
