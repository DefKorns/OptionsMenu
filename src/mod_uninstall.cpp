/**
  * Copyright (C) 2017-2020 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "framework/badge.h"
#include "framework/dialog.h"
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
    const std::string HiddenModPrefix = "options_deluxe";
    const int ListFontSize = 16;
    const int PanelFontSize = 16;
    const int PanelHeaderFontSize = 18;
    const int NoteFontSize = 15;

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

    int TextWidth(const std::string & text, int fontSize)
    {
        if(CanRenderWithTTF(text, fontSize))
            return MeasureTTFWidth(text, fontSize);
        return Utf8Length(text) * fontSize;
    }

    std::string TruncateToWidth(const std::string & text, int fontSize, int maxWidth)
    {
        if(TextWidth(text, fontSize) <= maxWidth)
            return text;
        int n = Utf8Length(text);
        while(n > 0 && TextWidth(TruncateUtf8(text, n) + "...", fontSize) > maxWidth)
            --n;
        return TruncateUtf8(text, n) + "...";
    }

    std::vector<std::string> WrapToWidth(const std::string & text, int fontSize, int maxWidth)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t pos = 0;
        while(pos < text.size())
        {
            size_t end = text.find(' ', pos);
            std::string word = text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
            pos = (end == std::string::npos) ? text.size() : end + 1;

            std::string candidate = line.empty() ? word : line + " " + word;
            if(TextWidth(candidate, fontSize) <= maxWidth)
            {
                line = candidate;
                continue;
            }
            if(!line.empty())
                lines.push_back(line);
            line = word;
            while(TextWidth(line, fontSize) > maxWidth && Utf8Length(line) > 1)
            {
                int n = Utf8Length(line) - 1;
                while(n > 1 && TextWidth(TruncateUtf8(line, n), fontSize) > maxWidth)
                    --n;
                std::string head = TruncateUtf8(line, n);
                lines.push_back(head);
                line = line.substr(head.size());
            }
        }
        if(!line.empty())
            lines.push_back(line);
        return lines;
    }

    std::vector<Texture> MakeLines(const std::vector<std::string> & lines, int fontSize, SDL_Renderer * renderer, Color color)
    {
        std::vector<Texture> textures;
        for(const std::string & line : lines)
            textures.emplace_back(line, fontSize, renderer, 0, 0, false, ToAbgr(color), true);
        return textures;
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
            while(!feof(pipe))
                if(fgets(buf, sizeof(buf), pipe))
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

    Texture gearIcon(OptionsLocation + UiTheme::AssetGear, renderer, UiTheme::GearX, UiTheme::GearY);
    Texture appTitleText("OptionsMenu", UiTheme::TitleFontSize, renderer, UiTheme::TitleX, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true);
    appTitleText.rect.y -= appTitleText.rect.h / 2;
    Texture appVersionText(MOD_VERSION, UiTheme::VersionFontSize, renderer, appTitleText.rect.x + appTitleText.rect.w + UiTheme::VersionGap, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true);
    appVersionText.rect.y -= appVersionText.rect.h / 2;
    Texture titleText(Translate("MODULE_UNINSTALLER"), UiTheme::SectionTitleFontSize, renderer, UiTheme::SectionTitleX, UiTheme::SectionTitleY, false, ToAbgr(UiTheme::Text), true);
    Texture creditText("created by CompCom - Modern UI by DefKorns", 16, renderer, UiTheme::CreditX, UiTheme::CreditY, false, ToAbgr(UiTheme::Text), true);

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
    const SDL_Rect noteBox{ UiTheme::DetailX, summaryBox.y + summaryBox.h + 16, UiTheme::DetailW, 72 };
    const int NoteIconCX = noteBox.x + 26;
    const int NoteTextX = noteBox.x + 46;

    Texture summaryHeader(Translate("MOD_UNINSTALLER_TO_REMOVE"), PanelHeaderFontSize, renderer, PanelInnerX, summaryBox.y + 14, false, ToAbgr(UiTheme::TextDim), true);
    Texture summaryCount;
    Texture noneMarkedText(Translate("MOD_UNINSTALL_NONE_MARKED"), PanelFontSize, renderer, PanelInnerX, SummaryListY, false, ToAbgr(UiTheme::Text), true);
    std::vector<Texture> noneHintLines = MakeLines(WrapToWidth(Translate("MOD_UNINSTALL_NONE_HINT"), PanelFontSize, PanelInnerW), PanelFontSize, renderer, UiTheme::TextDim);
    std::vector<Texture> rebootNoteLines = MakeLines(WrapToWidth(Translate("MOD_UNINSTALL_REBOOT_NOTE"), NoteFontSize, noteBox.x + noteBox.w - PanelPad - NoteTextX), NoteFontSize, renderer, UiTheme::TextDim);
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
    auto DrawFooterDivider = [&](int rightEdge)
    {
        int dividerX = rightEdge - UiTheme::BadgeDividerGapFromCluster;
        DrawVLine(renderer, dividerX, UiTheme::FooterY + 10, UiTheme::FooterY + UiTheme::FooterH - 10, UiTheme::Border, 2);
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
        titleText.Draw(renderer);
        creditText.Draw(renderer);
    };

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
            SDL_Rect rowRect{ UiTheme::ListX, UiTheme::RowFirstY + i*UiTheme::RowPitch, UiTheme::ListContentRightX - UiTheme::ListX, RowBoxH };
            int centerY = rowRect.y + rowRect.h/2;
            if(i == currentId)
            {
                DrawRoundedFillRect(renderer, rowRect, UiTheme::SelectedRowBg, UiTheme::BoxRadius);
                DrawStrokeRect(renderer, rowRect, UiTheme::Accent, 2, UiTheme::BoxRadius);
            }
            else
                DrawHLine(renderer, rowRect.x, rowRect.x + rowRect.w, rowRect.y + rowRect.h + 3, UiTheme::Border, 1);

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

        DrawStrokeRect(renderer, noteBox, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BoxRadius);
        int noteCenterY = noteBox.y + noteBox.h/2;
        DrawStrokeRect(renderer, { NoteIconCX - 9, noteCenterY - 9, 18, 18 }, UiTheme::TextDim, 2, 9);
        DrawFillRect(renderer, { NoteIconCX - 1, noteCenterY - 5, 2, 2 }, UiTheme::TextDim);
        DrawFillRect(renderer, { NoteIconCX - 1, noteCenterY - 1, 2, 6 }, UiTheme::TextDim);
        const int noteLinePitch = 20;
        int y = noteCenterY - (static_cast<int>(rebootNoteLines.size()) * noteLinePitch)/2;
        for(Texture & line : rebootNoteLines)
        {
            line.rect.x = NoteTextX;
            line.rect.y = y + (noteLinePitch - line.rect.h)/2;
            line.Draw(renderer);
            y += noteLinePitch;
        }
    };

    auto DrawScreen = [&]()
    {
        DrawChrome();
        DrawList();
        DrawSummary();

        int rightEdge = UiTheme::BadgeClusterRightX;
        if(!mods.empty())
            rightEdge = DrawBadge(mods[listOffset + currentId].marked ? badgeUnmark : badgeMark, rightEdge);
        rightEdge = DrawBadge(badgeBack, rightEdge);
        if(markedCount > 0)
            rightEdge = DrawBadge(uninstallPill, rightEdge);
        DrawFooterDivider(rightEdge);
    };

    auto render = [&]()
    {
        sdl_context.StartFrame();
        DrawScreen();
        SetDrawColor(renderer, bg);
        sdl_context.EndFrame();
    };

    auto ConfirmUninstall = [&]() -> bool
    {
        auto marked = MarkedMods();
        const int maxNames = 8;
        const int namePitch = 30;
        const int shown = std::min(maxNames, static_cast<int>(marked.size()));
        const bool overflow = static_cast<int>(marked.size()) > shown;
        const int listRows = shown + (overflow ? 1 : 0);

        const int boxW = 560;
        const int badgeRowH = Dialog::BadgeRowGap + UiTheme::BadgeOuterSize;
        const int boxH = 150 + listRows*namePitch + badgeRowH;
        const SDL_Rect box = Dialog::PanelRect(boxW, boxH);

        Texture confirmTitle(Translate("MOD_UNINSTALL_CONFIRM"), 24, renderer, 640, box.y + 36, true, ToAbgr(UiTheme::Text), true);
        std::vector<Texture> names;
        int nameW = 0;
        for(int i = 0; i < shown; ++i)
        {
            names.emplace_back(TruncateToWidth(marked[i]->name, PanelFontSize, boxW - 80), PanelFontSize, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true);
            nameW = std::max(nameW, names.back().rect.w);
        }
        if(overflow)
            names.emplace_back("+" + std::to_string(marked.size() - shown), PanelFontSize, renderer, 0, 0, false, ToAbgr(UiTheme::TextDim), true);
        const int listX = 640 - (nameW + BulletTextGap)/2;
        std::vector<Texture> noteLines = MakeLines(WrapToWidth(Translate("MOD_UNINSTALL_REBOOT_NOTE"), NoteFontSize, boxW - 48), NoteFontSize, renderer, UiTheme::TextDim);

        controller.GetButtonStatus(B);
        bool confirmed = false;
        for(;;)
        {
            SDL_Event e;
            while(SDL_PollEvent(&e))
                if(e.type == SDL_QUIT)
                    return false;
            controller.Update();
            if(sdl_context.powerwatch->buttonPress() || controller.GetButtonStatus(B))
                break;
            if(controller.PeekButtonStatus(START) && controller.PeekButtonStatus(SELECT))
            {
                confirmed = true;
                break;
            }

            sdl_context.StartFrame();
            DrawScreen();
            Dialog::DrawBackdrop(renderer);
            Dialog::DrawPanel(renderer, box);
            confirmTitle.Draw(renderer);
            int y = box.y + 78;
            for(int i = 0; i < static_cast<int>(names.size()); ++i)
            {
                if(i < shown)
                    DrawDot(renderer, listX, y);
                names[i].rect.x = listX + BulletTextGap;
                names[i].rect.y = y - names[i].rect.h/2;
                names[i].Draw(renderer);
                y += namePitch;
            }
            int noteY = box.y + box.h - badgeRowH - 34 - (static_cast<int>(noteLines.size()) - 1) * 10;
            for(Texture & line : noteLines)
            {
                line.rect.x = 640 - line.rect.w/2;
                line.rect.y = noteY - line.rect.h/2;
                line.Draw(renderer);
                noteY += 20;
            }

            badges.DrawCentered({ &badgeCancel, &confirmPill }, box.x + box.w / 2, box.y + box.h - Dialog::BottomPad - UiTheme::BadgeOuterSize);

            SetDrawColor(renderer, bg);
            sdl_context.EndFrame();
        }
        if(!confirmed)
            return false;

        std::ofstream out("/tmp/uninstall");
        if(!out.is_open())
            return false;
        for(const Mod * mod : marked)
            out << mod->name << " ";
        out.close();
        system(("/bin/sh " + folderLocation + "/FinishUninstall.sh &").c_str());
        return true;
    };

    for(;;)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
            if(e.type == SDL_QUIT)
                return ExitLeaveMenu;

        controller.Update();
        if(sdl_context.powerwatch->buttonPress())
            return ExitLeaveMenu;

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
            if(ConfirmUninstall())
                return ExitUninstallStarted;
        }

        render();
    }
}
