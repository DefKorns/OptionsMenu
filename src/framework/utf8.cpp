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

std::vector<unsigned int> Utf8ToCodepoints(const std::string & text)
{
    std::vector<unsigned int> codepoints;
    for(size_t i = 0; i < text.size();)
    {
        unsigned char firstByte = static_cast<unsigned char>(text[i]);
        unsigned int codepoint = 0xFFFD;
        size_t sequenceLength = 1;

        if(firstByte < 0x80)
        {
            codepoint = firstByte;
        }
        else if(firstByte >= 0xC2 && firstByte <= 0xDF && i + 1 < text.size())
        {
            unsigned char secondByte = static_cast<unsigned char>(text[i + 1]);
            if((secondByte & 0xC0) == 0x80)
            {
                codepoint = ((firstByte & 0x1F) << 6) | (secondByte & 0x3F);
                sequenceLength = 2;
            }
        }
        else if(firstByte >= 0xE0 && firstByte <= 0xEF && i + 2 < text.size())
        {
            unsigned char secondByte = static_cast<unsigned char>(text[i + 1]);
            unsigned char thirdByte = static_cast<unsigned char>(text[i + 2]);
            if((secondByte & 0xC0) == 0x80 && (thirdByte & 0xC0) == 0x80)
            {
                codepoint = ((firstByte & 0x0F) << 12) |
                            ((secondByte & 0x3F) << 6) | (thirdByte & 0x3F);
                sequenceLength = 3;
            }
        }

        codepoints.push_back(codepoint);
        i += sequenceLength;
    }
    return codepoints;
}

int Utf8Length(const std::string & text)
{
    int count = 0;
    for(size_t i = 0; i < text.size();)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        i += (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        ++count;
    }
    return count;
}

std::string TruncateUtf8(const std::string & text, int maxCodepoints)
{
    size_t i = 0;
    int count = 0;
    while(i < text.size() && count < maxCodepoints)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        size_t len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        i += len;
        ++count;
    }
    return text.substr(0, i);
}
