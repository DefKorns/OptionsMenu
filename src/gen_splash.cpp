/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "localization.h"
#include "framework/font8x8_lookup.h"
#include "framework/utf8.h"

#include <iostream>
#include <string>
#include <vector>
#include <png.h>

namespace
{
    const std::string OptionsRoot = "/etc/options_menu/";
    constexpr int ScreenW = 1280;
    constexpr int ScreenH = 720;
    constexpr int BytesPerPixel = 3;
    constexpr int GlyphBits = 8; // font8x8 glyphs are 8x8 bitmaps
    constexpr int Scale = 3;
    constexpr int GlyphPx = GlyphBits * Scale;
    constexpr int LineGap = GlyphPx / 2;
    constexpr int LineSpacing = GlyphPx + LineGap;
    constexpr unsigned char White = 255;

    // one line of text, horizontally centered, top edge at topY
    void DrawTextLine(std::vector<unsigned char> & pixels, const std::string & text, int topY)
    {
        const std::vector<unsigned int> codepoints = Utf8ToCodepoints(text);
        const int lineW = static_cast<int>(codepoints.size()) * GlyphPx;
        const int lineX = (ScreenW - lineW) / 2;

        for(size_t i = 0; i < codepoints.size(); ++i)
        {
            const char * glyph = Font8x8Glyph(codepoints[i]);
            const int glyphX = lineX + static_cast<int>(i) * GlyphPx;
            for(int row = 0; row < GlyphBits; ++row)
                for(int col = 0; col < GlyphBits; ++col)
                {
                    if(!(glyph[row] & (1 << col)))
                        continue;
                    for(int sy = 0; sy < Scale; ++sy)
                        for(int sx = 0; sx < Scale; ++sx)
                        {
                            const int px = glyphX + col * Scale + sx;
                            const int py = topY + row * Scale + sy;
                            if(px < 0 || px >= ScreenW || py < 0 || py >= ScreenH)
                                continue;
                            const size_t offset = (static_cast<size_t>(py) * ScreenW + px) * BytesPerPixel;
                            pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = White;
                        }
                }
        }
    }
}

int main(int argc, char * argv[])
{
    if(argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <output.png> <line-key> [line-key...]\n";
        return 1;
    }
    const std::string outputPath = argv[1];
    const int lineCount = argc - 2;

    LoadLanguageFromConfig(OptionsRoot);

    std::vector<unsigned char> pixels(static_cast<size_t>(ScreenW) * ScreenH * BytesPerPixel, 0);
    const int blockH = lineCount * LineSpacing - LineGap;
    const int firstLineY = (ScreenH - blockH) / 2;
    for(int i = 0; i < lineCount; ++i)
        DrawTextLine(pixels, Translate(argv[2 + i]), firstLineY + i * LineSpacing);

    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    png.width = ScreenW;
    png.height = ScreenH;
    png.format = PNG_FORMAT_RGB;

    if(!png_image_write_to_file(&png, outputPath.c_str(), 0, pixels.data(), 0, nullptr))
    {
        std::cerr << "Cannot write png file: " << outputPath << "\n";
        return 1;
    }

    return 0;
}
