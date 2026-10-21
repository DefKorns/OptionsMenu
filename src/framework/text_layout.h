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

#ifndef TEXT_LAYOUT_H_
#define TEXT_LAYOUT_H_

#include "color.h"
#include "texture.h"

#include <string>
#include <vector>

int TextWidth(const std::string & text, int fontSize);

std::string TruncateToWidth(const std::string & text, int fontSize, int maxWidth);

std::vector<std::string> WrapToWidth(const std::string & text, int fontSize, int maxWidth);

std::vector<Texture> MakeLines(const std::vector<std::string> & lines, int fontSize, SDL_Renderer * renderer, Color color);

void DrawLinesCentered(SDL_Renderer * renderer, std::vector<Texture> & lines, int centerX, int y, int pitch);

#endif
