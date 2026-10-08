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
#include "framework/uitheme.h"
#include "framework/utf8.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <linux/fb.h>
#include <png.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace
{
    const std::string OptionsRoot = "/etc/options_menu/";
    constexpr int ScreenW = 1280;
    constexpr int ScreenH = 720;
    constexpr int BytesPerPixel = 3;
    constexpr int GlyphBits = 8;
    constexpr int Scale = 3;
    constexpr int GlyphPx = GlyphBits * Scale;
    constexpr int LineGap = GlyphPx / 2;
    constexpr int LineSpacing = GlyphPx + LineGap;
    constexpr unsigned char White = 255;

    constexpr int BarW = 320;
    constexpr int BarH = 8;
    constexpr int BarGap = 32;
    constexpr int SegmentW = 80;
    constexpr int SweepMs = 1400;
    constexpr int TickMs = 33;

    using Pixels = std::vector<unsigned char>;

    void DrawTextLine(Pixels & pixels, const std::string & text, int topY)
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

    struct TextBlock
    {
        Pixels pixels;
        int bottomY;
    };

    std::string WithoutTrailingEllipsis(std::string text)
    {
        const std::string ellipsis = "…";
        for(;;)
        {
            if(!text.empty() && (text.back() == '.' || text.back() == ' '))
                text.pop_back();
            else if(text.size() >= ellipsis.size() && text.compare(text.size() - ellipsis.size(), ellipsis.size(), ellipsis) == 0)
                text.resize(text.size() - ellipsis.size());
            else
                return text;
        }
    }

    TextBlock RenderText(const std::vector<std::string> & keys, bool withBar)
    {
        TextBlock block{ Pixels(static_cast<size_t>(ScreenW) * ScreenH * BytesPerPixel, 0), 0 };
        const int lineCount = static_cast<int>(keys.size());
        const int blockH = lineCount * LineSpacing - LineGap;
        const int firstLineY = (ScreenH - blockH) / 2;
        for(int i = 0; i < lineCount; ++i)
        {
            const std::string line = Translate(keys[i]);
            DrawTextLine(block.pixels, withBar && i == lineCount - 1 ? WithoutTrailingEllipsis(line) : line, firstLineY + i * LineSpacing);
        }
        block.bottomY = firstLineY + blockH;
        return block;
    }

    bool WritePng(const Pixels & pixels, const std::string & outputPath)
    {
        png_image png{};
        png.version = PNG_IMAGE_VERSION;
        png.width = ScreenW;
        png.height = ScreenH;
        png.format = PNG_FORMAT_RGB;
        return png_image_write_to_file(&png, outputPath.c_str(), 0, pixels.data(), 0, nullptr) != 0;
    }

    class Framebuffer
    {
    public:
        Framebuffer()
        {
            fd_ = open("/dev/fb0", O_RDWR);
            if(fd_ < 0)
                return;
            fb_fix_screeninfo fix{};
            if(ioctl(fd_, FBIOGET_VSCREENINFO, &var_) < 0 || ioctl(fd_, FBIOGET_FSCREENINFO, &fix) < 0 ||
               (var_.bits_per_pixel != 16 && var_.bits_per_pixel != 32))
                return;
            stride_ = fix.line_length;
            size_ = static_cast<size_t>(fix.smem_len);
            void * mem = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
            if(mem != MAP_FAILED)
                mem_ = static_cast<unsigned char *>(mem);
        }
        ~Framebuffer()
        {
            if(mem_)
                munmap(mem_, size_);
            if(fd_ >= 0)
                close(fd_);
        }
        Framebuffer(const Framebuffer &) = delete;
        Framebuffer & operator=(const Framebuffer &) = delete;

        bool Ready() const { return mem_ != nullptr; }

        void Put(int x, int y, Color color)
        {
            if(x < 0 || y < 0 || x >= static_cast<int>(var_.xres) || y >= static_cast<int>(var_.yres))
                return;
            const size_t offset = static_cast<size_t>(y + var_.yoffset) * stride_ +
                                  static_cast<size_t>(x + var_.xoffset) * (var_.bits_per_pixel / 8);
            if(offset + var_.bits_per_pixel / 8 > size_)
                return;
            const Uint32 value = Channel(color.r, var_.red) | Channel(color.g, var_.green) |
                                 Channel(color.b, var_.blue) | Channel(0xFF, var_.transp);
            if(var_.bits_per_pixel == 32)
                std::memcpy(mem_ + offset, &value, 4);
            else
            {
                const Uint16 value16 = static_cast<Uint16>(value);
                std::memcpy(mem_ + offset, &value16, 2);
            }
        }

        void Fill(int x, int y, int w, int h, Color color)
        {
            for(int row = y; row < y + h; ++row)
                for(int col = x; col < x + w; ++col)
                    Put(col, row, color);
        }

    private:
        static Uint32 Channel(unsigned char value, const fb_bitfield & field)
        {
            if(field.length == 0)
                return 0;
            return (static_cast<Uint32>(value) >> (8 - field.length)) << field.offset;
        }

        int fd_ = -1;
        fb_var_screeninfo var_{};
        size_t stride_ = 0;
        size_t size_ = 0;
        unsigned char * mem_ = nullptr;
    };

    bool PidAlive(pid_t pid)
    {
        return kill(pid, 0) == 0 || errno == EPERM;
    }

    void DrawBar(Framebuffer & fb, int barY, int elapsedMs)
    {
        const int barX = (ScreenW - BarW) / 2;
        const int segmentX = barX - SegmentW + (BarW + SegmentW) * (elapsedMs % SweepMs) / SweepMs;
        const int visibleX = std::max(segmentX, barX);
        const int visibleEnd = std::min(segmentX + SegmentW, barX + BarW);
        fb.Fill(barX, barY, BarW, BarH, UiTheme::Border);
        if(visibleEnd > visibleX)
            fb.Fill(visibleX, barY, visibleEnd - visibleX, BarH, UiTheme::Accent);
    }

    void DrawText(Framebuffer & fb, const TextBlock & block)
    {
        for(int y = 0; y < ScreenH; ++y)
            for(int x = 0; x < ScreenW; ++x)
            {
                const size_t offset = (static_cast<size_t>(y) * ScreenW + x) * BytesPerPixel;
                fb.Put(x, y, { block.pixels[offset], block.pixels[offset + 1], block.pixels[offset + 2] });
            }
    }

    void DetachFromCaller()
    {
        setsid();
        const int devNull = open("/dev/null", O_RDWR);
        if(devNull < 0)
            return;
        dup2(devNull, STDIN_FILENO);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
        if(devNull > STDERR_FILENO)
            close(devNull);
    }

    int RunLoading()
    {
        const pid_t caller = getppid();
        const TextBlock block = RenderText({ "LOADING" }, true);
        UiTheme::LoadThemeConfig(OptionsRoot);
        const int barY = block.bottomY + BarGap;

        Framebuffer fb;
        if(!fb.Ready())
        {
            std::cerr << "Cannot open /dev/fb0\n";
            return 1;
        }
        DrawText(fb, block);
        DrawBar(fb, barY, 0);

        const pid_t child = fork();
        if(child != 0)
            return child < 0 ? 1 : 0;

        DetachFromCaller();
        for(int elapsedMs = TickMs; PidAlive(caller); elapsedMs += TickMs)
        {
            usleep(TickMs * 1000);
            DrawBar(fb, barY, elapsedMs);
        }
        return 0;
    }

    void PrintUsage(const char * self)
    {
        std::cerr << "Usage: " << self << " <output.png> <line-key> [line-key...]\n"
                  << "       " << self << " --loading\n";
    }
}

int main(int argc, char * argv[])
{
    const std::vector<std::string> args(argv + 1, argv + argc);
    if(args.size() == 1 && args[0] == "--loading")
    {
        LoadLanguageFromConfig(OptionsRoot);
        return RunLoading();
    }
    if(args.size() < 2)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    LoadLanguageFromConfig(OptionsRoot);
    const TextBlock block = RenderText(std::vector<std::string>(args.begin() + 1, args.end()), false);
    if(!WritePng(block.pixels, args[0]))
    {
        std::cerr << "Cannot write png file: " << args[0] << "\n";
        return 1;
    }
    return 0;
}
