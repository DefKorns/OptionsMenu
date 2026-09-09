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

#ifndef NINESLICE_H_
#define NINESLICE_H_

#include <memory>
#include <string>
#include <SDL.h>

// Stretches a small source PNG into an arbitrary dest rect without
// distorting its corners: corners draw at fixed size, edges stretch along
// one axis, the middle stretches on both. top=bottom=0 (or left=right=0)
// degenerates to a 3-slice.
struct NineSlice
{
    std::shared_ptr<SDL_Texture> texture;
    SDL_Rect srcRect{0, 0, 0, 0};
    int left = 0, right = 0, top = 0, bottom = 0;

    NineSlice();
    NineSlice(const std::string & pngFilePath, SDL_Renderer * renderer, int left, int right, int top, int bottom);

    void Draw(SDL_Renderer * renderer, const SDL_Rect & dest) const;
    void Draw(SDL_Renderer * renderer, const SDL_Rect & dest, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) const;
};

#endif
