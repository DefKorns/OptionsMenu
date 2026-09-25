/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#ifndef COLOR_H_
#define COLOR_H_

#include <SDL.h>

// opaque RGB; alpha is always 0xFF wherever it's drawn
struct Color
{
    Uint8 r, g, b;
};

// packed AABBGGRR, the layout Texture's text color parameter takes
constexpr Uint32 ToAbgr(Color color)
{
    return 0xFF000000u | (static_cast<Uint32>(color.b) << 16) | (static_cast<Uint32>(color.g) << 8) | color.r;
}

#endif
