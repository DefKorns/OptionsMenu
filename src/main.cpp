/**
  * Copyright (C) 2017-2018 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "framework/sdl_helper.h"
#include "framework/controller.h"
#include "framework/powerwatch.h"
#include "command.h"
#include "localization.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <dirent.h>

#ifndef MOD_VERSION
#define MOD_VERSION "dev"
#endif

void sReplace(std::string & command, std::string oldString, std::string newString)
{
    size_t pos;
    while((pos = command.find(oldString)) != std::string::npos)
    {
        command.replace(pos, oldString.size(), newString);
    }
}

static struct ExitManager
{
    std::string exitCommand;
    bool runExitCommand = true;
    ~ExitManager()
    {
        if(runExitCommand)
            system(exitCommand.c_str());
    }
} _exitManager;

int main(int argc, char * argv[])
{
    std::string optionsLocation(argv[0]);
    optionsLocation = optionsLocation.substr(0, optionsLocation.find_last_of('/')+1);
    _exitManager.exitCommand = "/bin/sh " + optionsLocation + "/scripts/ResumeUI.sh";
    std::string commandLocation(optionsLocation + "commands/");
    std::string scriptLocation(optionsLocation + "scripts/");
    std::string spriteSheetLocation;
    std::ifstream in("/tmp/spritesheet");
    std::getline(in, spriteSheetLocation);
    in.close();

    std::string langCode("en-US");
    in.open("/etc/options_menu/language.cfg");
    std::getline(in, langCode);
    in.close();
    if(langCode.empty())
        langCode = "en-US";
    LoadLanguage(optionsLocation, langCode);

    std::string uiStyle;
    in.open("/etc/options_menu/ui_style.cfg");
    std::getline(in, uiStyle);
    in.close();
    bool modernUI = (uiStyle != "classic");

    std::string titleString(Translate("OPTIONS_TITLE"));

    //Check for external drive
    bool usbReady = false;
    if(auto dir = opendir("/media/hakchi/"))
    {
        closedir(dir);
        usbReady = true;
    }

    //Parse args
    for(int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "--commandPath") == 0)
        {
            commandLocation = argv[i+1];
            ++i;
        }
        else if(strcmp(argv[i], "--scriptPath") == 0)
        {
            scriptLocation = argv[i+1];
            ++i;
        }
        else if(strcmp(argv[i], "--title") == 0)
        {
            titleString = Translate(argv[i+1]);
            ++i;
        }
    }

    //Read commands from command folder
    std::vector<Command> commands;
    if(auto dir = opendir(commandLocation.c_str()))
    {
        //Find all commands in folder
        std::list<std::string> fileList;
        while(auto entry = readdir(dir))
        {
            if(entry->d_type == DT_REG && entry->d_name[0] == 'c')
                fileList.push_back(entry->d_name);
        }
        closedir(dir);

        fileList.sort();
        for(auto & file : fileList)
        {
            in.open(commandLocation+file);
            if(in.is_open())
            {
                Command c(in);
                if(!(commands.size() == 0 && c.command.size() == 0))
                {
                    if(!c.usbOnly || (c.usbOnly && usbReady))
                    {
                        sReplace(c.command, "%options_path%", optionsLocation);
                        sReplace(c.command, "%script_dir%", scriptLocation);
                        sReplace(c.deleteCommand, "%options_path%", optionsLocation);
                        sReplace(c.deleteCommand, "%script_dir%", scriptLocation);
                        if(c.isToggle)
                        {
                            sReplace(c.stateCommand, "%options_path%", optionsLocation);
                            sReplace(c.stateCommand, "%script_dir%", scriptLocation);
                            c.UpdateState();
                        }
                        commands.push_back(c);
                    }
                }
            }
        }
    }
    else
    {
        std::cerr << "Cannot open input folder.\n";
        exit(1);
    }

    int currentCommandId = 0;
    const int DisplayItemCount = 16;

    //Create SDL Window/Renderer and Controller Handler
    SDL_Context sdl_context(std::chrono::milliseconds(33), false);
    auto renderer = sdl_context.renderer;
    Controller controller(1);

    //Create Options Flag to prevent multiple menu launches
    system("touch /tmp/options.flag");

    //Create Background
    auto spriteSheet = std::shared_ptr<SDL_Texture>(LoadTexturePNG(renderer, spriteSheetLocation), SDL_DestroyTexture);
    Texture banner(optionsLocation + "/images/banner.png", renderer, 640, 185, true);
    Sprite menuU = { spriteSheet, {1,43,432,40}, {-8,-24,1296,120} };
    Sprite menuL = { spriteSheet, {1,1,432,40}, {-8,630,1296,120} };
    const Uint8 bgR = modernUI ? UiTheme::BgR : 0x6e;
    const Uint8 bgG = modernUI ? UiTheme::BgG : 0x6e;
    const Uint8 bgB = modernUI ? UiTheme::BgB : 0x6e;
    SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 0xFF);

    //Create dialog frame + selection highlight (own art, not console theme sprites)
    NineSlice frame(optionsLocation + UiTheme::AssetFrame, renderer, UiTheme::FrameInset, UiTheme::FrameInset, UiTheme::FrameInset, UiTheme::FrameInset);
    NineSlice highlight(optionsLocation + UiTheme::AssetHighlight, renderer, UiTheme::HighlightInsetLR, UiTheme::HighlightInsetLR, 0, 0);
    Texture gearIcon(optionsLocation + UiTheme::AssetGear, renderer, UiTheme::GearX, UiTheme::GearY);
    Texture switchOn(optionsLocation + UiTheme::AssetSwitchOn, renderer);
    Texture switchOff(optionsLocation + UiTheme::AssetSwitchOff, renderer);
    Texture badgeOuter(optionsLocation + UiTheme::AssetBadgeOuter, renderer);
    Texture badgeInner(optionsLocation + UiTheme::AssetBadgeInner, renderer);

    //Create Textures for Strings
    Texture appTitleText("OptionsMenu", UiTheme::TitleFontSize, renderer, UiTheme::TitleX, UiTheme::TitleY, false, 0xFFFFFFFF);
    appTitleText.rect.y -= appTitleText.rect.h / 2; // vertically center on the gear (Texture only supports centering both axes together)
    Texture appVersionText(MOD_VERSION, UiTheme::VersionFontSize, renderer, appTitleText.rect.x + appTitleText.rect.w + UiTheme::VersionGap, UiTheme::TitleY, false, 0xFFFFFFFF);
    appVersionText.rect.y -= appVersionText.rect.h / 2;
    Texture titleText = modernUI
        ? Texture(titleString, UiTheme::SectionTitleFontSize, renderer, UiTheme::SectionTitleCenterX, UiTheme::SectionTitleY, true)
        : Texture(titleString, 36, renderer, 640, 185, true);
    SDL_Rect highlightRect{ UiTheme::HighlightX, UiTheme::RowFirstY - 2, UiTheme::HighlightW, UiTheme::HighlightH };
    Texture pointerText("->", 16, renderer, 20, UiTheme::RowFirstY, false, 0xFF00FF00);
    SDL_Rect & pointerRect = pointerText.rect;
    Texture CompComText = modernUI
        ? Texture("created by CompCom", 16, renderer)
        : Texture("created by CompCom", 16, renderer, 1100, 620, true);
    if(modernUI)
    {
        CompComText.rect.x = UiTheme::CreditRightX - CompComText.rect.w;
        CompComText.rect.y = UiTheme::CreditY;
    }
    Texture deleteHint(Translate("WIFI_DELETE_HINT_FOOTER"), 16, renderer, 30, 612);
    Texture scrollUp = modernUI ? Texture("^", 16, renderer, UiTheme::ScrollX, UiTheme::ScrollUpY) : Texture("^", 16, renderer, 30, 248);
    Texture scrollDown = scrollUp;
    scrollDown.rect.y = modernUI ? UiTheme::ScrollDownY : (252+DisplayItemCount*18);

    //Badge cluster (top-right button hints): A/B always shown, right-aligned;
    //X shown to their left, per-row, when the current command has a delete action.
    struct Badge { Texture letter; Texture label; UiTheme::BadgeColor rim; UiTheme::BadgeColor fill; };
    Badge badgeA{ Texture("A", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor), Texture(Translate("HINT_SELECT"), 16, renderer), UiTheme::BadgeADark, UiTheme::BadgeA };
    Badge badgeB{ Texture("B", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor), Texture(Translate("HINT_BACK"), 16, renderer), UiTheme::BadgeBDark, UiTheme::BadgeB };
    Badge badgeX{ Texture("X", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor), Texture(Translate("HINT_DELETE"), 16, renderer), UiTheme::BadgeXDark, UiTheme::BadgeX };
    auto DrawBadge = [&](Badge & badge, int rightEdgeX) -> int
    {
        int groupW = UiTheme::BadgeOuterSize + UiTheme::BadgeLabelGap + badge.label.rect.w;
        int x = rightEdgeX - groupW;
        int y = UiTheme::BadgeBandY;
        badgeOuter.rect = { x, y, UiTheme::BadgeOuterSize, UiTheme::BadgeOuterSize };
        SDL_SetTextureColorMod(badgeOuter.texture.get(), badge.rim.r, badge.rim.g, badge.rim.b);
        badgeOuter.Draw(renderer);
        int innerOffset = (UiTheme::BadgeOuterSize - UiTheme::BadgeInnerSize) / 2;
        badgeInner.rect = { x+innerOffset, y+innerOffset, UiTheme::BadgeInnerSize, UiTheme::BadgeInnerSize };
        SDL_SetTextureColorMod(badgeInner.texture.get(), badge.fill.r, badge.fill.g, badge.fill.b);
        badgeInner.Draw(renderer);
        badge.letter.rect.x = x + (UiTheme::BadgeOuterSize - badge.letter.rect.w)/2;
        badge.letter.rect.y = y + (UiTheme::BadgeOuterSize - badge.letter.rect.h)/2;
        badge.letter.Draw(renderer);
        badge.label.rect.x = x + UiTheme::BadgeOuterSize + UiTheme::BadgeLabelGap;
        badge.label.rect.y = y + (UiTheme::BadgeOuterSize - badge.label.rect.h)/2;
        badge.label.Draw(renderer);
        return x - UiTheme::BadgeGroupGap;
    };

    //Create Command Texture
    const int ChildIndent = 4*16;
    const int rowTextBaseX = modernUI ? UiTheme::RowTextX : 50;
    for(Command & c : commands)
        c.texture = Texture(Translate(c.name), 16, renderer, rowTextBaseX + (c.child ? ChildIndent : 0), 0);

    int topListItemNumber = 1;
    std::shared_ptr<Texture> PreviewImage;
    auto SetCurrentCommand = [&] (int newCommandId)
    {
        currentCommandId = newCommandId;
        const Command & currentCommand = commands[currentCommandId];

        bool updateCommandYPos = false;
        if(currentCommandId < topListItemNumber)
        {
            topListItemNumber = currentCommandId;
            updateCommandYPos = true;
        }
        else if(currentCommandId >= topListItemNumber+DisplayItemCount)
        {
            topListItemNumber = currentCommandId-DisplayItemCount+1;
            updateCommandYPos = true;
        }
        if(updateCommandYPos)
        {
            int y = UiTheme::RowFirstY;
            for(int i = 0, count = std::min(DisplayItemCount,static_cast<int>(commands.size())); i < count; ++i)
            {
                commands[i+topListItemNumber].texture.rect.y = y;
                y += 18;
            }
        }

        highlightRect.y = currentCommand.texture.rect.y - 2;
        highlightRect.w = UiTheme::HighlightW;
        pointerRect.y = currentCommand.texture.rect.y;
        if(currentCommand.previewImage.size())
        {
            //Don't let the highlight bar run underneath a row's preview image.
            //Fixed width, not derived from this row's own previewImageX, so the
            //bar doesn't resize row-to-row depending on where each image sits.
            highlightRect.w = UiTheme::HighlightWWithPreview;
            PreviewImage = std::make_shared<Texture>(currentCommand.previewImage, renderer, currentCommand.previewImageX, currentCommand.previewImageY);
            double wAspectRatio = (double)currentCommand.previewImageHeight / (double)PreviewImage->rect.h * (double)PreviewImage->rect.w;
            double hAspectRatio = (double)currentCommand.previewImageWidth / (double)PreviewImage->rect.w * (double)PreviewImage->rect.h;
            if (currentCommand.previewImageWidth > 0)
            {
                PreviewImage->rect.w = currentCommand.previewImageWidth;
                if (currentCommand.previewImageHeight <= 0)
                {
                    PreviewImage->rect.h = hAspectRatio;
                }
            }
            if (currentCommand.previewImageHeight > 0)
            {
                PreviewImage->rect.h = currentCommand.previewImageHeight;
                if (currentCommand.previewImageWidth <= 0)
                {
                    PreviewImage->rect.w = wAspectRatio;
                }
            }
        }
        else
            PreviewImage.reset();
    };
    SetCurrentCommand(0);

    for(;;)
    {
        //Clear Buffer and Update Input State
        sdl_context.StartFrame();
        controller.Update();

        SDL_Event e;
        //Poll for quit event (CTRL+C)/Close
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return 0;
            }
        }

        if(sdl_context.powerwatch->buttonPress())
        {
            break;
        }
        if(controller.GetButtonStatus(A) || controller.GetButtonStatus(START))
        {
            if(commands[currentCommandId].runInternal)
            {
                commands[currentCommandId].RunCommand(sdl_context, &controller, menuL, menuU, bgR, bgG, bgB);
                if(commands[currentCommandId].isToggle)
                    commands[currentCommandId].UpdateState();
            }
            else
            {
                system(commands[currentCommandId].command.c_str());
                _exitManager.runExitCommand = commands[currentCommandId].restartUI;
                break;
            }
        }
        else if(controller.GetButtonStatus(UP))
        {
            int newCommandId = currentCommandId;
            do { newCommandId = (newCommandId-1+commands.size())%commands.size(); }
            while(commands[newCommandId].command.size() == 0);
            SetCurrentCommand(newCommandId);
        }
        else if(controller.GetButtonStatus(DOWN))
        {
            int newCommandId = currentCommandId;
            do { newCommandId = (newCommandId+1)%commands.size(); }
            while(commands[newCommandId].command.size() == 0);
            SetCurrentCommand(newCommandId);
        }
        else if(controller.GetButtonStatus(B))
            SetCurrentCommand(commands.size()-1);
        else if(controller.GetButtonStatus(X) && !commands[currentCommandId].deleteCommand.empty())
        {
            const std::string & confirmKey = commands[currentCommandId].deleteConfirmKey;
            Texture confirmTitle(Translate(confirmKey.empty() ? "DELETE_CONFIRM_GENERIC" : confirmKey), 24, renderer, 640, 320, true);
            Texture confirmHint(Translate("DELETE_CONFIRM_HINT"), 16, renderer, 640, 360, true);
            bool confirmed = false;
            for(;;)
            {
                controller.Update();
                if(controller.GetButtonStatus(B))
                    break;
                if(controller.GetButtonStatus(A) || controller.GetButtonStatus(START))
                {
                    confirmed = true;
                    break;
                }
                sdl_context.StartFrame();
                if(modernUI)
                    frame.Draw(renderer, UiTheme::FrameRect);
                else
                {
                    banner.Draw(renderer);
                    menuU.Draw(renderer);
                    menuL.Draw(renderer);
                }
                confirmTitle.Draw(renderer);
                confirmHint.Draw(renderer);
                sdl_context.EndFrame();
            }
            if(confirmed)
            {
                system(commands[currentCommandId].deleteCommand.c_str());
                _exitManager.runExitCommand = false;
                break;
            }
        }

        //Draw all textures
        if(modernUI)
        {
            frame.Draw(renderer, UiTheme::FrameRect);
            gearIcon.Draw(renderer);
            appTitleText.Draw(renderer);
            appVersionText.Draw(renderer);
            SDL_Rect dividerRect{ UiTheme::HeaderDividerX, UiTheme::HeaderDividerY, UiTheme::HeaderDividerW, UiTheme::HeaderDividerH };
            SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderFillRect(renderer, &dividerRect);
            SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 0xFF);
        }
        else
        {
            banner.Draw(renderer);
            menuU.Draw(renderer);
            menuL.Draw(renderer);
        }
        titleText.Draw(renderer);
        if(modernUI)
            highlight.Draw(renderer, highlightRect, UiTheme::HighlightR, UiTheme::HighlightG, UiTheme::HighlightB);
        for(int i = 0, count = std::min(DisplayItemCount,static_cast<int>(commands.size())); i < count; ++i)
        {
            Command & rowCommand = commands[i+topListItemNumber];
            rowCommand.texture.Draw(renderer);
            if(modernUI && rowCommand.isToggle)
            {
                Texture & rowSwitch = rowCommand.stateOn ? switchOn : switchOff;
                rowSwitch.rect.x = UiTheme::SwitchRightX - UiTheme::SwitchW;
                rowSwitch.rect.y = rowCommand.texture.rect.y;
                rowSwitch.Draw(renderer);
            }
        }
        if(PreviewImage.get())
            PreviewImage->Draw(renderer);
        CompComText.Draw(renderer);

        if(modernUI)
        {
            //Badge cluster: A rightmost, B to its left, both always shown; X
            //further left, shown only when the current row has a delete action
            //(keeps A/B from jittering as X appears/disappears while scrolling).
            int rightEdge = UiTheme::BadgeClusterRightX;
            rightEdge = DrawBadge(badgeA, rightEdge);
            rightEdge = DrawBadge(badgeB, rightEdge);
            if(!commands[currentCommandId].deleteCommand.empty())
                DrawBadge(badgeX, rightEdge);
        }
        else
        {
            pointerText.Draw(renderer);
            if(commands[currentCommandId].deleteCommand.size())
                deleteHint.Draw(renderer);
        }

        // Display Scroll Arrows when needed
        if(topListItemNumber != 0)
            scrollUp.Draw(renderer);
        if((topListItemNumber + DisplayItemCount) < commands.size())
            scrollDown.Draw(renderer, SDL_FLIP_VERTICAL);

        //Render Framebuffer and Wait for Next Frame
        sdl_context.EndFrame();
    }

    return 0;
}
