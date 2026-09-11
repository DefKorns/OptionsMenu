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
    void RunCommand(SDL_Context & sdl_context, Controller * controller, const ModernChrome & chrome, Uint8 bgR = UiTheme::BgR, Uint8 bgG = UiTheme::BgG, Uint8 bgB = UiTheme::BgB) const;
    void UpdateState();

    std::string name;
    std::string command;
    std::string deleteCommand;
    std::string deleteConfirmKey;
    std::string stateCommand;
    bool runInternal = true;
    bool restartUI = false;
    bool ignoreInterrupt = false;
    bool usbOnly = false;
    bool child = false;
    bool isToggle = false;
    bool stateOn = false;
    bool hasSubmenu = false; // COMMAND_STR relaunches options with a different --commandPath
    Texture texture;
    std::string previewImage;
    int previewImageX = 920; // default position when a command sets PREVIEW_IMAGE without _X/_Y
    int previewImageY = 300;
    int previewImageWidth = -1;
    int previewImageHeight = -1;
};

#endif
