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

#ifndef DRAW_HELPERS_H_
#define DRAW_HELPERS_H_

#include "color.h"

#include <SDL.h>

void SetDrawColor(SDL_Renderer * renderer, Color color);
// tints a white asset texture
void SetColorMod(SDL_Texture * texture, Color color);

// flat-line drawing for Modern UI - no textured art
void DrawStrokeRect(SDL_Renderer * renderer, SDL_Rect rect, Color color, int width, int radius = 0);
void DrawFillRect(SDL_Renderer * renderer, SDL_Rect rect, Color color);
void DrawRoundedFillRect(SDL_Renderer * renderer, SDL_Rect rect, Color color, int radius);
// paints the corner-square pixels outside the rounded radius with a flat
// color - drawn over a rectangularly-clipped image, it fakes rounding it
// without an alpha mask or render target
void DrawRoundedCornerMask(SDL_Renderer * renderer, SDL_Rect rect, Color color, int radius);
void DrawHLine(SDL_Renderer * renderer, int x0, int x1, int y, Color color, int width = 1);
void DrawVLine(SDL_Renderer * renderer, int x, int y0, int y1, Color color, int width = 1);

#endif
