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
    std::string enableIfCommand;
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
    int previewImageX = 920;
    int previewImageY = 300;
    int previewImageWidth = -1;
    int previewImageHeight = -1;
    bool previewNearest = false;
    bool previewSquare = false;
    int previewGridCols = -1;
    bool previewFitContain = false;
    bool previewHideLabel = false;
};

#endif
