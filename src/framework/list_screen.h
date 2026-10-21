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

#ifndef LIST_SCREEN_H_
#define LIST_SCREEN_H_

#include "texture.h"

#include <SDL.h>
#include <string>
#include <vector>

class ScreenChrome
{
public:
    ScreenChrome(const std::string & optionsLocation, SDL_Renderer * renderer, const std::string & appTitle,
                 const std::string & version, const std::string & credit);
    void SetSectionTitle(const std::string & title);
    const SDL_Rect & SectionTitleRect() const;
    // everything but the section title, for screens that lay out their own
    void DrawFrame();
    void Draw();

private:
    SDL_Renderer * renderer_;
    Texture gear_;
    Texture appTitle_;
    Texture appVersion_;
    Texture sectionTitle_;
    Texture credit_;
};

void DrawFooterDivider(SDL_Renderer * renderer, int rightEdge);

SDL_Rect DrawListRow(SDL_Renderer * renderer, int index, int rightX, int rowHeight, bool selected);

class ScrollArrows
{
public:
    ScrollArrows(const std::string & optionsLocation, SDL_Renderer * renderer);
    void Draw(bool moreAbove, bool moreBelow);

private:
    SDL_Renderer * renderer_;
    Texture up_;
    Texture down_;
};

class NoteBox
{
public:
    NoteBox(SDL_Renderer * renderer, SDL_Rect rect);
    void SetText(const std::string & text);
    void Draw();

private:
    SDL_Renderer * renderer_;
    SDL_Rect rect_;
    std::vector<Texture> lines_;
};

#endif
