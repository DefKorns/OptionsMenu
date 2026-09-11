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

// truncates by UTF-8 codepoint, not byte, so multi-byte glyphs (e.g. Japanese) aren't split
std::string TruncateUtf8(const std::string & text, int maxCodepoints)
{
    size_t i = 0;
    int count = 0;
    while(i < text.size() && count < maxCodepoints)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        size_t len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        i += len;
        ++count;
    }
    return text.substr(0, i);
}

int Utf8Length(const std::string & text)
{
    int count = 0;
    for(size_t i = 0; i < text.size();)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        i += (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        ++count;
    }
    return count;
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
    SetTTFFontPath(optionsLocation);

    std::string uiStyle;
    in.open("/etc/options_menu/ui_style.cfg");
    std::getline(in, uiStyle);
    in.close();
    bool modernUI = (uiStyle != "classic");

    std::string titleString(Translate("OPTIONS_TITLE"));
    std::string titleKey = "OPTIONS_TITLE";
    // ancestors, root-first: "path,scriptPath,titleKey;..." - inherited via env, no argv needed
    const char * backStackEnv = getenv("OM_BACK_STACK");
    std::string backStack = backStackEnv ? backStackEnv : "";

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
            titleKey = argv[i+1];
            titleString = Translate(argv[i+1]);
            ++i;
        }
    }

    // what a child screen inherits as its own OM_BACK_STACK
    std::string myEntry = commandLocation + "," + scriptLocation + "," + titleKey;
    setenv("OM_BACK_STACK", (backStack.empty() ? myEntry : backStack + ";" + myEntry).c_str(), 1);

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

    // commands[0] below would be UB on an empty vector
    if(commands.empty())
    {
        std::cerr << "No usable commands in " << commandLocation << "\n";
        exit(1);
    }

    // synthesized, not read from c9999_Back/c9999_Exit files - Back only if backStack has an entry to pop
    static const char * const NavPreviewImage = "/etc/options_menu/images/preview_placeholder.png";
    const int pinnedStartIndex = static_cast<int>(commands.size());
    if(!backStack.empty())
    {
        // pop the last entry (our parent), re-join the rest for the parent's own Back chain
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

        Command back;
        back.name = "BACK";
        back.runInternal = false;
        back.restartUI = false;
        back.previewImage = NavPreviewImage;
        // env prefix overrides OM_BACK_STACK for just this launch, to the parent's own remaining ancestors
        back.command = "usleep 50000 && OM_BACK_STACK=\"" + remainingStack + "\" " + optionsLocation + "options --commandPath " + backPath
            + (backScriptPath.empty() ? "" : " --scriptPath " + backScriptPath)
            + " --title \"" + backTitleKey + "\" &";
        commands.push_back(back);
    }
    Command exitCmd;
    exitCmd.name = "EXIT";
    exitCmd.runInternal = false;
    exitCmd.restartUI = true;
    exitCmd.previewImage = NavPreviewImage;
    exitCmd.command = "echo \"Closing Options Menu\"";
    commands.push_back(exitCmd);

    int currentCommandId = 0;

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

    // only the delete-confirm modal still uses this (NineSlice)
    NineSlice frame(optionsLocation + UiTheme::AssetFrame, renderer, UiTheme::FrameInset, UiTheme::FrameInset, UiTheme::FrameInset, UiTheme::FrameInset);
    Texture gearIcon(optionsLocation + UiTheme::AssetGear, renderer, UiTheme::GearX, UiTheme::GearY);
    Texture switchOn(optionsLocation + UiTheme::AssetSwitchOn, renderer);
    Texture switchOff(optionsLocation + UiTheme::AssetSwitchOff, renderer);
    Texture badgeOuter(optionsLocation + UiTheme::AssetBadgeOuter, renderer);
    Texture badgeInner(optionsLocation + UiTheme::AssetBadgeInner, renderer);

    //Create Textures for Strings
    // only drawn in the modernUI branch
    Texture appTitleText("OptionsMenu", UiTheme::TitleFontSize, renderer, UiTheme::TitleX, UiTheme::TitleY, false, 0xFFFFFFFF, true);
    appTitleText.rect.y -= appTitleText.rect.h / 2; // vertically center on the gear (Texture only supports centering both axes together)
    Texture appVersionText(MOD_VERSION, UiTheme::VersionFontSize, renderer, appTitleText.rect.x + appTitleText.rect.w + UiTheme::VersionGap, UiTheme::TitleY, false, 0xFFFFFFFF, true);
    appVersionText.rect.y -= appVersionText.rect.h / 2;
    Texture titleText = modernUI
        ? Texture(titleString, UiTheme::SectionTitleFontSize, renderer, UiTheme::SectionTitleX, UiTheme::SectionTitleY, false, 0xFFFFFFFF, true)
        : Texture(titleString, 36, renderer, 640, 185, true);
    SDL_Rect selectedRowRect{ UiTheme::ListX, UiTheme::RowFirstY - 2, UiTheme::ListContentRightX - UiTheme::ListX, 0 };
    Texture pointerText("->", 16, renderer, 20, UiTheme::RowFirstY, false, 0xFF00FF00);
    SDL_Rect & pointerRect = pointerText.rect;
    Texture CompComText = modernUI
        ? Texture("created by CompCom - Modern UI by DefKorns", 16, renderer, UiTheme::CreditX, UiTheme::CreditY, false, 0xFFFFFFFF, true)
        : Texture("created by CompCom", 16, renderer, 1100, 620, true);
    Texture deleteHint(Translate("WIFI_DELETE_HINT_FOOTER"), 16, renderer, 30, 612);
    Texture scrollUp = modernUI
        ? Texture("^", 16, renderer, UiTheme::ScrollX, UiTheme::ScrollUpY, false, 0xFFFFFFFF, true)
        : Texture("^", 16, renderer, 30, 248);
    if(modernUI)
        scrollUp.rect.x -= scrollUp.rect.w / 2; // ScrollX is the gutter's center, not the glyph's left edge
    Texture scrollDown = scrollUp;
    scrollDown.rect.y = modernUI ? UiTheme::ScrollDownY : (252+UiTheme::DisplayItemCount*18);

    //Badge cluster (top-right hints): A/B always shown, X only if the row has a delete action
    // badges only ever exist in modernUI, so their textures are unconditionally TTF
    struct Badge { Texture letter; Texture label; UiTheme::BadgeColor rim; UiTheme::BadgeColor fill; };
    Badge badgeA{ Texture("A", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor, true), Texture(Translate("HINT_SELECT"), 16, renderer, 0, 0, false, 0xFFFFFFFF, true), UiTheme::BadgeADark, UiTheme::BadgeA };
    Badge badgeB{ Texture("B", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor, true), Texture(Translate("HINT_BACK"), 16, renderer, 0, 0, false, 0xFFFFFFFF, true), UiTheme::BadgeBDark, UiTheme::BadgeB };
    // hold-to-delete hint, different color than the real B badge
    Badge badgeHold{ Texture("B", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor, true), Texture(Translate("HINT_DELETE"), 16, renderer, 0, 0, false, 0xFFFFFFFF, true), UiTheme::BadgeXDark, UiTheme::BadgeX };
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
    const int RowGlyphSize = 16;
    const int RowTextGapPx = 16; // gap kept between truncated text and preview image/switch
    Texture chevronText(">", RowGlyphSize, renderer, 0, 0, false, UiTheme::TextDimColor, modernUI);
    for(Command & c : commands)
    {
        // self-relaunch into the same dir (language, ModernUI toggle) isn't a submenu
        if(c.hasSubmenu)
        {
            size_t pos = c.command.find("--commandPath");
            size_t pathStart = c.command.find_first_not_of(' ', pos + std::string("--commandPath").size());
            size_t pathEnd = c.command.find(' ', pathStart);
            std::string targetPath = c.command.substr(pathStart, pathEnd - pathStart);
            if(targetPath == commandLocation)
                c.hasSubmenu = false;
        }
        // Back also relaunches elsewhere - name is the untranslated key, language-independent
        if(c.name == "BACK")
            c.hasSubmenu = false;

        int textX = rowTextBaseX + (c.child ? ChildIndent : 0);
        std::string label = Translate(c.name);

        // classic UI: only rows with a preview image get less width for text.
        // modern UI: the list column has a fixed width regardless (previews
        // now live in the detail panel, not floating on the row).
        int maxRight = -1;
        if(!modernUI && c.previewImage.size())
            maxRight = c.previewImageX - RowTextGapPx;
        if(modernUI)
            maxRight = UiTheme::RowControlRightX - RowTextGapPx;

        bool rowUsesTTF = modernUI && CanRenderWithTTF(label, RowGlyphSize);
        if(maxRight >= 0)
        {
            int available = std::max(0, maxRight - textX);
            if(rowUsesTTF)
            {
                // proportional font: shrink by measured width, not a fixed px/char guess
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
        }

        c.texture = Texture(label, RowGlyphSize, renderer, textX, 0, false, 0xFFFFFFFF, modernUI);
    }

    const int modernRowPitch = std::max(UiTheme::RowPitch, GetTTFLineHeight(RowGlyphSize));
    int pinnedAreaTop = UiTheme::FooterDividerY - UiTheme::PinnedBottomMargin;
    if(modernUI)
    {
        int slotBottom = UiTheme::FooterDividerY - UiTheme::PinnedBottomMargin;
        for(int i = static_cast<int>(commands.size())-1; i >= pinnedStartIndex; --i)
        {
            int slotTop = slotBottom - modernRowPitch;
            commands[i].texture.rect.y = slotTop + (modernRowPitch - commands[i].texture.rect.h) / 2 + UiTheme::RowTextYNudge;
            slotBottom = slotTop;
        }
        pinnedAreaTop = slotBottom;
    }
    // scrollable rows that fit above the pinned block - varies per screen
    const int DisplayItemCount = modernUI
        ? std::max(1, (pinnedAreaTop - UiTheme::RowFirstY) / modernRowPitch)
        : UiTheme::DisplayItemCount;

    int topListItemNumber = 1;
    std::shared_ptr<Texture> PreviewImage;
    auto SetCurrentCommand = [&] (int newCommandId)
    {
        currentCommandId = newCommandId;
        const Command & currentCommand = commands[currentCommandId];

        // pinned rows don't affect scroll position
        int scrollBound = modernUI ? pinnedStartIndex : static_cast<int>(commands.size());
        int scrollTarget = modernUI ? std::min(currentCommandId, std::max(0, pinnedStartIndex-1)) : currentCommandId;

        bool updateCommandYPos = false;
        if(scrollTarget < topListItemNumber)
        {
            topListItemNumber = scrollTarget;
            updateCommandYPos = true;
        }
        else if(scrollTarget >= topListItemNumber+DisplayItemCount)
        {
            topListItemNumber = scrollTarget-DisplayItemCount+1;
            updateCommandYPos = true;
        }
        if(updateCommandYPos)
        {
            int y = UiTheme::RowFirstY;
            int rowPitch = modernUI ? modernRowPitch : 18;
            for(int i = 0, count = std::min(DisplayItemCount, scrollBound-topListItemNumber); i < count; ++i)
            {
                Texture & rowTexture = commands[i+topListItemNumber].texture;
                // centered in its slot - TTF glyphs have ascender padding, top-align leaves a gap
                rowTexture.rect.y = modernUI ? (y + (rowPitch - rowTexture.rect.h) / 2 + UiTheme::RowTextYNudge) : y;
                y += rowPitch;
            }
        }

        selectedRowRect.y = currentCommand.texture.rect.y - 2;
        selectedRowRect.h = currentCommand.texture.rect.h + 4; // wrap the row's actual text height (16px font8x8 or variable-height TTF), not a fixed guess
        pointerRect.y = currentCommand.texture.rect.y;

        // scaled to fit the detail panel box, ignores the command's own previewImageX/Y/W/H
        if(modernUI && currentCommand.previewImage.size())
        {
            PreviewImage = std::make_shared<Texture>(currentCommand.previewImage, renderer, 0, 0);
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

    auto DrawRow = [&](Command & rowCommand, bool isLastOverall)
    {
        bool selected = &rowCommand == &commands[currentCommandId];
        rowCommand.texture.Draw(renderer);
        if(modernUI && rowCommand.isToggle)
        {
            Texture & rowSwitch = rowCommand.stateOn ? switchOn : switchOff;
            rowSwitch.rect.x = UiTheme::SwitchRightX - UiTheme::SwitchW;
            rowSwitch.rect.y = rowCommand.texture.rect.y + (rowCommand.texture.rect.h - UiTheme::SwitchH) / 2;
            rowSwitch.Draw(renderer);
        }
        else if(modernUI && rowCommand.hasSubmenu)
        {
            chevronText.rect.x = UiTheme::RowControlRightX - chevronText.rect.w;
            chevronText.rect.y = rowCommand.texture.rect.y + (rowCommand.texture.rect.h - chevronText.rect.h) / 2;
            chevronText.Draw(renderer);
        }
        // skip for separators and the last row - nothing to separate there
        if(modernUI && !selected && !rowCommand.command.empty() && !isLastOverall)
            DrawHLine(renderer, UiTheme::ListX, UiTheme::ListContentRightX, rowCommand.texture.rect.y + rowCommand.texture.rect.h + 3, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB);
    };

    // modern-UI-only drawing, called early - must run before DrawRow (bg before text)
    auto DrawModernUI = [&]()
    {
        // one continuous border across header+body+footer, plus the footer divider
        DrawStrokeRect(renderer, UiTheme::OuterRect, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth, UiTheme::BorderRadius);
        gearIcon.Draw(renderer);
        appTitleText.Draw(renderer);
        appVersionText.Draw(renderer);
        DrawHLine(renderer, UiTheme::HeaderDividerX, UiTheme::HeaderDividerX + UiTheme::HeaderDividerW, UiTheme::HeaderDividerY, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth);
        DrawHLine(renderer, UiTheme::OuterRect.x, UiTheme::OuterRect.x + UiTheme::OuterRect.w, UiTheme::FooterDividerY, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth);
        // vertically centered on titleText's own rendered height, not a fixed guess
        int accentBarY = titleText.rect.y + (titleText.rect.h - UiTheme::SectionAccentBarH) / 2;
        DrawFillRect(renderer, { UiTheme::SectionTitleX - UiTheme::SectionAccentBarW - 14, accentBarY, UiTheme::SectionAccentBarW, UiTheme::SectionAccentBarH }, UiTheme::AccentR, UiTheme::AccentG, UiTheme::AccentB);

        DrawRoundedFillRect(renderer, selectedRowRect, UiTheme::SelectedRowBgR, UiTheme::SelectedRowBgG, UiTheme::SelectedRowBgB, UiTheme::BoxRadius);
        DrawStrokeRect(renderer, selectedRowRect, UiTheme::AccentR, UiTheme::AccentG, UiTheme::AccentB, 2, UiTheme::BoxRadius);

        // just the preview image - icon/title box looked wrong with no per-command art
        SDL_Rect previewBox{ UiTheme::DetailX, UiTheme::PreviewBoxY, UiTheme::DetailW, UiTheme::PreviewBoxH };
        DrawStrokeRect(renderer, previewBox, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth, UiTheme::BoxRadius);
        if(PreviewImage.get())
        {
            PreviewImage->rect.x = previewBox.x + (previewBox.w - PreviewImage->rect.w) / 2;
            PreviewImage->rect.y = previewBox.y + (previewBox.h - PreviewImage->rect.h) / 2;
            PreviewImage->Draw(renderer);
        }

        // footer: A rightmost, B left of it, hold-to-delete hint further left
        int rightEdge = UiTheme::BadgeClusterRightX;
        rightEdge = DrawBadge(badgeA, rightEdge);
        rightEdge = DrawBadge(badgeB, rightEdge);
        if(!commands[currentCommandId].deleteCommand.empty())
            rightEdge = DrawBadge(badgeHold, rightEdge);
        int dividerX = rightEdge - UiTheme::BadgeDividerGapFromCluster;
        DrawVLine(renderer, dividerX, UiTheme::FooterY + 10, UiTheme::FooterY + UiTheme::FooterH - 10, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, 2);
    };

    // true if deleted - caller should break out of the main loop
    auto ConfirmDelete = [&]() -> bool
    {
        const std::string & confirmKey = commands[currentCommandId].deleteConfirmKey;
        Texture confirmTitle(Translate(confirmKey.empty() ? "DELETE_CONFIRM_GENERIC" : confirmKey), 24, renderer, 640, 320, true, 0xFFFFFFFF, modernUI);
        Texture confirmHint(Translate("DELETE_CONFIRM_HINT"), 16, renderer, 640, 360, true, 0xFFFFFFFF, modernUI);
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
        if(!confirmed)
            return false;
        system(commands[currentCommandId].deleteCommand.c_str());
        _exitManager.runExitCommand = false;
        return true;
    };

    // ~1s at this loop's ~33ms frame time, same tick-count style as daemon.cpp's L+R hold
    const short bHoldThreshold = 30;
    short bHoldCount = 0;

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
                commands[currentCommandId].RunCommand(sdl_context, &controller, menuL, menuU, modernUI, { gearIcon, appTitleText, appVersionText, CompComText }, bgR, bgG, bgB);
                if(commands[currentCommandId].isToggle)
                    commands[currentCommandId].UpdateState();
            }
            else
            {
                // release the display first - the child's startup delay assumed our teardown was instant
                sdl_context.Shutdown();
                system(commands[currentCommandId].command.c_str());
                _exitManager.runExitCommand = commands[currentCommandId].restartUI;
                break;
            }
        }
        else if(controller.GetButtonStatus(UP))
        {
            // bounded so an all-headers list can't spin forever
            int newCommandId = currentCommandId;
            for(size_t tries = 0; tries < commands.size(); ++tries)
            {
                newCommandId = (newCommandId-1+commands.size())%commands.size();
                if(commands[newCommandId].command.size() != 0)
                    break;
            }
            SetCurrentCommand(newCommandId);
        }
        else if(controller.GetButtonStatus(DOWN))
        {
            int newCommandId = currentCommandId;
            for(size_t tries = 0; tries < commands.size(); ++tries)
            {
                newCommandId = (newCommandId+1)%commands.size();
                if(commands[newCommandId].command.size() != 0)
                    break;
            }
            SetCurrentCommand(newCommandId);
        }

        // tap = jump to last item, hold ~1s = delete
        if(controller.PeekButtonStatus(B))
        {
            ++bHoldCount;
            if(bHoldCount == bHoldThreshold && !commands[currentCommandId].deleteCommand.empty() && ConfirmDelete())
                break;
        }
        else
        {
            if(bHoldCount > 0 && bHoldCount < bHoldThreshold)
                SetCurrentCommand(commands.size()-1);
            bHoldCount = 0;
        }

        //Draw all textures
        if(modernUI)
            DrawModernUI();
        else
        {
            banner.Draw(renderer);
            menuU.Draw(renderer);
            menuL.Draw(renderer);
        }
        titleText.Draw(renderer);

        int lastCommandIndex = static_cast<int>(commands.size()) - 1;
        int scrollDrawBound = modernUI ? pinnedStartIndex : static_cast<int>(commands.size());
        for(int i = 0, count = std::min(DisplayItemCount, scrollDrawBound-topListItemNumber); i < count; ++i)
            DrawRow(commands[i+topListItemNumber], (i+topListItemNumber) == lastCommandIndex);

        // pinned trailing rows (Back/Exit) - always visible near the footer
        if(modernUI)
            for(int i = pinnedStartIndex; i <= lastCommandIndex; ++i)
                DrawRow(commands[i], i == lastCommandIndex);

        CompComText.Draw(renderer);

        if(!modernUI)
        {
            pointerText.Draw(renderer);
            if(commands[currentCommandId].deleteCommand.size())
                deleteHint.Draw(renderer);
        }

        // bounded by scrollDrawBound, not commands.size() - pinned rows need no scroll
        if(topListItemNumber != 0)
            scrollUp.Draw(renderer);
        if((topListItemNumber + DisplayItemCount) < scrollDrawBound)
            scrollDown.Draw(renderer, SDL_FLIP_VERTICAL);

        // flat-line helpers leave the draw color dirty - reset before EndFrame
        if(modernUI)
            SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 0xFF);

        //Render Framebuffer and Wait for Next Frame
        sdl_context.EndFrame();
    }

    return 0;
}
