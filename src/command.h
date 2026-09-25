/**
  * Copyright (C) 2017-2018 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#ifndef COMMAND_H_
#define COMMAND_H_

#include "framework/sdl_helper.h"

class Controller;

// header/footer textures, shared by reference with the main screen
struct ModernChrome
{
    Texture & gearIcon;
    Texture & appTitleText;
    Texture & appVersionText;
    Texture & creditText;
};

struct Command
{
    Command();
    Command(std::ifstream & in);
    void RunCommand(SDL_Context & sdl_context, Controller * controller, const ModernChrome & chrome, Color bg = UiTheme::Bg) const;
    void UpdateState();

    std::string name;
    std::string command;
    std::string deleteCommand;
    std::string deleteConfirmKey;
    std::string stateCommand;
    std::string enableIfCommand; // shell condition; row is skipped unless it exits 0
    bool runInternal = true;
    bool restartUI = false;
    bool ignoreInterrupt = false;
    bool usbOnly = false;
    bool child = false;
    bool isToggle = false;
    bool stateOn = false;
    bool hasSubmenu = false;
    Texture texture;
    std::string previewImage;
    int previewImageX = 920; // default position when a command sets PREVIEW_IMAGE without _X/_Y
    int previewImageY = 300;
    int previewImageWidth = -1;
    int previewImageHeight = -1;
    bool previewNearest = false; // nearest-neighbor scaling instead of linear - for small pixel-art sprites blown up a lot, where linear just blurs them
    bool previewSquare = false; // this screen's preview art is square/near-square - grid tiles should be too, not the default wide tile
    int previewGridCols = -1; // explicit grid column count override, unset (-1) defers to the square/wide default
    bool previewFitContain = false; // scale to fit within the tile instead of cropping to cover it - for portrait art in a wide/square tile
    bool previewHideLabel = false; // no caption below this tile - the art speaks for itself
};

#endif
