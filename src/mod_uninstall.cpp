/**
  * Copyright (C) 2017-2020 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "framework/badge.h"
#include "framework/sdl_helper.h"
#include "framework/controller.h"
#include "framework/powerwatch.h"
#include "framework/draw_helpers.h"
#include "framework/list_jump.h"
#include "framework/list_screen.h"
#include "framework/panel_dialog.h"
#include "framework/text_layout.h"
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
    const std::string HiddenModPrefix = "options_deluxe";
    const int ListFontSize = 16;
    const int PanelFontSize = 16;
    const int PanelHeaderFontSize = 18;

    const int ExitBack = 0;
    const int ExitLeaveMenu = 1;
    const int ExitUninstallStarted = 2;

    const Color White{ 255, 255, 255 };

    struct Mod
    {
        std::string name;
        Texture text;
        bool marked = false;
    };

    std::string TrimTrailingNewline(std::string s)
    {
        while(!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        return s;
    }

    void DrawDot(SDL_Renderer * renderer, int x, int centerY)
    {
        DrawRoundedFillRect(renderer, { x, centerY - 4, 8, 8 }, UiTheme::CheckboxOn, 4);
    }
}

int main(int argc, char * argv[])
{
    std::string folderLocation(argv[0]);
    folderLocation = folderLocation.substr(0, folderLocation.find_last_of('/')+1);

    std::vector<Mod> mods;
    {
        char buf[256];
        auto pipe = popen("hakchi pack_list", "r");
        if(pipe)
        {
            while(fgets(buf, sizeof(buf), pipe))
            {
                std::string name = TrimTrailingNewline(buf);
                if(!name.empty() && name.compare(0, HiddenModPrefix.size(), HiddenModPrefix) != 0)
                {
                    Mod mod;
                    mod.name = name;
                    mods.push_back(mod);
                }
            }
            pclose(pipe);
        }
    }

    LoadLanguageFromConfig(OptionsLocation);
    SetTTFFontPath(OptionsLocation);
    UiTheme::LoadThemeConfig(OptionsLocation);

    SDL_Context sdl_context(std::chrono::milliseconds(33), false);
    auto renderer = sdl_context.renderer;
    Controller controller(1);

    const Color bg = UiTheme::Bg;
    SetDrawColor(renderer, bg);

    ScreenChrome chrome(OptionsLocation, renderer, "OptionsMenu", MOD_VERSION, "created by CompCom - Modern UI by DefKorns");
    chrome.SetSectionTitle(Translate("MODULE_UNINSTALLER"));

    const int RowBoxH = UiTheme::RowPitch - 6;
    Texture checkboxOn(OptionsLocation + UiTheme::AssetCheckboxOn, renderer);
    SetColorMod(checkboxOn.texture.get(), UiTheme::CheckboxOn);
    Texture checkboxOff(OptionsLocation + UiTheme::AssetCheckboxOff, renderer);
    SetColorMod(checkboxOff.texture.get(), UiTheme::CheckboxOff);
    const int CheckboxSize = 20;
    const int CheckboxX = UiTheme::RowControlRightX - CheckboxSize;
    const int RowTextMaxW = CheckboxX - 16 - UiTheme::RowTextX;
    const int displayCount = std::max(1, (UiTheme::FooterDividerY - UiTheme::ListBottomMargin - UiTheme::RowFirstY) / UiTheme::RowPitch);

    for(Mod & mod : mods)
        mod.text = Texture(TruncateToWidth(mod.name, ListFontSize, RowTextMaxW), ListFontSize, renderer, UiTheme::RowTextX, 0, false, ToAbgr(White), true);
    Texture emptyText(Translate("MOD_UNINSTALL_EMPTY"), ListFontSize, renderer, UiTheme::RowTextX, UiTheme::RowFirstY, false, ToAbgr(UiTheme::TextDim), true);

    Texture scrollUp(OptionsLocation + UiTheme::AssetChevronUp, renderer, UiTheme::ScrollX, UiTheme::ScrollUpY);
    SetColorMod(scrollUp.texture.get(), UiTheme::ScrollArrow);
    Texture scrollDown = scrollUp;
    scrollDown.rect.y = UiTheme::ScrollDownY;

    const int PanelPad = 16;
    const int PanelInnerX = UiTheme::DetailX + PanelPad;
    const int PanelInnerW = UiTheme::DetailW - 2*PanelPad;
    const SDL_Rect summaryBox{ UiTheme::DetailX, UiTheme::RowFirstY, UiTheme::DetailW, 260 };
    const int SummaryDividerY = summaryBox.y + 46;
    const int SummaryListY = summaryBox.y + 62;
    const int SummaryRowPitch = 28;
    const int SummaryMaxRows = (summaryBox.y + summaryBox.h - PanelPad - SummaryListY) / SummaryRowPitch;
    const int BulletTextGap = 18;
    NoteBox rebootNote(renderer, { UiTheme::DetailX, summaryBox.y + summaryBox.h + 16, UiTheme::DetailW, 72 });
    rebootNote.SetText(Translate("MOD_UNINSTALL_REBOOT_NOTE"));

    Texture summaryHeader(Translate("MOD_UNINSTALLER_TO_REMOVE"), PanelHeaderFontSize, renderer, PanelInnerX, summaryBox.y + 14, false, ToAbgr(UiTheme::TextDim), true);
    Texture summaryCount;
    Texture noneMarkedText(Translate("MOD_UNINSTALL_NONE_MARKED"), PanelFontSize, renderer, PanelInnerX, SummaryListY, false, ToAbgr(UiTheme::Text), true);
    std::vector<Texture> noneHintLines = MakeLines(WrapToWidth(Translate("MOD_UNINSTALL_NONE_HINT"), PanelFontSize, PanelInnerW), PanelFontSize, renderer, UiTheme::TextDim);
    Texture moreText;
    std::vector<Texture> summaryNames;

    BadgePainter badges(OptionsLocation, renderer);
    auto MakeBadge = [&](const std::string & letter, const std::string & labelKey, Color rim, Color fill) -> Badge
    {
        return badges.Make(letter, Translate(labelKey), rim, fill);
    };
    Badge badgeMark = MakeBadge("A", "HINT_MARK", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge badgeUnmark = MakeBadge("A", "HINT_UNMARK", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge badgeBack = MakeBadge("B", "HINT_BACK", UiTheme::BadgeBDark, UiTheme::BadgeB);
    Badge badgeCancel = MakeBadge("B", "HINT_CANCEL", UiTheme::BadgeBDark, UiTheme::BadgeB);
    Badge uninstallPill = badges.MakePill("Start", "");
    Badge confirmPill = badges.MakePill("Start + Select", Translate("HINT_CONFIRM"));
    auto DrawBadge = [&](Badge & badge, int rightEdgeX) -> int
    {
        return badges.Draw(badge, rightEdgeX, UiTheme::BadgeBandY);
    };

    int listOffset = 0;
    int currentId = 0;
    int markedCount = 0;
    const int modCount = static_cast<int>(mods.size());

    auto MarkedMods = [&]()
    {
        std::vector<const Mod *> marked;
        for(const Mod & mod : mods)
            if(mod.marked)
                marked.push_back(&mod);
        return marked;
    };

    auto RefreshSummary = [&]()
    {
        auto marked = MarkedMods();
        const int count = static_cast<int>(marked.size());
        markedCount = count;
        summaryCount = Texture(std::to_string(count), PanelHeaderFontSize, renderer, 0, summaryBox.y + 14, false, ToAbgr(count ? UiTheme::CheckboxOn : UiTheme::TextDim), true);
        summaryCount.rect.x = summaryBox.x + summaryBox.w - PanelPad - summaryCount.rect.w;
        uninstallPill.label = Texture(Translate("HINT_UNINSTALL") + " (" + std::to_string(count) + ")", 16, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true);

        int shown = (count > SummaryMaxRows) ? SummaryMaxRows - 1 : count;
        summaryNames.clear();
        for(int i = 0; i < shown; ++i)
            summaryNames.emplace_back(TruncateToWidth(marked[i]->name, PanelFontSize, PanelInnerW - BulletTextGap), PanelFontSize, renderer, PanelInnerX + BulletTextGap, 0, false, ToAbgr(UiTheme::Text), true);
        moreText = (shown < count)
            ? Texture("+" + std::to_string(count - shown), PanelFontSize, renderer, PanelInnerX + BulletTextGap, 0, false, ToAbgr(UiTheme::TextDim), true)
            : Texture();
    };
    RefreshSummary();

    auto DrawList = [&]()
    {
        if(mods.empty())
        {
            emptyText.rect.y = UiTheme::RowFirstY + (RowBoxH - emptyText.rect.h)/2;
            emptyText.Draw(renderer);
            return;
        }
        int count = std::min(displayCount, modCount - listOffset);
        for(int i = 0; i < count; ++i)
        {
            Mod & mod = mods[listOffset + i];
            SDL_Rect rowRect = DrawListRow(renderer, i, UiTheme::ListContentRightX, RowBoxH, i == currentId);
            int centerY = rowRect.y + rowRect.h/2;
            mod.text.rect.y = centerY - mod.text.rect.h/2;
            SetColorMod(mod.text.texture.get(), mod.marked ? UiTheme::TextDim : UiTheme::Text);
            mod.text.Draw(renderer);
            if(mod.marked)
                DrawHLine(renderer, mod.text.rect.x, mod.text.rect.x + mod.text.rect.w, centerY + 1, UiTheme::TextDim, 1);
            Texture & checkbox = mod.marked ? checkboxOn : checkboxOff;
            checkbox.rect.x = CheckboxX;
            checkbox.rect.y = centerY - checkbox.rect.h/2;
            checkbox.Draw(renderer);
        }
        if(listOffset != 0)
            scrollUp.Draw(renderer);
        if(listOffset + displayCount < modCount)
            scrollDown.Draw(renderer, SDL_FLIP_VERTICAL);
    };

    auto DrawSummary = [&]()
    {
        DrawStrokeRect(renderer, summaryBox, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BoxRadius);
        summaryHeader.Draw(renderer);
        summaryCount.Draw(renderer);
        DrawHLine(renderer, PanelInnerX, PanelInnerX + PanelInnerW, SummaryDividerY, UiTheme::Border, 1);
        if(markedCount == 0)
        {
            noneMarkedText.Draw(renderer);
            int y = SummaryListY + 28;
            for(Texture & line : noneHintLines)
            {
                line.rect.x = PanelInnerX;
                line.rect.y = y;
                line.Draw(renderer);
                y += 22;
            }
        }
        else
        {
            int y = SummaryListY;
            for(Texture & name : summaryNames)
            {
                int centerY = y + SummaryRowPitch/2 - 4;
                DrawDot(renderer, PanelInnerX, centerY);
                name.rect.y = centerY - name.rect.h/2;
                name.Draw(renderer);
                y += SummaryRowPitch;
            }
            if(moreText.texture)
            {
                moreText.rect.y = y + SummaryRowPitch/2 - 4 - moreText.rect.h/2;
                moreText.Draw(renderer);
            }
        }

        rebootNote.Draw();
    };

    auto DrawScreen = [&]()
    {
        chrome.Draw();
        DrawList();
        DrawSummary();

        int rightEdge = UiTheme::BadgeClusterRightX;
        if(!mods.empty())
            rightEdge = DrawBadge(mods[listOffset + currentId].marked ? badgeUnmark : badgeMark, rightEdge);
        rightEdge = DrawBadge(badgeBack, rightEdge);
        if(markedCount > 0)
            rightEdge = DrawBadge(uninstallPill, rightEdge);
        DrawFooterDivider(renderer, rightEdge);
    };

    auto render = [&]()
    {
        sdl_context.StartFrame();
        DrawScreen();
        SetDrawColor(renderer, bg);
        sdl_context.EndFrame();
    };

    auto ConfirmUninstall = [&]()
    {
        auto marked = MarkedMods();
        const int maxNames = 8;
        PanelDialogText text;
        text.title = Translate("MOD_UNINSTALL_CONFIRM");
        for(int i = 0; i < std::min(maxNames, static_cast<int>(marked.size())); ++i)
            text.body.push_back(marked[i]->name);
        text.hiddenCount = std::max(0, static_cast<int>(marked.size()) - maxNames);
        text.note = Translate("MOD_UNINSTALL_REBOOT_NOTE");
        PanelDialog dialog(renderer, badges, PanelDialog::Kind::Confirm, text, { &badgeCancel, &confirmPill });
        dialog.SetBulletColor(UiTheme::CheckboxOn);
        const DialogResult answer = dialog.Run(sdl_context, controller, DrawScreen);
        if(answer != DialogResult::Confirmed)
            return answer;

        std::ofstream out("/tmp/uninstall");
        if(!out.is_open())
            return DialogResult::Cancelled;
        for(const Mod * mod : marked)
            out << mod->name << " ";
        out.close();
        system(("/bin/sh " + folderLocation + "/FinishUninstall.sh &").c_str());
        return DialogResult::Confirmed;
    };

    ListJumpInput jumpInput;
    for(;;)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
            if(e.type == SDL_QUIT)
                return ExitLeaveMenu;

        controller.Update();
        if(sdl_context.powerwatch->buttonPress())
            return ExitLeaveMenu;

        const ListJump jump = jumpInput.Poll(controller, SDL_GetTicks());
        if(controller.HeldRepeat(UP) && !mods.empty())
        {
            if(currentId > 0)
                --currentId;
            else if(listOffset > 0)
                --listOffset;
        }
        else if(controller.HeldRepeat(DOWN) && !mods.empty())
        {
            if(currentId < displayCount-1 && listOffset+currentId+1 < modCount)
                ++currentId;
            else if(listOffset+displayCount < modCount)
                ++listOffset;
        }
        else if(jump != ListJump::None && !mods.empty())
        {
            std::vector<std::string> labels;
            for(const Mod & mod : mods)
                labels.push_back(mod.name);
            ApplyListJump(jump, labels, 0, displayCount, listOffset, currentId);
        }
        else if(controller.GetButtonStatus(A) && !mods.empty())
        {
            Mod & mod = mods[listOffset + currentId];
            mod.marked = !mod.marked;
            RefreshSummary();
        }
        else if(controller.GetButtonStatus(B))
            return ExitBack;
        else if(controller.GetButtonStatus(START) && markedCount > 0)
        {
            const DialogResult answer = ConfirmUninstall();
            if(answer == DialogResult::Quit)
                return ExitLeaveMenu;
            if(answer == DialogResult::Confirmed)
                return ExitUninstallStarted;
        }

        render();
    }
}
