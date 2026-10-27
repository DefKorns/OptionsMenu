/**
  * Copyright (C) 2017-2018 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "framework/badge.h"
#include "framework/close_on_exec.h"
#include "framework/dialog.h"
#include "framework/sdl_helper.h"
#include "framework/controller.h"
#include "framework/powerwatch.h"
#include "framework/draw_helpers.h"
#include "framework/list_screen.h"
#include "framework/utf8.h"
#include "application_lifecycle.h"
#include "back_stack.h"
#include "command.h"
#include "download_screen.h"
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

static ApplicationLifecycle lifecycle;

int main(int argc, char * argv[])
{
    std::string optionsLocation(argv[0]);
    optionsLocation = CollapseSlashes(optionsLocation.substr(0, optionsLocation.find_last_of('/')+1));
    lifecycle.SetResumeScript(optionsLocation + "/scripts/ResumeUI.sh");
    lifecycle.InstallSignalHandlers();
    std::string commandLocation(optionsLocation + "commands/");
    std::string scriptLocation(optionsLocation + "scripts/");

    LoadLanguageFromConfig(optionsLocation);
    SetTTFFontPath(optionsLocation);
    UiTheme::LoadThemeConfig(optionsLocation);

    std::string titleKey = "OPTIONS_TITLE";
    std::string selectedMarker;
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
        if(strcmp(argv[i], "--commandPath") == 0 && i + 1 < argc)
        {
            commandLocation = argv[i+1];
            ++i;
        }
        else if(strcmp(argv[i], "--scriptPath") == 0 && i + 1 < argc)
        {
            scriptLocation = argv[i+1];
            ++i;
        }
        else if(strcmp(argv[i], "--title") == 0 && i + 1 < argc)
        {
            titleKey = argv[i+1];
            ++i;
        }
        else if(strcmp(argv[i], "--selectedMarker") == 0 && i + 1 < argc)
        {
            selectedMarker = argv[i+1];
            ++i;
        }
    }
    commandLocation = CollapseSlashes(commandLocation);
    scriptLocation = CollapseSlashes(scriptLocation);

    const std::string myEntry = back_stack::MakeEntry(commandLocation, scriptLocation, titleKey, selectedMarker);
    backStack = back_stack::PopIfLast(backStack, myEntry);
    setenv("OM_BACK_STACK", back_stack::Push(backStack, myEntry).c_str(), 1);

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
                if(!(commands.empty() && !c.Selectable()))
                {
                    if(!c.usbOnly || (c.usbOnly && usbReady))
                    {
                        if(!c.enableIfCommand.empty())
                        {
                            ExpandTemplates(c.enableIfCommand);
                            MarkFdsCloseOnExec();
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
                        if(c.isChoice)
                        {
                            for(std::string * choiceCommand : { &c.choiceListCommand, &c.choiceGetCommand, &c.choiceSetCommand, &c.choiceResolvedCommand })
                                ExpandTemplates(*choiceCommand);
                            c.UpdateChoice();
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
        lifecycle.Decide(ExitAction::Resume);
        exit(1);
    }

    while(!commands.empty() && !commands.back().Selectable())
        commands.pop_back();
    if(commands.empty())
    {
        std::cerr << "No usable commands in " << commandLocation << "\n";
        lifecycle.Decide(ExitAction::Resume);
        exit(1);
    }

    const std::string backCommand = back_stack::BackCommand(backStack, optionsLocation);

    int currentCommandId = 0;

    SDL_Context sdl_context(std::chrono::milliseconds(33), false);
    auto renderer = sdl_context.renderer;
    Controller controller(1);

    MarkFdsCloseOnExec();
    system("touch /tmp/options.flag");

    const Color bg = UiTheme::Bg;
    SetDrawColor(renderer, bg);

    Texture switchOn(optionsLocation + UiTheme::AssetSwitchOn, renderer);
    Texture switchOff(optionsLocation + UiTheme::AssetSwitchOff, renderer);

    ScreenChrome chrome(optionsLocation, renderer, "OptionsMenu", MOD_VERSION, "created by CompCom - Modern UI by DefKorns");
    SDL_Rect selectedRowRect{ UiTheme::ListX, UiTheme::RowFirstY - 2, UiTheme::ListContentRightX - UiTheme::ListX, 0 };
    ScrollArrows scrollArrows(optionsLocation, renderer);

    BadgePainter badges(optionsLocation, renderer);
    auto MakeBadge = [&](const char * letter, const char * hintKey, Color rim, Color fill) -> Badge
    {
        return badges.Make(letter, Translate(hintKey), rim, fill);
    };
    Badge badgeA, badgeB, badgeBExit, badgeHold, badgeAuto;
    auto DrawBadge = [&](Badge & badge, int rightEdgeX) -> int
    {
        return badges.Draw(badge, rightEdgeX, UiTheme::BadgeBandY);
    };

    const int ChildIndent = 4*16;
    const int RowGlyphSize = 16;
    const int RowTextGapPx = 16;
    Texture chevronIcon(optionsLocation + UiTheme::AssetChevronRight, renderer);
    SetColorMod(chevronIcon.texture.get(), UiTheme::Accent);
    ValuePicker valuePicker(optionsLocation, renderer);
    for(Command & c : commands)
    {
        if(c.hasSubmenu)
        {
            const std::string flag = "--commandPath";
            const size_t pos = c.command.find(flag);
            const size_t pathStart = pos == std::string::npos ? pos : c.command.find_first_not_of(' ', pos + flag.size());
            if(pathStart != std::string::npos)
            {
                const size_t pathEnd = c.command.find(' ', pathStart);
                const std::string targetPath = CollapseSlashes(c.command.substr(pathStart, pathEnd == std::string::npos ? pathEnd : pathEnd - pathStart));
                if(targetPath == commandLocation)
                    c.hasSubmenu = false;
            }
        }
    }

    // everything translated, again after the language changes
    auto BuildTexts = [&]()
    {
        chrome.SetSectionTitle(Translate(titleKey));
        badgeA = MakeBadge("A", "HINT_SELECT", UiTheme::BadgeADark, UiTheme::BadgeA);
        badgeB = MakeBadge("B", "HINT_BACK", UiTheme::BadgeBDark, UiTheme::BadgeB);
        badgeBExit = MakeBadge("B", "EXIT", UiTheme::BadgeBDark, UiTheme::BadgeB);
        badgeHold = MakeBadge("B", "HINT_DELETE", UiTheme::BadgeXDark, UiTheme::BadgeX);
        valuePicker.SetHintLabel(Translate("HINT_CHANGE"));

        for(Command & c : commands)
        {
        int textX = UiTheme::RowTextX + (c.child ? ChildIndent : 0);
        std::string label = Translate(c.name);

        int maxRight = UiTheme::RowControlRightX - RowTextGapPx;
        if(c.isChoice)
        {
            const ChoiceOption * shown = c.ShownChoice();
            std::string value = shown ? Translate(shown->label) : c.choiceResolved;
            if(c.choiceValue.empty() && !c.choiceAutoKey.empty())
                value = value.empty() ? Translate(c.choiceAutoKey) : Translate(c.choiceAutoKey) + " · " + value;
            c.valueTexture = Texture(value, RowGlyphSize, renderer, 0, 0, false, ToAbgr(UiTheme::TextDim), true);
            c.previewImage = shown ? shown->preview : "";
            maxRight -= valuePicker.Width(c.valueTexture);
        }
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
        if(!c.rowIcon.empty())
            c.rowIconTexture = Texture(c.rowIcon, renderer, 0, 0);
        }
    };
    BuildTexts();

    const int modernRowPitch = std::max(UiTheme::RowPitch, GetTTFLineHeight(RowGlyphSize));
    const int commandCount = static_cast<int>(commands.size());
    const int DisplayItemCount = std::max(1, (UiTheme::FooterDividerY - UiTheme::ListBottomMargin - UiTheme::RowFirstY) / modernRowPitch);

    int topListItemNumber = 1;
    std::shared_ptr<Texture> PreviewImage;
    auto LayoutRows = [&]()
    {
        int y = UiTheme::RowFirstY;
        for(int i = 0, count = std::min(DisplayItemCount, commandCount-topListItemNumber); i < count; ++i)
        {
            Texture & rowTexture = commands[i+topListItemNumber].texture;
            rowTexture.rect.y = y + (modernRowPitch - rowTexture.rect.h) / 2 + UiTheme::RowTextYNudge;
            y += modernRowPitch;
        }
    };
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
            LayoutRows();

        if(currentCommand.isChoice && !currentCommand.choiceAutoKey.empty())
            badgeAuto = badges.MakePill("SELECT", Translate(currentCommand.choiceAutoKey));

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
    // with --selectedMarker, the row whose command ends in the marker's word (or <word>.sh) is the active one
    std::vector<bool> isActive(commands.size(), false);
    auto RefreshActive = [&]() -> int
    {
        std::fill(isActive.begin(), isActive.end(), false);
        std::string marker;
        if(!selectedMarker.empty())
        {
            std::ifstream markerIn(selectedMarker);
            std::getline(markerIn, marker);
        }
        while(!marker.empty() && (marker.back() == '\r' || marker.back() == ' '))
            marker.pop_back();
        if(marker.empty())
            return -1;
        int first = -1;
        for(size_t i = 0; i < commands.size(); ++i)
        {
            const std::string & cmd = commands[i].command;
            const size_t end = cmd.find_last_not_of(" \t&");
            if(end == std::string::npos)
                continue;
            const size_t sep = cmd.find_last_of(" \t/", end);
            const std::string word = cmd.substr(sep == std::string::npos ? 0 : sep + 1, end - (sep == std::string::npos ? 0 : sep + 1) + 1);
            if(word == marker || word == marker + ".sh")
            {
                isActive[i] = true;
                if(first < 0)
                    first = static_cast<int>(i);
            }
        }
        return first;
    };
    // the first call lays the rows out
    SetCurrentCommand(0);
    const int activeCommandId = RefreshActive();
    if(activeCommandId > 0)
        SetCurrentCommand(activeCommandId);

    auto MoveSelection = [&](int step)
    {
        int newCommandId = currentCommandId;
        for(size_t tries = 0; tries < commands.size(); ++tries)
        {
            newCommandId = (newCommandId + step + commands.size()) % commands.size();
            if(commands[newCommandId].Selectable())
                break;
        }
        SetCurrentCommand(newCommandId);
    };

    auto DrawRow = [&](Command & rowCommand, bool isLastOverall)
    {
        bool selected = &rowCommand == &commands[currentCommandId];
        const bool active = isActive[static_cast<size_t>(&rowCommand - commands.data())];
        if(active && !selected)
            DrawStrokeRect(renderer, { UiTheme::ListX, rowCommand.texture.rect.y - 2, UiTheme::ListContentRightX - UiTheme::ListX, rowCommand.texture.rect.h + 4 },
                           UiTheme::ActiveBorder, 2, UiTheme::BoxRadius);
        rowCommand.texture.Draw(renderer);
        if(rowCommand.isToggle)
        {
            Texture & rowSwitch = rowCommand.stateOn ? switchOn : switchOff;
            rowSwitch.rect.x = UiTheme::SwitchRightX - UiTheme::SwitchW;
            rowSwitch.rect.y = rowCommand.texture.rect.y + (rowCommand.texture.rect.h - UiTheme::SwitchH) / 2;
            rowSwitch.Draw(renderer);
        }
        else if(rowCommand.rowIconTexture.texture)
        {
            Texture & icon = rowCommand.rowIconTexture;
            icon.rect.x = UiTheme::RowControlRightX - icon.rect.w;
            icon.rect.y = rowCommand.texture.rect.y + (rowCommand.texture.rect.h - icon.rect.h) / 2;
            icon.Draw(renderer);
        }
        else if(rowCommand.isChoice)
            valuePicker.DrawValue(rowCommand.valueTexture, UiTheme::RowControlRightX, rowCommand.texture.rect.y + rowCommand.texture.rect.h / 2, selected);
        else if(rowCommand.hasSubmenu)
        {
            chevronIcon.rect.x = UiTheme::RowControlRightX - chevronIcon.rect.w;
            chevronIcon.rect.y = rowCommand.texture.rect.y + (rowCommand.texture.rect.h - chevronIcon.rect.h) / 2;
            chevronIcon.Draw(renderer);
        }
        if(!selected && !active && rowCommand.Selectable() && !isLastOverall)
            DrawHLine(renderer, UiTheme::ListX, UiTheme::ListContentRightX, rowCommand.texture.rect.y + rowCommand.texture.rect.h + 3, UiTheme::Border);
    };

    auto DrawChrome = [&]()
    {
        chrome.Draw();

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
        const Command & current = commands[currentCommandId];
        if(!current.isChoice)
            rightEdge = DrawBadge(badgeA, rightEdge);
        rightEdge = DrawBadge(backStack.empty() ? badgeBExit : badgeB, rightEdge);
        if(current.isChoice)
        {
            if(!current.choiceAutoKey.empty() && !current.choiceValue.empty())
                rightEdge = DrawBadge(badgeAuto, rightEdge);
            rightEdge = valuePicker.DrawHint(rightEdge, UiTheme::BadgeBandY);
        }
        if(!commands[currentCommandId].deleteCommand.empty() || !commands[currentCommandId].deleteBlockedKey.empty())
            rightEdge = DrawBadge(badgeHold, rightEdge);
        DrawFooterDivider(renderer, rightEdge);
    };

    auto DrawScreen = [&]()
    {
        DrawChrome();

        for(int i = 0, count = std::min(DisplayItemCount, commandCount-topListItemNumber); i < count; ++i)
            DrawRow(commands[i+topListItemNumber], (i+topListItemNumber) == commandCount-1);

        scrollArrows.Draw(topListItemNumber != 0, (topListItemNumber + DisplayItemCount) < commandCount);
    };

    auto Confirm = [&](const std::string & title) -> bool
    {
        ConfirmDialog dialog(renderer, badges, title,
                             MakeBadge("A", "HINT_CONFIRM", UiTheme::BadgeADark, UiTheme::BadgeA),
                             MakeBadge("B", "HINT_CANCEL", UiTheme::BadgeBDark, UiTheme::BadgeB));
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
            DrawScreen();
            dialog.Draw();
            SetDrawColor(renderer, bg);
            sdl_context.EndFrame();
        }
        return confirmed;
    };

    auto ShowBlocked = [&](const std::string & title)
    {
        MessageDialog dialog(renderer, badges, title,
                             MakeBadge("B", "HINT_BACK", UiTheme::BadgeBDark, UiTheme::BadgeB));
        controller.GetButtonStatus(B);
        for(;;)
        {
            controller.Update();
            if(controller.GetButtonStatus(B) || controller.GetButtonStatus(A) || controller.GetButtonStatus(START))
                break;
            sdl_context.StartFrame();
            DrawScreen();
            dialog.Draw();
            SetDrawColor(renderer, bg);
            sdl_context.EndFrame();
        }
    };

    auto ConfirmDelete = [&]() -> bool
    {
        const std::string & confirmKey = commands[currentCommandId].deleteConfirmKey;
        if(!Confirm(Translate(confirmKey.empty() ? "DELETE_CONFIRM_GENERIC" : confirmKey)))
            return false;
        lifecycle.Decide(ExitAction::Leave);
        MarkFdsCloseOnExec();
        system(commands[currentCommandId].deleteCommand.c_str());
        return true;
    };

    // a choice can be the menu's own language, so the strings reload with it
    auto ApplyChoice = [&](const std::string & value)
    {
        commands[currentCommandId].SetChoice(value);
        LoadLanguageFromConfig(optionsLocation);
        BuildTexts();
        LayoutRows();
        SetCurrentCommand(currentCommandId);
    };
    // with CHOICE_AUTO, the empty value (automatic) comes before the first option
    auto CycleChoice = [&](int step)
    {
        const Command & current = commands[currentCommandId];
        const int count = static_cast<int>(current.choices.size());
        const int first = current.choiceAutoKey.empty() ? 0 : -1;
        const int span = count - first;
        if(count == 0)
            return;
        int index = first;
        for(int i = 0; i < count; ++i)
            if(current.choices[i].value == current.choiceValue)
                index = i;
        index = (index - first + step + span) % span + first;
        ApplyChoice(index < 0 ? "" : current.choices[index].value);
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
                lifecycle.Decide(ExitAction::Resume);
                return 0;
            }
        }

        if(sdl_context.powerwatch->buttonPress())
        {
            lifecycle.Decide(ExitAction::Resume);
            break;
        }
        const bool onPicker = commands[currentCommandId].isChoice;
        if(onPicker && controller.HeldRepeat(LEFT))
            CycleChoice(-1);
        else if(onPicker && controller.HeldRepeat(RIGHT))
            CycleChoice(1);
        else if(onPicker && (controller.GetButtonStatus(SELECT) || controller.GetButtonStatus(Y)))
        {
            const Command & current = commands[currentCommandId];
            if(!current.choiceAutoKey.empty() && !current.choiceValue.empty())
                ApplyChoice("");
        }
        else if(onPicker && (controller.GetButtonStatus(A) || controller.GetButtonStatus(START)))
        {
        }
        else if(controller.GetButtonStatus(A) || controller.GetButtonStatus(START))
        {
            const std::string & confirmKey = commands[currentCommandId].confirmKey;
            const bool confirmed = confirmKey.empty() || Confirm(Translate(confirmKey));
            if(confirmed && commands[currentCommandId].runInternal)
            {
                commands[currentCommandId].RunCommand(sdl_context, &controller, chrome, bg);
                if(commands[currentCommandId].isToggle)
                    for(Command & c : commands)
                        if(c.isToggle)
                            c.UpdateState();
                RefreshActive();
            }
            else if(confirmed && commands[currentCommandId].runDownload)
            {
                RunDownloadScreen(commands[currentCommandId].command, "", Translate(commands[currentCommandId].name), "",
                                  sdl_context, controller, chrome);
                RefreshActive();
            }
            else if(confirmed)
            {
                lifecycle.Decide(commands[currentCommandId].restartUI ? ExitAction::Resume : ExitAction::Leave);
                MarkFdsCloseOnExec();
                system(commands[currentCommandId].command.c_str());
                break;
            }
        }
        else if(controller.HeldRepeat(UP))
            MoveSelection(-1);
        else if(controller.HeldRepeat(DOWN))
            MoveSelection(1);

        bool bHeldNow = controller.PeekButtonStatus(B);
        // must run every frame: it only resets its timer when sampled while released
        const unsigned int bHeldMs = controller.HeldMillis(B);
        if(bHeldNow)
        {
            if(!bHoldFired && bHeldMs >= bHoldThresholdMs)
            {
                bHoldFired = true;
                if(!commands[currentCommandId].deleteBlockedKey.empty())
                    ShowBlocked(Translate(commands[currentCommandId].deleteBlockedKey));
                else if(!commands[currentCommandId].deleteCommand.empty() && ConfirmDelete())
                    break;
            }
        }
        else
        {
            if(bWasHeld && !bHoldFired)
            {
                lifecycle.Decide(backCommand.empty() ? ExitAction::Resume : ExitAction::Leave);
                if(!backCommand.empty())
                {
                    MarkFdsCloseOnExec();
                    system(backCommand.c_str());
                }
                break;
            }
            bHoldFired = false;
        }
        bWasHeld = bHeldNow;

        DrawScreen();
        SetDrawColor(renderer, bg);
        sdl_context.EndFrame();
    }

    return 0;
}
