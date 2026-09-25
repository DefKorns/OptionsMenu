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

#include "utf8.h"

namespace
{
    constexpr unsigned int ReplacementChar = 0xFFFD;

    bool IsContinuation(unsigned char byte)
    {
        return (byte & 0xC0) == 0x80;
    }

    // byte count implied by a lead byte; invalid leads count as 1 so callers
    // always make progress
    size_t SequenceLength(unsigned char lead)
    {
        if(lead >= 0xF0 && lead <= 0xF4) return 4;
        if(lead >= 0xE0) return lead <= 0xEF ? 3 : 1;
        if(lead >= 0xC2) return 2;
        return 1;
    }

    struct DecodeResult
    {
        unsigned int codepoint;
        bool valid; // false if the sequence is malformed or cut short
    };

    DecodeResult DecodeAt(const std::string & text, size_t pos, size_t length)
    {
        const unsigned char lead = static_cast<unsigned char>(text[pos]);
        if(length == 1)
            return { lead, lead < 0x80 };
        if(pos + length > text.size())
            return { ReplacementChar, false };

        static const unsigned char LeadMask[] = { 0, 0, 0x1F, 0x0F, 0x07 };
        unsigned int codepoint = lead & LeadMask[length];
        for(size_t i = 1; i < length; ++i)
        {
            const unsigned char byte = static_cast<unsigned char>(text[pos + i]);
            if(!IsContinuation(byte))
                return { ReplacementChar, false };
            codepoint = (codepoint << 6) | (byte & 0x3F);
        }
        return { codepoint, true };
    }
}

std::vector<unsigned int> Utf8ToCodepoints(const std::string & text)
{
    std::vector<unsigned int> codepoints;
    for(size_t pos = 0; pos < text.size();)
    {
        const size_t length = SequenceLength(static_cast<unsigned char>(text[pos]));
        const DecodeResult decoded = DecodeAt(text, pos, length);
        if(decoded.valid)
        {
            codepoints.push_back(decoded.codepoint);
            pos += length;
        }
        else
        {
            // only the lead byte is consumed, so a stray lead can't swallow
            // the valid characters after it
            codepoints.push_back(ReplacementChar);
            ++pos;
        }
    }
    return codepoints;
}

int Utf8Length(const std::string & text)
{
    int count = 0;
    for(size_t pos = 0; pos < text.size(); pos += SequenceLength(static_cast<unsigned char>(text[pos])))
        ++count;
    return count;
}

std::string TruncateUtf8(const std::string & text, int maxCodepoints)
{
    size_t pos = 0;
    for(int count = 0; pos < text.size() && count < maxCodepoints; ++count)
        pos += SequenceLength(static_cast<unsigned char>(text[pos]));
    return text.substr(0, pos);
}
