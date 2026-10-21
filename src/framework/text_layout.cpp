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

#include "text_layout.h"
#include "utf8.h"

int TextWidth(const std::string & text, int fontSize)
{
    if(CanRenderWithTTF(text, fontSize))
        return MeasureTTFWidth(text, fontSize);
    return Utf8Length(text) * fontSize;
}

std::string TruncateToWidth(const std::string & text, int fontSize, int maxWidth)
{
    if(TextWidth(text, fontSize) <= maxWidth)
        return text;
    int n = Utf8Length(text);
    while(n > 0 && TextWidth(TruncateUtf8(text, n) + "...", fontSize) > maxWidth)
        --n;
    return TruncateUtf8(text, n) + "...";
}

std::vector<std::string> WrapToWidth(const std::string & text, int fontSize, int maxWidth)
{
    std::vector<std::string> lines;
    std::string line;
    size_t pos = 0;
    while(pos < text.size())
    {
        size_t end = text.find(' ', pos);
        std::string word = text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        pos = (end == std::string::npos) ? text.size() : end + 1;

        std::string candidate = line.empty() ? word : line + " " + word;
        if(TextWidth(candidate, fontSize) <= maxWidth)
        {
            line = candidate;
            continue;
        }
        if(!line.empty())
            lines.push_back(line);
        line = word;
        while(TextWidth(line, fontSize) > maxWidth && Utf8Length(line) > 1)
        {
            int n = Utf8Length(line) - 1;
            while(n > 1 && TextWidth(TruncateUtf8(line, n), fontSize) > maxWidth)
                --n;
            std::string head = TruncateUtf8(line, n);
            lines.push_back(head);
            line = line.substr(head.size());
        }
    }
    if(!line.empty())
        lines.push_back(line);
    return lines;
}

std::vector<Texture> MakeLines(const std::vector<std::string> & lines, int fontSize, SDL_Renderer * renderer, Color color)
{
    std::vector<Texture> textures;
    for(const std::string & line : lines)
        textures.emplace_back(line, fontSize, renderer, 0, 0, false, ToAbgr(color), true);
    return textures;
}

void DrawLinesCentered(SDL_Renderer * renderer, std::vector<Texture> & lines, int centerX, int y, int pitch)
{
    for(Texture & line : lines)
    {
        line.rect.x = centerX - line.rect.w / 2;
        line.rect.y = y;
        line.Draw(renderer);
        y += pitch;
    }
}
