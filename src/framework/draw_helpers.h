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

#include <SDL.h>

// flat-line drawing for Modern UI - no textured art
void DrawStrokeRect(SDL_Renderer * r, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb, int width, int radius = 0);
void DrawFillRect(SDL_Renderer * r, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb);
void DrawRoundedFillRect(SDL_Renderer * r, SDL_Rect rect, Uint8 cr, Uint8 cg, Uint8 cb, int radius);
void DrawHLine(SDL_Renderer * r, int x0, int x1, int y, Uint8 cr, Uint8 cg, Uint8 cb, int width = 1);
void DrawVLine(SDL_Renderer * r, int x, int y0, int y1, Uint8 cr, Uint8 cg, Uint8 cb, int width = 1);

#endif
