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

#include "nineslice.h"
#include "texture.h"

NineSlice::NineSlice() {}

NineSlice::NineSlice(const std::string & pngFilePath, SDL_Renderer * renderer, int left, int right, int top, int bottom)
    : left(left), right(right), top(top), bottom(bottom)
{
    texture = std::shared_ptr<SDL_Texture>(LoadTexturePNG(renderer, pngFilePath, &srcRect), SDL_DestroyTexture);
}

void NineSlice::Draw(SDL_Renderer * renderer, const SDL_Rect & dest) const
{
    if(!texture.get())
        return;

    const int srcW = srcRect.w, srcH = srcRect.h;
    const int colSrcX[3] = { 0, left, srcW - right };
    const int colSrcW[3] = { left, srcW - left - right, right };
    const int colDstX[3] = { dest.x, dest.x + left, dest.x + dest.w - right };
    const int colDstW[3] = { left, dest.w - left - right, right };

    const int rowSrcY[3] = { 0, top, srcH - bottom };
    const int rowSrcH[3] = { top, srcH - top - bottom, bottom };
    const int rowDstY[3] = { dest.y, dest.y + top, dest.y + dest.h - bottom };
    const int rowDstH[3] = { top, dest.h - top - bottom, bottom };

    for(int col = 0; col < 3; ++col)
    {
        if(colSrcW[col] <= 0 || colDstW[col] <= 0)
            continue;
        for(int row = 0; row < 3; ++row)
        {
            if(rowSrcH[row] <= 0 || rowDstH[row] <= 0)
                continue;
            SDL_Rect src{ srcRect.x + colSrcX[col], srcRect.y + rowSrcY[row], colSrcW[col], rowSrcH[row] };
            SDL_Rect dst{ colDstX[col], rowDstY[row], colDstW[col], rowDstH[row] };
            SDL_RenderCopyEx(renderer, texture.get(), &src, &dst, 0, NULL, SDL_FLIP_NONE);
        }
    }
}

void NineSlice::Draw(SDL_Renderer * renderer, const SDL_Rect & dest, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const
{
    if(!texture.get())
        return;

    SDL_SetTextureColorMod(texture.get(), r, g, b);
    SDL_SetTextureAlphaMod(texture.get(), a);
    Draw(renderer, dest);
    SDL_SetTextureColorMod(texture.get(), 255, 255, 255);
    SDL_SetTextureAlphaMod(texture.get(), 255);
}
