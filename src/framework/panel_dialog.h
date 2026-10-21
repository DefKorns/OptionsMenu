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

#ifndef PANEL_DIALOG_H_
#define PANEL_DIALOG_H_

#include "badge.h"
#include "color.h"
#include "texture.h"

#include <functional>
#include <string>
#include <vector>

struct SDL_Context;
class Controller;

enum class DialogResult { Confirmed, Cancelled, Quit };

struct PanelDialogText
{
    std::string title;
    std::vector<std::string> body;
    int hiddenCount = 0;
    std::string note;
};

// Acknowledge closes on A only. Confirm takes A, ConfirmDestructive needs Start + Select held; both cancel on B.
class PanelDialog
{
public:
    enum class Kind { Acknowledge, Confirm, ConfirmDestructive };

    PanelDialog(SDL_Renderer * renderer, BadgePainter & painter, Kind kind, const PanelDialogText & text, std::vector<Badge *> badges);
    void SetBulletColor(Color color);
    DialogResult Run(SDL_Context & sdl, Controller & controller, const std::function<void()> & drawUnder);

private:
    void Layout();
    void Draw();

    SDL_Renderer * renderer_;
    BadgePainter * painter_;
    Kind kind_;
    PanelDialogText text_;
    std::vector<Badge *> badges_;
    bool bullets_ = false;
    Color bulletColor_{ 0, 0, 0 };

    std::vector<Texture> title_;
    int titleExtraH_ = 0;
    std::vector<Texture> body_;
    int bulletRows_ = 0;
    int bulletX_ = 0;
    std::vector<Texture> note_;
    SDL_Rect box_{ 0, 0, 0, 0 };
};

// One frame only; the caller runs the blocking work right after.
void ShowBusy(SDL_Context & sdl, const std::function<void()> & drawUnder, const std::string & message);

void DropPendingInput(Controller & controller);

#endif
