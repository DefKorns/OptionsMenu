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

#include "draw_helpers.h"

#include <cmath>
#include <map>
#include <tuple>
#include <vector>

namespace
{
    using CornerPoints = std::vector<SDL_Point>;

    enum class CornerRegion { Arc, Disk, Outside };

    // offsets (dx, dy) from a corner's circle center, within one quadrant.
    // Arc: ring of the given width on the radius. Disk: full quarter-disk.
    // Outside: corner-square pixels beyond the radius. Cached per shape.
    const CornerPoints & CornerOffsets(CornerRegion region, int radius, int width = 0)
    {
        static std::map<std::tuple<CornerRegion, int, int>, CornerPoints> cache;
        const auto key = std::make_tuple(region, radius, width);
        const auto it = cache.find(key);
        if(it != cache.end())
            return it->second;

        CornerPoints points;
        for(int dy = 0; dy <= radius; ++dy)
            for(int dx = 0; dx <= radius; ++dx)
            {
                const double dist = std::sqrt(static_cast<double>(dx*dx + dy*dy));
                const bool insideRadius = dist <= radius + 0.5;
                const bool inRegion =
                    region == CornerRegion::Arc ? insideRadius && dist > radius - width + 0.5 :
                    region == CornerRegion::Disk ? insideRadius :
                    !insideRadius;
                if(inRegion)
                    points.push_back({ dx, dy });
            }
        return cache.emplace(key, std::move(points)).first->second;
    }

    // mirrors quadrant offsets into all 4 corners of rect and plots them
    void DrawMirroredCorners(SDL_Renderer * renderer, SDL_Rect rect, int radius, const CornerPoints & offsets)
    {
        const int x0 = rect.x, y0 = rect.y, x1 = rect.x + rect.w - 1, y1 = rect.y + rect.h - 1;
        // reused across calls (single render thread) to avoid per-frame heap allocation
        static CornerPoints points;
        points.clear();
        points.reserve(offsets.size() * 4);
        for(const SDL_Point & p : offsets)
        {
            points.push_back({ x0 + radius - p.x, y0 + radius - p.y }); // top-left
            points.push_back({ x1 - radius + p.x, y0 + radius - p.y }); // top-right
            points.push_back({ x0 + radius - p.x, y1 - radius + p.y }); // bottom-left
            points.push_back({ x1 - radius + p.x, y1 - radius + p.y }); // bottom-right
        }
        SDL_RenderDrawPoints(renderer, points.data(), static_cast<int>(points.size()));
    }
}

void DrawStrokeRect(SDL_Renderer * renderer, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb, int width, int radius)
{
    SDL_SetRenderDrawColor(renderer, cr, cg, cb, 0xFF);
    const int x0 = rect.x, y0 = rect.y, x1 = rect.x + rect.w - 1, y1 = rect.y + rect.h - 1;
    for(int i = 0; i < width; ++i)
    {
        SDL_RenderDrawLine(renderer, x0 + radius, y0 + i, x1 - radius, y0 + i); // top
        SDL_RenderDrawLine(renderer, x0 + radius, y1 - i, x1 - radius, y1 - i); // bottom
        SDL_RenderDrawLine(renderer, x0 + i, y0 + radius, x0 + i, y1 - radius); // left
        SDL_RenderDrawLine(renderer, x1 - i, y0 + radius, x1 - i, y1 - radius); // right
    }
    if(radius > 0)
        DrawMirroredCorners(renderer, rect, radius, CornerOffsets(CornerRegion::Arc, radius, width));
}

void DrawFillRect(SDL_Renderer * renderer, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(renderer, cr, cg, cb, 0xFF);
    SDL_RenderFillRect(renderer, &rect);
}

void DrawRoundedFillRect(SDL_Renderer * renderer, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb, int radius)
{
    SDL_SetRenderDrawColor(renderer, cr, cg, cb, 0xFF);
    if(radius <= 0)
    {
        SDL_RenderFillRect(renderer, &rect);
        return;
    }
    // cross of two bands, corners filled by the quarter-disks
    const SDL_Rect hBand{ rect.x, rect.y + radius, rect.w, rect.h - 2*radius };
    const SDL_Rect vBand{ rect.x + radius, rect.y, rect.w - 2*radius, rect.h };
    SDL_RenderFillRect(renderer, &hBand);
    SDL_RenderFillRect(renderer, &vBand);
    DrawMirroredCorners(renderer, rect, radius, CornerOffsets(CornerRegion::Disk, radius));
}

void DrawRoundedCornerMask(SDL_Renderer * renderer, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb, int radius)
{
    if(radius <= 0)
        return;
    SDL_SetRenderDrawColor(renderer, cr, cg, cb, 0xFF);
    DrawMirroredCorners(renderer, rect, radius, CornerOffsets(CornerRegion::Outside, radius));
}

void DrawHLine(SDL_Renderer * renderer, int x0, int x1, int y, Uint8 cr, Uint8 cg, Uint8 cb, int width)
{
    DrawFillRect(renderer, { x0, y, x1 - x0, width }, cr, cg, cb);
}

void DrawVLine(SDL_Renderer * renderer, int x, int y0, int y1, Uint8 cr, Uint8 cg, Uint8 cb, int width)
{
    DrawFillRect(renderer, { x, y0, width, y1 - y0 }, cr, cg, cb);
}
