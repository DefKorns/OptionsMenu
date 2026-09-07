/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "localization.h"
#include "framework/font8x8.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <png.h>

namespace
{
    const int Width = 1280;
    const int Height = 720;
    const int Scale = 3;
    const int Cell = 8 * Scale;
    const int LineSpacing = Cell + Cell/2;

    std::vector<unsigned int> Utf8ToCodepoints(const std::string & text)
    {
        std::vector<unsigned int> codepoints;
        for (size_t i = 0; i < text.size();)
        {
            unsigned char firstByte = static_cast<unsigned char>(text[i]);
            unsigned int codepoint = 0xFFFD;
            size_t sequenceLength = 1;

            if (firstByte < 0x80)
            {
                codepoint = firstByte;
            }
            else if (firstByte >= 0xC2 && firstByte <= 0xDF && i + 1 < text.size())
            {
                unsigned char secondByte = static_cast<unsigned char>(text[i + 1]);
                if ((secondByte & 0xC0) == 0x80)
                {
                    codepoint = ((firstByte & 0x1F) << 6) | (secondByte & 0x3F);
                    sequenceLength = 2;
                }
            }
            else if (firstByte >= 0xE0 && firstByte <= 0xEF && i + 2 < text.size())
            {
                unsigned char secondByte = static_cast<unsigned char>(text[i + 1]);
                unsigned char thirdByte = static_cast<unsigned char>(text[i + 2]);
                if ((secondByte & 0xC0) == 0x80 && (thirdByte & 0xC0) == 0x80)
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

    const char * GlyphFor(unsigned int codepoint)
    {
        if (codepoint < 0x80)
            return font8x8_basic[codepoint];
        if (codepoint >= 0xA0 && codepoint <= 0xFF)
            return font8x8_ext_latin[codepoint - 0xA0];
        if (codepoint >= 0x3040 && codepoint <= 0x309F)
            return font8x8_hiragana[codepoint - 0x3040];
        return font8x8_basic[0x3F];
    }

    void DrawLine(std::vector<unsigned char> & pixels, const std::string & text, int topY)
    {
        auto codepoints = Utf8ToCodepoints(text);
        int totalWidth = static_cast<int>(codepoints.size()) * Cell;
        int x0 = (Width - totalWidth) / 2;

        for (size_t i = 0; i < codepoints.size(); ++i)
        {
            const char * bitmap = GlyphFor(codepoints[i]);
            int ox = x0 + static_cast<int>(i) * Cell;
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    if (!(bitmap[y] & (1 << x)))
                        continue;
                    for (int sy = 0; sy < Scale; ++sy)
                    {
                        for (int sx = 0; sx < Scale; ++sx)
                        {
                            int px = ox + x*Scale + sx;
                            int py = topY + y*Scale + sy;
                            if (px < 0 || px >= Width || py < 0 || py >= Height)
                                continue;
                            size_t idx = (static_cast<size_t>(py) * Width + px) * 3;
                            pixels[idx] = pixels[idx+1] = pixels[idx+2] = 255;
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char * argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <output.png> <line-key> [line-key...]\n";
        return 1;
    }
    std::string outputPath = argv[1];
    int lineCount = argc - 2;

    std::string langCode("en-US");
    std::ifstream in("/etc/options_menu/language.cfg");
    std::getline(in, langCode);
    in.close();
    if (langCode.empty())
        langCode = "en-US";
    LoadLanguage("/etc/options_menu/", langCode);

    std::vector<unsigned char> pixels(Width * Height * 3, 0);
    int blockHeight = lineCount * Cell + (lineCount - 1) * (Cell/2);
    int startY = (Height - blockHeight) / 2;
    for (int i = 0; i < lineCount; ++i)
        DrawLine(pixels, Translate(argv[2 + i]), startY + i * LineSpacing);

    png_image png;
    memset(&png, 0, sizeof(png));
    png.version = PNG_IMAGE_VERSION;
    png.width = Width;
    png.height = Height;
    png.format = PNG_FORMAT_RGB;

    if (!png_image_write_to_file(&png, outputPath.c_str(), 0, pixels.data(), 0, nullptr))
    {
        std::cerr << "Cannot write png file: " << outputPath << "\n";
        return 1;
    }

    return 0;
}
