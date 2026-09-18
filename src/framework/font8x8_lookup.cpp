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

#include "font8x8_lookup.h"
#include "font8x8.h"

#include <algorithm>

static const char * LookupKanjiGlyph(unsigned int codepoint)
{
    auto it = std::lower_bound(std::begin(font8x8_kanji), std::end(font8x8_kanji), codepoint,
        [](const Font8x8KanjiEntry & entry, unsigned int cp) { return entry.codepoint < cp; });
    if(it != std::end(font8x8_kanji) && it->codepoint == codepoint)
        return reinterpret_cast<const char *>(it->glyph);
    return nullptr;
}

const char * Font8x8Glyph(unsigned int codepoint)
{
    if(codepoint < 0x80)
        return font8x8_basic[codepoint];
    if(codepoint >= 0xA0 && codepoint <= 0xFF)
        return font8x8_ext_latin[codepoint - 0xA0];
    if(codepoint >= 0x0400 && codepoint <= 0x04FF)
        return font8x8_cyrillic[codepoint - 0x0400];
    if(codepoint >= 0x3040 && codepoint <= 0x309F)
        return font8x8_hiragana[codepoint - 0x3040];
    if(codepoint >= 0x30A0 && codepoint <= 0x30FF)
        return font8x8_katakana[codepoint - 0x30A0];
    if(codepoint >= 0x4E00 && codepoint <= 0x9FFF)
    {
        const char * kanjiGlyph = LookupKanjiGlyph(codepoint);
        if(kanjiGlyph)
            return kanjiGlyph;
    }
    return font8x8_basic[0x3F];
}
