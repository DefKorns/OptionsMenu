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

#include <vector>

class Controller;
class ScreenChrome;

struct ChoiceOption
{
    std::string value;
    std::string label;
    std::string preview;
};

struct Command
{
    Command();
    Command(std::ifstream & in);
    void RunCommand(SDL_Context & sdl_context, Controller * controller, ScreenChrome & chrome, Color bg = UiTheme::Bg) const;
    void UpdateState();
    bool Selectable() const { return !command.empty() || isChoice; }
    void UpdateChoice();
    void SetChoice(const std::string & value);
    // the value in effect: the automatic one resolves through CHOICE_RESOLVED
    const ChoiceOption * ShownChoice() const;

    std::string name;
    std::string command;
    std::string deleteCommand;
    std::string deleteConfirmKey;
    std::string deleteBlockedKey;
    std::string confirmKey;
    std::string stateCommand;
    std::string enableIfCommand;
    bool runInternal = true;
    bool runDownload = false;
    bool restartUI = false;
    bool ignoreInterrupt = false;
    bool usbOnly = false;
    bool child = false;
    bool isToggle = false;
    bool stateOn = false;
    bool hasSubmenu = false;
    bool isChoice = false;
    std::string choiceListCommand;
    std::string choiceGetCommand;
    std::string choiceSetCommand;
    std::string choiceResolvedCommand;
    std::string choiceAutoKey;
    std::vector<ChoiceOption> choices;
    std::string choiceValue;
    std::string choiceResolved;
    std::string choiceAutoResolved;
    Texture texture;
    std::string previewImage;
    Texture valueTexture;
    std::string rowIcon;
    Texture rowIconTexture;
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
