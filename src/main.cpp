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
#include "framework/draw_helpers.h"
#include "framework/utf8.h"
#include "command.h"
#include "localization.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <cmath>
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

std::string CollapseSlashes(const std::string & path)
{
    std::string result;
    for(char ch : path)
        if(ch != '/' || result.empty() || result.back() != '/')
            result += ch;
    return result;
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
    optionsLocation = CollapseSlashes(optionsLocation.substr(0, optionsLocation.find_last_of('/')+1));
    _exitManager.exitCommand = "/bin/sh " + optionsLocation + "/scripts/ResumeUI.sh";
    std::string commandLocation(optionsLocation + "commands/");
    std::string scriptLocation(optionsLocation + "scripts/");

    LoadLanguageFromConfig(optionsLocation);
    SetTTFFontPath(optionsLocation);
    UiTheme::LoadThemeConfig(optionsLocation);

    std::string titleString(Translate("OPTIONS_TITLE"));
    std::string titleKey = "OPTIONS_TITLE";
    const char * backStackEnv = getenv("OM_BACK_STACK");
    std::string backStack = backStackEnv ? backStackEnv : "";

    bool usbReady = false;
    if(auto dir = opendir("/media/hakchi/"))
    {
        closedir(dir);
        usbReady = true;
    }

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
            titleKey = argv[i+1];
            titleString = Translate(argv[i+1]);
            ++i;
        }
    }
    commandLocation = CollapseSlashes(commandLocation);
    scriptLocation = CollapseSlashes(scriptLocation);

    std::string myEntry = commandLocation + "," + scriptLocation + "," + titleKey;
    size_t lastSep = backStack.find_last_of(';');
    std::string lastEntry = backStack.empty() ? "" : (lastSep == std::string::npos ? backStack : backStack.substr(lastSep + 1));
    if(lastEntry == myEntry)
        backStack = (lastSep == std::string::npos) ? "" : backStack.substr(0, lastSep);
    setenv("OM_BACK_STACK", (backStack.empty() ? myEntry : backStack + ";" + myEntry).c_str(), 1);

    auto ExpandTemplates = [&](std::string & cmd)
    {
        sReplace(cmd, "%options_path%", optionsLocation);
        sReplace(cmd, "%script_dir%", scriptLocation);
    };

    std::vector<Command> commands;
    std::ifstream in;
    if(auto dir = opendir(commandLocation.c_str()))
    {
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
                        if(!c.enableIfCommand.empty())
                        {
                            ExpandTemplates(c.enableIfCommand);
                            if(system(("source /etc/preinit; script_init; " + c.enableIfCommand).c_str()) != 0)
                                continue;
                        }
                        ExpandTemplates(c.command);
                        ExpandTemplates(c.deleteCommand);
                        if(c.isToggle)
                        {
                            ExpandTemplates(c.stateCommand);
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

    if(commands.empty())
    {
        std::cerr << "No usable commands in " << commandLocation << "\n";
        exit(1);
    }

    std::string backCommand;
    if(!backStack.empty())
    {
        std::vector<std::string> entries;
        for(size_t start = 0; start < backStack.size();)
        {
            size_t sep = backStack.find(';', start);
            if(sep == std::string::npos) sep = backStack.size();
            entries.push_back(backStack.substr(start, sep-start));
            start = sep+1;
        }
        std::string lastEntry = entries.back();
        entries.pop_back();
        std::string remainingStack;
        for(size_t i = 0; i < entries.size(); ++i)
        {
            if(i) remainingStack += ";";
            remainingStack += entries[i];
        }

        size_t c1 = lastEntry.find(','), c2 = lastEntry.find(',', c1+1);
        std::string backPath = lastEntry.substr(0, c1);
        std::string backScriptPath = lastEntry.substr(c1+1, c2-c1-1);
        std::string backTitleKey = lastEntry.substr(c2+1);

        backCommand = "usleep 50000 && OM_BACK_STACK=\"" + remainingStack + "\" " + optionsLocation + "options --commandPath " + backPath
            + (backScriptPath.empty() ? "" : " --scriptPath " + backScriptPath)
            + " --title \"" + backTitleKey + "\" &";
    }

    int currentCommandId = 0;

    SDL_Context sdl_context(std::chrono::milliseconds(33), false);
    auto renderer = sdl_context.renderer;
    Controller controller(1);

    system("touch /tmp/options.flag");

    const Color bg = UiTheme::Bg;
    SetDrawColor(renderer, bg);

    Texture gearIcon(optionsLocation + UiTheme::AssetGear, renderer, UiTheme::GearX, UiTheme::GearY);
    Texture switchOn(optionsLocation + UiTheme::AssetSwitchOn, renderer);
    Texture switchOff(optionsLocation + UiTheme::AssetSwitchOff, renderer);
    Texture badgeOuter(optionsLocation + UiTheme::AssetBadgeOuter, renderer);
    Texture badgeInner(optionsLocation + UiTheme::AssetBadgeInner, renderer);

    Texture appTitleText("OptionsMenu", UiTheme::TitleFontSize, renderer, UiTheme::TitleX, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true);
    appTitleText.rect.y -= appTitleText.rect.h / 2;
    Texture appVersionText(MOD_VERSION, UiTheme::VersionFontSize, renderer, appTitleText.rect.x + appTitleText.rect.w + UiTheme::VersionGap, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true);
    appVersionText.rect.y -= appVersionText.rect.h / 2;
    Texture titleText(titleString, UiTheme::SectionTitleFontSize, renderer, UiTheme::SectionTitleX, UiTheme::SectionTitleY, false, ToAbgr(UiTheme::Text), true);
    SDL_Rect selectedRowRect{ UiTheme::ListX, UiTheme::RowFirstY - 2, UiTheme::ListContentRightX - UiTheme::ListX, 0 };
    Texture CompComText("created by CompCom - Modern UI by DefKorns", 16, renderer, UiTheme::CreditX, UiTheme::CreditY, false, ToAbgr(UiTheme::Text), true);
    Texture scrollUp(optionsLocation + UiTheme::AssetChevronUp, renderer, UiTheme::ScrollX, UiTheme::ScrollUpY);
    SetColorMod(scrollUp.texture.get(), UiTheme::ScrollArrow);
    scrollUp.rect.x -= scrollUp.rect.w / 2;
    Texture scrollDown = scrollUp;
    scrollDown.rect.y = UiTheme::ScrollDownY;

    struct Badge { Texture letter; Texture label; Color rim; Color fill; };
    auto MakeBadge = [&](const char * letter, const char * hintKey, Color rim, Color fill) -> Badge
    {
        return { Texture(letter, 16, renderer, 0, 0, false, ToAbgr(UiTheme::BadgeLetter), true), Texture(Translate(hintKey), 16, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true), rim, fill };
    };
    Badge badgeA = MakeBadge("A", "HINT_SELECT", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge badgeB = MakeBadge("B", "HINT_BACK", UiTheme::BadgeBDark, UiTheme::BadgeB);
    Badge badgeBExit = MakeBadge("B", "EXIT", UiTheme::BadgeBDark, UiTheme::BadgeB);
    Badge badgeHold = MakeBadge("B", "HINT_DELETE", UiTheme::BadgeXDark, UiTheme::BadgeX);
    auto DrawBadge = [&](Badge & badge, int rightEdgeX) -> int
    {
        int groupW = UiTheme::BadgeOuterSize + UiTheme::BadgeLabelGap + badge.label.rect.w;
        int x = rightEdgeX - groupW;
        int y = UiTheme::BadgeBandY;
        badgeOuter.rect = { x, y, UiTheme::BadgeOuterSize, UiTheme::BadgeOuterSize };
        SetColorMod(badgeOuter.texture.get(), badge.rim);
        badgeOuter.Draw(renderer);
        int innerOffset = (UiTheme::BadgeOuterSize - UiTheme::BadgeInnerSize) / 2;
        badgeInner.rect = { x+innerOffset, y+innerOffset, UiTheme::BadgeInnerSize, UiTheme::BadgeInnerSize };
        SetColorMod(badgeInner.texture.get(), badge.fill);
        badgeInner.Draw(renderer);
        badge.letter.rect.x = x + (UiTheme::BadgeOuterSize - badge.letter.rect.w)/2 + 1;
        badge.letter.rect.y = y + (UiTheme::BadgeOuterSize - badge.letter.rect.h)/2 - 1;
        badge.letter.Draw(renderer);
        badge.label.rect.x = x + UiTheme::BadgeOuterSize + UiTheme::BadgeLabelGap;
        badge.label.rect.y = y + (UiTheme::BadgeOuterSize - badge.label.rect.h)/2;
        badge.label.Draw(renderer);
        return x - UiTheme::BadgeGroupGap;
    };

    const int ChildIndent = 4*16;
    const int RowGlyphSize = 16;
    const int RowTextGapPx = 16;
    Texture chevronIcon(optionsLocation + UiTheme::AssetChevronRight, renderer);
    SetColorMod(chevronIcon.texture.get(), UiTheme::Accent);
    for(Command & c : commands)
    {
        if(c.hasSubmenu)
        {
            size_t pos = c.command.find("--commandPath");
            size_t pathStart = c.command.find_first_not_of(' ', pos + std::string("--commandPath").size());
            size_t pathEnd = c.command.find(' ', pathStart);
            std::string targetPath = CollapseSlashes(c.command.substr(pathStart, pathEnd - pathStart));
            if(targetPath == commandLocation)
                c.hasSubmenu = false;
        }

        int textX = UiTheme::RowTextX + (c.child ? ChildIndent : 0);
        std::string label = Translate(c.name);

        int maxRight = UiTheme::RowControlRightX - RowTextGapPx;
        bool rowUsesTTF = CanRenderWithTTF(label, RowGlyphSize);
        int available = std::max(0, maxRight - textX);
        if(rowUsesTTF)
        {
            if(MeasureTTFWidth(label, RowGlyphSize) > available)
            {
                int n = Utf8Length(label);
                while(n > 0 && MeasureTTFWidth(TruncateUtf8(label, n) + "...", RowGlyphSize) > available)
                    --n;
                label = TruncateUtf8(label, n) + "...";
            }
        }
        else
        {
            int maxChars = available / RowGlyphSize;
            if(TruncateUtf8(label, maxChars).size() != label.size())
                label = TruncateUtf8(label, std::max(0, maxChars - 3)) + "...";
        }

        c.texture = Texture(label, RowGlyphSize, renderer, textX, 0, false, ToAbgr(UiTheme::Text), true);
    }

    const int modernRowPitch = std::max(UiTheme::RowPitch, GetTTFLineHeight(RowGlyphSize));
    const int commandCount = static_cast<int>(commands.size());
    const int DisplayItemCount = std::max(1, (UiTheme::FooterDividerY - UiTheme::ListBottomMargin - UiTheme::RowFirstY) / modernRowPitch);

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
            for(int i = 0, count = std::min(DisplayItemCount, commandCount-topListItemNumber); i < count; ++i)
            {
                Texture & rowTexture = commands[i+topListItemNumber].texture;
                rowTexture.rect.y = y + (modernRowPitch - rowTexture.rect.h) / 2 + UiTheme::RowTextYNudge;
                y += modernRowPitch;
            }
        }

        selectedRowRect.y = currentCommand.texture.rect.y - 2;
        selectedRowRect.h = currentCommand.texture.rect.h + 4;

        if(currentCommand.previewImage.size())
        {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
            PreviewImage = std::make_shared<Texture>(currentCommand.previewImage, renderer, 0, 0);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
            if(PreviewImage->rect.w > 0 && PreviewImage->rect.h > 0)
            {
                const int maxW = UiTheme::DetailW - 2*UiTheme::ContentPadding;
                const int maxH = UiTheme::PreviewBoxH - 2*UiTheme::ContentPadding;
                double scale = std::min((double)maxW / PreviewImage->rect.w, (double)maxH / PreviewImage->rect.h);
                PreviewImage->rect.w = static_cast<int>(PreviewImage->rect.w * scale);
                PreviewImage->rect.h = static_cast<int>(PreviewImage->rect.h * scale);
            }
        }
        else
            PreviewImage.reset();
    };
    SetCurrentCommand(0);

    auto MoveSelection = [&](int step)
    {
        int newCommandId = currentCommandId;
        for(size_t tries = 0; tries < commands.size(); ++tries)
        {
            newCommandId = (newCommandId + step + commands.size()) % commands.size();
            if(commands[newCommandId].command.size() != 0)
                break;
        }
        SetCurrentCommand(newCommandId);
    };

    auto DrawRow = [&](Command & rowCommand, bool isLastOverall)
    {
        bool selected = &rowCommand == &commands[currentCommandId];
        rowCommand.texture.Draw(renderer);
        if(rowCommand.isToggle)
        {
            Texture & rowSwitch = rowCommand.stateOn ? switchOn : switchOff;
            rowSwitch.rect.x = UiTheme::SwitchRightX - UiTheme::SwitchW;
            rowSwitch.rect.y = rowCommand.texture.rect.y + (rowCommand.texture.rect.h - UiTheme::SwitchH) / 2;
            rowSwitch.Draw(renderer);
        }
        else if(rowCommand.hasSubmenu)
        {
            chevronIcon.rect.x = UiTheme::RowControlRightX - chevronIcon.rect.w;
            chevronIcon.rect.y = rowCommand.texture.rect.y + (rowCommand.texture.rect.h - chevronIcon.rect.h) / 2;
            chevronIcon.Draw(renderer);
        }
        if(!selected && !rowCommand.command.empty() && !isLastOverall)
            DrawHLine(renderer, UiTheme::ListX, UiTheme::ListContentRightX, rowCommand.texture.rect.y + rowCommand.texture.rect.h + 3, UiTheme::Border);
    };

    auto DrawChrome = [&]()
    {
        DrawStrokeRect(renderer, UiTheme::OuterRect, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BorderRadius);
        gearIcon.Draw(renderer);
        appTitleText.Draw(renderer);
        appVersionText.Draw(renderer);
        DrawHLine(renderer, UiTheme::HeaderDividerX, UiTheme::HeaderDividerX + UiTheme::HeaderDividerW, UiTheme::HeaderDividerY, UiTheme::Border, UiTheme::BorderWidth);
        DrawHLine(renderer, UiTheme::OuterRect.x, UiTheme::OuterRect.x + UiTheme::OuterRect.w, UiTheme::FooterDividerY, UiTheme::Border, UiTheme::BorderWidth);
        int accentBarY = titleText.rect.y + (titleText.rect.h - UiTheme::SectionAccentBarH) / 2;
        DrawFillRect(renderer, { UiTheme::SectionTitleX - UiTheme::SectionAccentBarW - 14, accentBarY, UiTheme::SectionAccentBarW, UiTheme::SectionAccentBarH }, UiTheme::Accent);

        DrawRoundedFillRect(renderer, selectedRowRect, UiTheme::SelectedRowBg, UiTheme::BoxRadius);
        DrawStrokeRect(renderer, selectedRowRect, UiTheme::Accent, 2, UiTheme::BoxRadius);

        if(PreviewImage.get())
        {
            SDL_Rect previewBox{ UiTheme::DetailX, UiTheme::PreviewBoxY, UiTheme::DetailW, UiTheme::PreviewBoxH };
            DrawStrokeRect(renderer, previewBox, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BoxRadius);
            PreviewImage->rect.x = previewBox.x + (previewBox.w - PreviewImage->rect.w) / 2;
            PreviewImage->rect.y = previewBox.y + (previewBox.h - PreviewImage->rect.h) / 2;
            PreviewImage->Draw(renderer);
        }

        int rightEdge = UiTheme::BadgeClusterRightX;
        rightEdge = DrawBadge(badgeA, rightEdge);
        rightEdge = DrawBadge(backStack.empty() ? badgeBExit : badgeB, rightEdge);
        if(!commands[currentCommandId].deleteCommand.empty())
            rightEdge = DrawBadge(badgeHold, rightEdge);
        int dividerX = rightEdge - UiTheme::BadgeDividerGapFromCluster;
        DrawVLine(renderer, dividerX, UiTheme::FooterY + 10, UiTheme::FooterY + UiTheme::FooterH - 10, UiTheme::Border, 2);
    };

    auto ConfirmDelete = [&]() -> bool
    {
        const std::string & confirmKey = commands[currentCommandId].deleteConfirmKey;
        Texture confirmTitle(Translate(confirmKey.empty() ? "DELETE_CONFIRM_GENERIC" : confirmKey), 24, renderer, 640, 320, true, ToAbgr(UiTheme::Text), true);
        Texture confirmHint(Translate("DELETE_CONFIRM_HINT"), 16, renderer, 640, 360, true, ToAbgr(UiTheme::Text), true);
        controller.GetButtonStatus(B);
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
            DrawFillRect(renderer, UiTheme::FrameRect, UiTheme::Bg);
            DrawStrokeRect(renderer, UiTheme::FrameRect, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BorderRadius);
            confirmTitle.Draw(renderer);
            confirmHint.Draw(renderer);
            SetDrawColor(renderer, bg);
            sdl_context.EndFrame();
        }
        if(!confirmed)
            return false;
        system(commands[currentCommandId].deleteCommand.c_str());
        _exitManager.runExitCommand = false;
        return true;
    };

    const unsigned int bHoldThresholdMs = 1000;
    bool bWasHeld = false, bHoldFired = false;

    for(;;)
    {
        sdl_context.StartFrame();
        controller.Update();

        SDL_Event e;
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
                commands[currentCommandId].RunCommand(sdl_context, &controller, { gearIcon, appTitleText, appVersionText, CompComText }, bg);
                if(commands[currentCommandId].isToggle)
                    for(Command & c : commands)
                        if(c.isToggle)
                            c.UpdateState();
            }
            else
            {
                system(commands[currentCommandId].command.c_str());
                _exitManager.runExitCommand = commands[currentCommandId].restartUI;
                break;
            }
        }
        else if(controller.HeldRepeat(UP))
            MoveSelection(-1);
        else if(controller.HeldRepeat(DOWN))
            MoveSelection(1);

        bool bHeldNow = controller.PeekButtonStatus(B);
        if(bHeldNow)
        {
            if(!bHoldFired && controller.HeldMillis(B) >= bHoldThresholdMs)
            {
                bHoldFired = true;
                if(!commands[currentCommandId].deleteCommand.empty() && ConfirmDelete())
                    break;
            }
        }
        else
        {
            if(bWasHeld && !bHoldFired)
            {
                if(!backCommand.empty())
                    system(backCommand.c_str());
                _exitManager.runExitCommand = backCommand.empty();
                break;
            }
            bHoldFired = false;
        }
        bWasHeld = bHeldNow;

        DrawChrome();
        titleText.Draw(renderer);

        for(int i = 0, count = std::min(DisplayItemCount, commandCount-topListItemNumber); i < count; ++i)
            DrawRow(commands[i+topListItemNumber], (i+topListItemNumber) == commandCount-1);

        CompComText.Draw(renderer);

        if(topListItemNumber != 0)
            scrollUp.Draw(renderer);
        if((topListItemNumber + DisplayItemCount) < commandCount)
            scrollDown.Draw(renderer, SDL_FLIP_VERTICAL);

        SetDrawColor(renderer, bg);

        sdl_context.EndFrame();
    }

    return 0;
}
