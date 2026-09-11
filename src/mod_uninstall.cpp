/**
  * Copyright (C) 2017-2020 CompCom
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
#include "localization.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    const std::string OptionsLocation = "/etc/options_menu/";
    const int ListFontSize = 16;

    struct Mod
    {
        std::string name;
        Texture text;
    };

    // "hakchi pack_list" lines keep their trailing newline
    std::string TrimTrailingNewline(std::string s)
    {
        while(!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        return s;
    }

    std::string TruncateToWidth(const std::string & text, int maxWidth)
    {
        if(MeasureTTFWidth(text, ListFontSize) <= maxWidth)
            return text;
        int n = Utf8Length(text);
        while(n > 0 && MeasureTTFWidth(TruncateUtf8(text, n) + "...", ListFontSize) > maxWidth)
            --n;
        return TruncateUtf8(text, n) + "...";
    }
}

int main(int argc, char * argv[])
{
    std::string folderLocation(argv[0]);
    folderLocation = folderLocation.substr(0, folderLocation.find_last_of('/')+1);

    std::vector<Mod> installedMods;
    {
        char buf[256];
        auto pipe = popen("hakchi pack_list", "r");
        if(pipe)
        {
            while(!feof(pipe))
                if(fgets(buf, sizeof(buf), pipe))
                    installedMods.push_back({TrimTrailingNewline(buf), {}});
            pclose(pipe);
        }
    }

    if(installedMods.empty())
    {
        std::cerr << "Cannot find mods";
        return 1;
    }

    LoadLanguageFromConfig(OptionsLocation);
    SetTTFFontPath(OptionsLocation);

    SDL_Context sdl_context(std::chrono::milliseconds(33), false);
    auto renderer = sdl_context.renderer;
    Controller controller(1);

    const Uint8 bgR = UiTheme::BgR, bgG = UiTheme::BgG, bgB = UiTheme::BgB;
    SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, SDL_ALPHA_OPAQUE);

    Texture gearIcon(OptionsLocation + UiTheme::AssetGear, renderer, UiTheme::GearX, UiTheme::GearY);
    Texture appTitleText("OptionsMenu", UiTheme::TitleFontSize, renderer, UiTheme::TitleX, UiTheme::TitleY, false, 0xFFFFFFFF, true);
    appTitleText.rect.y -= appTitleText.rect.h / 2;
    Texture appVersionText(MOD_VERSION, UiTheme::VersionFontSize, renderer, appTitleText.rect.x + appTitleText.rect.w + UiTheme::VersionGap, UiTheme::TitleY, false, 0xFFFFFFFF, true);
    appVersionText.rect.y -= appVersionText.rect.h / 2;
    Texture titleText(Translate("MODULE_UNINSTALLER"), UiTheme::SectionTitleFontSize, renderer, UiTheme::SectionTitleX, UiTheme::SectionTitleY, false, 0xFFFFFFFF, true);
    Texture creditText("created by CompCom - Modern UI by DefKorns", 16, renderer, UiTheme::CreditX, UiTheme::CreditY, false, 0xFFFFFFFF, true);

    // two columns sharing the row list's usual width, split by a center divider
    const int ColumnGap = 40;
    const int ColumnW = (UiTheme::ListContentRightX - UiTheme::ListX - ColumnGap) / 2;
    const int LeftColumnX = UiTheme::ListX;
    const int RightColumnX = LeftColumnX + ColumnW + ColumnGap;
    const int DividerX = LeftColumnX + ColumnW + ColumnGap/2;
    const int ColumnHeaderY = UiTheme::SectionTitleY + 46;
    const int ListTopY = ColumnHeaderY + 32;
    const int ListBottomY = UiTheme::FooterDividerY - 16;
    const int rowPitch = GetTTFLineHeight(ListFontSize);
    const int displayCount = std::max(1, (ListBottomY - ListTopY) / rowPitch);

    Texture installedHeader(Translate("MOD_UNINSTALLER_INSTALLED"), 18, renderer, LeftColumnX, ColumnHeaderY, false, UiTheme::TextDimColor, true);
    Texture toRemoveHeader(Translate("MOD_UNINSTALLER_TO_REMOVE"), 18, renderer, RightColumnX, ColumnHeaderY, false, UiTheme::TextDimColor, true);

    for(Mod & mod : installedMods)
        mod.text = Texture(TruncateToWidth(mod.name, ColumnW), ListFontSize, renderer, LeftColumnX, 0, false, 0xFFFFFFFF, true);

    // this screen's list spans a different vertical range than the main menu's, so the
    // chevrons are anchored to it directly rather than reusing UiTheme::ScrollUpY/DownY
    Texture scrollUp("^", 16, renderer, UiTheme::ScrollX, ListTopY + 10, false, 0xFFFFFFFF, true);
    scrollUp.rect.x -= scrollUp.rect.w / 2;
    Texture scrollDown = scrollUp;
    scrollDown.rect.y = ListBottomY - 50;

    Texture badgeOuter(OptionsLocation + UiTheme::AssetBadgeOuter, renderer);
    Texture badgeInner(OptionsLocation + UiTheme::AssetBadgeInner, renderer);
    struct Badge { Texture letter; Texture label; UiTheme::BadgeColor rim; UiTheme::BadgeColor fill; };
    Badge badgeAdd{ Texture("A", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor, true), Texture(Translate("HINT_ADD"), 16, renderer, 0, 0, false, 0xFFFFFFFF, true), UiTheme::BadgeADark, UiTheme::BadgeA };
    Badge badgeUndo{ Texture("B", 16, renderer, 0, 0, false, UiTheme::BadgeLetterColor, true), Texture(Translate("HINT_UNDO"), 16, renderer, 0, 0, false, 0xFFFFFFFF, true), UiTheme::BadgeBDark, UiTheme::BadgeB };
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

    // wide "Start" chip instead of a single-letter circle - "St" alone reads ambiguous next to Select
    Texture pillText("Start", 14, renderer, 0, 0, false, UiTheme::BadgeLetterColor, true);
    Texture uninstallLabel(Translate("HINT_UNINSTALL"), 16, renderer, 0, 0, false, 0xFFFFFFFF, true);
    auto DrawPillBadge = [&](Texture & pill, Texture & label, const UiTheme::BadgeColor & rim, const UiTheme::BadgeColor & fill, int rightEdgeX) -> int
    {
        const int pillPadX = 10;
        int pillW = pill.rect.w + pillPadX*2;
        int groupW = pillW + UiTheme::BadgeLabelGap + label.rect.w;
        int x = rightEdgeX - groupW;
        int y = UiTheme::BadgeBandY;
        SDL_Rect pillRect{ x, y, pillW, UiTheme::BadgeOuterSize };
        DrawRoundedFillRect(renderer, pillRect, rim.r, rim.g, rim.b, pillRect.h/2);
        // same ring thickness as the letter badges' outer/inner circle pair
        int border = (UiTheme::BadgeOuterSize - UiTheme::BadgeInnerSize) / 2;
        SDL_Rect fillRect{ x+border, y+border, pillW-border*2, UiTheme::BadgeInnerSize };
        DrawRoundedFillRect(renderer, fillRect, fill.r, fill.g, fill.b, fillRect.h/2);
        pill.rect.x = x + (pillW - pill.rect.w)/2;
        pill.rect.y = y + (UiTheme::BadgeOuterSize - pill.rect.h)/2;
        pill.Draw(renderer);
        label.rect.x = x + pillW + UiTheme::BadgeLabelGap;
        label.rect.y = y + (UiTheme::BadgeOuterSize - label.rect.h)/2;
        label.Draw(renderer);
        return x - UiTheme::BadgeGroupGap;
    };

    int installListOffset = 0;
    int currentId = 0;
    std::vector<Mod> uninstallList;

    auto DrawChrome = [&]()
    {
        DrawStrokeRect(renderer, UiTheme::OuterRect, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth, UiTheme::BorderRadius);
        gearIcon.Draw(renderer);
        appTitleText.Draw(renderer);
        appVersionText.Draw(renderer);
        DrawHLine(renderer, UiTheme::HeaderDividerX, UiTheme::HeaderDividerX + UiTheme::HeaderDividerW, UiTheme::HeaderDividerY, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth);
        DrawHLine(renderer, UiTheme::OuterRect.x, UiTheme::OuterRect.x + UiTheme::OuterRect.w, UiTheme::FooterDividerY, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth);
        int accentBarY = titleText.rect.y + (titleText.rect.h - UiTheme::SectionAccentBarH) / 2;
        DrawFillRect(renderer, { UiTheme::SectionTitleX - UiTheme::SectionAccentBarW - 14, accentBarY, UiTheme::SectionAccentBarW, UiTheme::SectionAccentBarH }, UiTheme::AccentR, UiTheme::AccentG, UiTheme::AccentB);
        titleText.Draw(renderer);
        creditText.Draw(renderer);
    };

    auto render = [&]()
    {
        sdl_context.StartFrame();
        DrawChrome();
        installedHeader.Draw(renderer);
        toRemoveHeader.Draw(renderer);
        DrawVLine(renderer, DividerX, ColumnHeaderY, ListBottomY, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth);

        int count = std::min(displayCount, (int)installedMods.size()-installListOffset);
        for(int i = 0; i < count; ++i)
        {
            Texture & rowText = installedMods[installListOffset+i].text;
            rowText.rect.x = LeftColumnX;
            rowText.rect.y = ListTopY + i*rowPitch + (rowPitch - rowText.rect.h)/2;
            if(i == currentId)
            {
                SDL_Rect rowRect{ LeftColumnX - 8, rowText.rect.y - 2, ColumnW + 8, rowText.rect.h + 4 };
                DrawRoundedFillRect(renderer, rowRect, UiTheme::SelectedRowBgR, UiTheme::SelectedRowBgG, UiTheme::SelectedRowBgB, UiTheme::BoxRadius);
                DrawStrokeRect(renderer, rowRect, UiTheme::AccentR, UiTheme::AccentG, UiTheme::AccentB, 2, UiTheme::BoxRadius);
            }
            rowText.Draw(renderer);
        }
        if(installListOffset != 0)
            scrollUp.Draw(renderer);
        if(installListOffset + displayCount < (int)installedMods.size())
            scrollDown.Draw(renderer, SDL_FLIP_VERTICAL);

        int uninstallCount = std::min(displayCount, (int)uninstallList.size());
        for(int i = 0; i < uninstallCount; ++i)
        {
            Texture & rowText = uninstallList[i].text;
            rowText.rect.x = RightColumnX;
            rowText.rect.y = ListTopY + i*rowPitch + (rowPitch - rowText.rect.h)/2;
            rowText.Draw(renderer);
        }

        int rightEdge = UiTheme::BadgeClusterRightX;
        rightEdge = DrawBadge(badgeAdd, rightEdge);
        rightEdge = DrawBadge(badgeUndo, rightEdge);
        DrawPillBadge(pillText, uninstallLabel, UiTheme::BadgeStartDark, UiTheme::BadgeStart, rightEdge);

        SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 0xFF); // flat helpers leave the draw color dirty
        sdl_context.EndFrame();
    };

    // true if the uninstall was confirmed and kicked off - caller should break out of the main loop
    auto ConfirmUninstall = [&]() -> bool
    {
        Texture confirmTitle(Translate("MOD_UNINSTALL_CONFIRM"), 24, renderer, 640, 320, true, 0xFFFFFFFF, true);
        Texture confirmHint(Translate("MOD_UNINSTALL_CONFIRM_HINT"), 16, renderer, 640, 360, true, 0xFFFFFFFF, true);
        controller.GetButtonStatus(B); // consume the still-held B from whatever press opened this dialog
        bool confirmed = false;
        for(;;)
        {
            controller.Update();
            if(sdl_context.powerwatch->buttonPress() || controller.GetButtonStatus(B))
                break;
            if(controller.PeekButtonStatus(START) && controller.PeekButtonStatus(SELECT))
            {
                confirmed = true;
                break;
            }
            sdl_context.StartFrame();
            DrawFillRect(renderer, UiTheme::FrameRect, bgR, bgG, bgB);
            DrawStrokeRect(renderer, UiTheme::FrameRect, UiTheme::BorderR, UiTheme::BorderG, UiTheme::BorderB, UiTheme::BorderWidth, UiTheme::BorderRadius);
            confirmTitle.Draw(renderer);
            confirmHint.Draw(renderer);
            SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 0xFF);
            sdl_context.EndFrame();
        }
        if(!confirmed)
            return false;

        std::ofstream out("/tmp/uninstall");
        if(out.is_open())
        {
            for(auto & mod : uninstallList)
                out << mod.name << " ";
            out.close();
            system(("/bin/sh " + folderLocation + "/FinishUninstall.sh &").c_str());
        }
        return true;
    };

    for(;;)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
            if(e.type == SDL_QUIT)
                return 0;

        controller.Update();
        if(sdl_context.powerwatch->buttonPress())
            break;

        if(controller.GetButtonStatus(UP))
        {
            if(currentId > 0)
                --currentId;
            else if(installListOffset)
                --installListOffset;
        }
        else if(controller.GetButtonStatus(DOWN))
        {
            if(currentId < displayCount-1 && installListOffset+currentId+1 < (int)installedMods.size())
                ++currentId;
            else if(installListOffset+displayCount < (int)installedMods.size())
                ++installListOffset;
        }
        else if(controller.GetButtonStatus(A))
        {
            if(currentId != -1 && !installedMods.empty())
            {
                uninstallList.push_back(installedMods[installListOffset+currentId]);
                installedMods.erase(installedMods.begin()+installListOffset+currentId);
                if(installListOffset && installListOffset+displayCount > (int)installedMods.size())
                    --installListOffset;
                if(installListOffset == 0 && currentId >= (int)installedMods.size())
                    currentId = static_cast<int>(installedMods.size()) - 1; // -1 (empty) is a valid sentinel here
            }
        }
        else if(controller.GetButtonStatus(B))
        {
            if(!uninstallList.empty())
            {
                installedMods.push_back(uninstallList.back());
                uninstallList.pop_back();
                if(currentId == -1)
                    currentId = 0;
            }
        }
        else if(controller.GetButtonStatus(START))
        {
            if(uninstallList.empty() || ConfirmUninstall())
                break;
        }

        render();
    }

    return 0;
}
