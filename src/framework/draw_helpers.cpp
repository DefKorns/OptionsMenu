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
#include <vector>

// corner arc points within [radius-width, radius], cached per (radius,width)
static const std::vector<SDL_Point> & RoundedCornerArc(int radius, int width)
{
    static std::map<std::pair<int,int>, std::vector<SDL_Point>> cache;
    auto key = std::make_pair(radius, width);
    auto it = cache.find(key);
    if(it != cache.end())
        return it->second;
    std::vector<SDL_Point> pts;
    for(int dy = 0; dy <= radius; ++dy)
        for(int dx = 0; dx <= radius; ++dx)
        {
            double dist = std::sqrt(static_cast<double>(dx*dx + dy*dy));
            if(dist <= radius + 0.5 && dist > radius - width + 0.5)
                pts.push_back({dx, dy});
        }
    return cache.emplace(key, std::move(pts)).first->second;
}

void DrawStrokeRect(SDL_Renderer * r, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb, int width, int radius)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 0xFF);
    int x0 = rect.x, y0 = rect.y, x1 = rect.x+rect.w-1, y1 = rect.y+rect.h-1;
    for(int i = 0; i < width; ++i)
    {
        SDL_RenderDrawLine(r, x0+radius, y0+i, x1-radius, y0+i);   // top
        SDL_RenderDrawLine(r, x0+radius, y1-i, x1-radius, y1-i);   // bottom
        SDL_RenderDrawLine(r, x0+i, y0+radius, x0+i, y1-radius);   // left
        SDL_RenderDrawLine(r, x1-i, y0+radius, x1-i, y1-radius);   // right
    }
    if(radius <= 0)
        return;
    const std::vector<SDL_Point> & arc = RoundedCornerArc(radius, width);
    std::vector<SDL_Point> points;
    points.reserve(arc.size() * 4);
    for(const SDL_Point & p : arc)
    {
        points.push_back({ x0+radius-p.x, y0+radius-p.y }); // top-left
        points.push_back({ x1-radius+p.x, y0+radius-p.y }); // top-right
        points.push_back({ x0+radius-p.x, y1-radius+p.y }); // bottom-left
        points.push_back({ x1-radius+p.x, y1-radius+p.y }); // bottom-right
    }
    SDL_RenderDrawPoints(r, points.data(), static_cast<int>(points.size()));
}

void DrawFillRect(SDL_Renderer * r, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 0xFF);
    SDL_RenderFillRect(r, &rect);
}

// like RoundedCornerArc but the full quarter-disk, for a solid fill
static const std::vector<SDL_Point> & RoundedCornerDisk(int radius)
{
    static std::map<int, std::vector<SDL_Point>> cache;
    auto it = cache.find(radius);
    if(it != cache.end())
        return it->second;
    std::vector<SDL_Point> pts;
    for(int dy = 0; dy <= radius; ++dy)
        for(int dx = 0; dx <= radius; ++dx)
            if(std::sqrt(static_cast<double>(dx*dx + dy*dy)) <= radius + 0.5)
                pts.push_back({dx, dy});
    return cache.emplace(radius, std::move(pts)).first->second;
}

void DrawRoundedFillRect(SDL_Renderer * r, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb, int radius)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 0xFF);
    if(radius <= 0)
    {
        SDL_RenderFillRect(r, &rect);
        return;
    }
    int x0 = rect.x, y0 = rect.y, x1 = rect.x+rect.w-1, y1 = rect.y+rect.h-1;
    SDL_Rect mid{ x0, y0+radius, rect.w, rect.h-2*radius };
    SDL_Rect midV{ x0+radius, y0, rect.w-2*radius, rect.h };
    SDL_RenderFillRect(r, &mid);
    SDL_RenderFillRect(r, &midV);
    const std::vector<SDL_Point> & disk = RoundedCornerDisk(radius);
    std::vector<SDL_Point> points;
    points.reserve(disk.size() * 4);
    for(const SDL_Point & p : disk)
    {
        points.push_back({ x0+radius-p.x, y0+radius-p.y });
        points.push_back({ x1-radius+p.x, y0+radius-p.y });
        points.push_back({ x0+radius-p.x, y1-radius+p.y });
        points.push_back({ x1-radius+p.x, y1-radius+p.y });
    }
    SDL_RenderDrawPoints(r, points.data(), static_cast<int>(points.size()));
}

void DrawHLine(SDL_Renderer * r, int x0, int x1, int y, Uint8 cr, Uint8 cg, Uint8 cb, int width)
{
    DrawFillRect(r, { x0, y, x1-x0, width }, cr, cg, cb);
}

void DrawVLine(SDL_Renderer * r, int x, int y0, int y1, Uint8 cr, Uint8 cg, Uint8 cb, int width)
{
    DrawFillRect(r, { x, y0, width, y1-y0 }, cr, cg, cb);
}
