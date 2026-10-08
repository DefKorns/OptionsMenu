/**
  * Copyright (C) 2026 DefKorns
  * Based on the Module Uninstaller screen by CompCom.
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
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace
{
    const std::string OptionsLocation = "/etc/options_menu/";
    const std::string ScriptLocation = OptionsLocation + "scripts/";
    const std::string SelectionFile = "/tmp/save_manager_selection";
    const std::string RetroArchPrefix = "retroarch/";

    // narrower list than the shared theme: entries are short codes, the detail panels need the room
    const int ListW = 600;
    const int ListRightX = UiTheme::ListX + ListW - 40;
    const int ControlRightX = ListRightX - 16;
    const int DetailX = UiTheme::ListX + ListW + 16;
    const int DetailW = UiTheme::FrameX + UiTheme::FrameW - 16 - DetailX;

    const int ListFontSize = 16;
    const int PanelFontSize = 16;
    const int PanelHeaderFontSize = 18;
    const int NoteFontSize = 15;

    const int ExitBack = 0;
    const int ExitLeaveMenu = 1;

    const Color White{ 255, 255, 255 };

    enum class Action { Backup, RestoreUsb, Clean, RestoreDeleted };

    struct ActionInfo
    {
        Action action;
        std::string titleKey;
        std::string descriptionKey;
    };

    const std::vector<ActionInfo> Actions{
        { Action::Backup, "BACKUP_SAVES_USB", "SAVE_MANAGER_BACKUP_DESC" },
        { Action::RestoreUsb, "RESTORE_SAVES_USB", "SAVE_MANAGER_RESTORE_DESC" },
        { Action::Clean, "SAVE_MANAGER_CLEAN", "SAVE_MANAGER_CLEAN_DESC" },
        { Action::RestoreDeleted, "SAVE_MANAGER_RESTORE_DELETED", "SAVE_MANAGER_RESTORE_DELETED_DESC" },
    };

    struct SaveEntry
    {
        std::string code;
        Texture text;
        bool marked = false;
    };

    std::string TrimTrailingNewline(std::string s)
    {
        while(!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        return s;
    }

    std::vector<std::string> ReadLines(const std::string & command)
    {
        std::vector<std::string> lines;
        FILE * pipe = popen(command.c_str(), "r");
        if(!pipe)
            return lines;
        char buf[256];
        while(fgets(buf, sizeof(buf), pipe))
        {
            std::string line = TrimTrailingNewline(buf);
            if(!line.empty())
                lines.push_back(line);
        }
        pclose(pipe);
        return lines;
    }

    std::string OrphanSavesCommand(const std::string & args)
    {
        return "sh " + ScriptLocation + "OrphanSaves.sh " + args;
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

    std::string EntryLabel(const std::string & entry)
    {
        if(entry.compare(0, RetroArchPrefix.size(), RetroArchPrefix) == 0)
            return "RetroArch: " + entry.substr(RetroArchPrefix.size());
        return entry;
    }

    void DrawDot(SDL_Renderer * renderer, int x, int centerY)
    {
        DrawRoundedFillRect(renderer, { x, centerY - 4, 8, 8 }, UiTheme::Accent, 4);
    }

    void DrawLinesCentered(SDL_Renderer * renderer, std::vector<Texture> & lines, int centerX, int y, int pitch)
    {
        for(Texture & line : lines)
        {
            line.rect.x = centerX - line.rect.w/2;
            line.rect.y = y;
            line.Draw(renderer);
            y += pitch;
        }
    }
}

int main(int, char **)
{
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
    Texture creditText("Save Manager - by DefKorns", 16, renderer, UiTheme::CreditX, UiTheme::CreditY, false, ToAbgr(UiTheme::Text), true);
    Texture titleText;
    auto SetTitle = [&](const std::string & key)
    {
        titleText = Texture(Translate(key), UiTheme::SectionTitleFontSize, renderer, UiTheme::SectionTitleX, UiTheme::SectionTitleY, false, ToAbgr(UiTheme::Text), true);
    };

    const int RowBoxH = UiTheme::RowPitch - 6;
    Texture checkboxOn(OptionsLocation + UiTheme::AssetCheckboxOn, renderer);
    SetColorMod(checkboxOn.texture.get(), UiTheme::Accent);
    Texture checkboxOff(OptionsLocation + UiTheme::AssetCheckboxOff, renderer);
    SetColorMod(checkboxOff.texture.get(), UiTheme::CheckboxOff);
    const int CheckboxSize = 20;
    const int CheckboxX = ControlRightX - CheckboxSize;
    const int RowTextMaxW = CheckboxX - 16 - UiTheme::RowTextX;
    const int displayCount = std::max(1, (UiTheme::FooterDividerY - UiTheme::ListBottomMargin - UiTheme::RowFirstY) / UiTheme::RowPitch);

    Texture scrollUp(OptionsLocation + UiTheme::AssetChevronUp, renderer, UiTheme::ScrollX, UiTheme::ScrollUpY);
    SetColorMod(scrollUp.texture.get(), UiTheme::ScrollArrow);
    Texture scrollDown = scrollUp;
    scrollDown.rect.y = UiTheme::ScrollDownY;

    const int PanelPad = 16;
    const int PanelInnerX = DetailX + PanelPad;
    const int PanelInnerW = DetailW - 2*PanelPad;
    const SDL_Rect summaryBox{ DetailX, UiTheme::RowFirstY, DetailW, 260 };
    const int SummaryDividerY = summaryBox.y + 46;
    const int SummaryListY = summaryBox.y + 62;
    const int SummaryRowPitch = 28;
    const int SummaryMaxRows = (summaryBox.y + summaryBox.h - PanelPad - SummaryListY) / SummaryRowPitch;
    const int BulletTextGap = 18;
    const SDL_Rect noteBox{ DetailX, summaryBox.y + summaryBox.h + 16, DetailW, 72 };
    const int NoteIconCX = noteBox.x + 26;
    const int StatPitch = 26;
    const int StatsBoxH = 4*StatPitch + 2*PanelPad - 6;
    const int DetailBottomY = UiTheme::FooterDividerY - UiTheme::ListBottomMargin;
    const SDL_Rect statsBox{ DetailX, DetailBottomY - StatsBoxH, DetailW, StatsBoxH };
    const SDL_Rect descriptionBox{ DetailX, UiTheme::RowFirstY, DetailW, statsBox.y - 16 - UiTheme::RowFirstY };
    const int NoteTextX = noteBox.x + 46;

    BadgePainter badges(OptionsLocation, renderer);
    auto MakeBadge = [&](const std::string & letter, const std::string & labelKey, Color rim, Color fill) -> Badge
    {
        return badges.Make(letter, Translate(labelKey), rim, fill);
    };
    Badge badgeSelect = MakeBadge("A", "HINT_SELECT", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge badgeMark = MakeBadge("A", "HINT_MARK", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge badgeUnmark = MakeBadge("A", "HINT_UNMARK", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge badgeOk = MakeBadge("A", "HINT_OK", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge badgeBack = MakeBadge("B", "HINT_BACK", UiTheme::BadgeBDark, UiTheme::BadgeB);
    Badge badgeCancel = MakeBadge("B", "HINT_CANCEL", UiTheme::BadgeBDark, UiTheme::BadgeB);
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

    auto DrawRowFrame = [&](int index, bool selected) -> SDL_Rect
    {
        SDL_Rect rowRect{ UiTheme::ListX, UiTheme::RowFirstY + index*UiTheme::RowPitch, ListRightX - UiTheme::ListX, RowBoxH };
        if(selected)
        {
            DrawRoundedFillRect(renderer, rowRect, UiTheme::SelectedRowBg, UiTheme::BoxRadius);
            DrawStrokeRect(renderer, rowRect, UiTheme::Accent, 2, UiTheme::BoxRadius);
        }
        else
            DrawHLine(renderer, rowRect.x, rowRect.x + rowRect.w, rowRect.y + rowRect.h + 3, UiTheme::Border, 1);
        return rowRect;
    };

    auto DrawNote = [&](std::vector<Texture> & lines)
    {
        DrawStrokeRect(renderer, noteBox, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BoxRadius);
        int noteCenterY = noteBox.y + noteBox.h/2;
        DrawStrokeRect(renderer, { NoteIconCX - 9, noteCenterY - 9, 18, 18 }, UiTheme::TextDim, 2, 9);
        DrawFillRect(renderer, { NoteIconCX - 1, noteCenterY - 5, 2, 2 }, UiTheme::TextDim);
        DrawFillRect(renderer, { NoteIconCX - 1, noteCenterY - 1, 2, 6 }, UiTheme::TextDim);
        const int noteLinePitch = 20;
        int y = noteCenterY - (static_cast<int>(lines.size()) * noteLinePitch)/2;
        for(Texture & line : lines)
        {
            line.rect.x = NoteTextX;
            line.rect.y = y + (noteLinePitch - line.rect.h)/2;
            line.Draw(renderer);
            y += noteLinePitch;
        }
    };

    auto NoteLines = [&](const std::string & key)
    {
        return MakeLines(WrapToWidth(Translate(key), NoteFontSize, noteBox.x + noteBox.w - PanelPad - NoteTextX), NoteFontSize, renderer, UiTheme::TextDim);
    };

    // Modal panel over the current screen: confirm (Start + Select) or acknowledge (A); B always cancels.
    auto RunDialog = [&](const std::function<void()> & drawUnder, const std::string & titleKey, const std::vector<std::string> & bodyLines,
                         const std::string & noteKey, bool confirm) -> bool
    {
        const int boxW = 600;
        const int bodyPitch = 30;
        const int badgeRowH = Dialog::BadgeRowGap + UiTheme::BadgeOuterSize;
        std::vector<Texture> body;
        for(const std::string & line : bodyLines)
            body.emplace_back(TruncateToWidth(line, PanelFontSize, boxW - 80), PanelFontSize, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true);
        std::vector<Texture> noteLines = noteKey.empty() ? std::vector<Texture>()
            : MakeLines(WrapToWidth(Translate(noteKey), NoteFontSize, boxW - 48), NoteFontSize, renderer, UiTheme::TextDim);
        const int boxH = 110 + static_cast<int>(body.size())*bodyPitch + static_cast<int>(noteLines.size())*20 + badgeRowH;
        const SDL_Rect box = Dialog::PanelRect(boxW, boxH);
        Texture title(Translate(titleKey), Dialog::TitleFontSize, renderer, 640, box.y + 36, true, ToAbgr(UiTheme::Text), true);

        controller.GetButtonStatus(A);
        controller.GetButtonStatus(B);
        for(;;)
        {
            SDL_Event e;
            while(SDL_PollEvent(&e))
                if(e.type == SDL_QUIT)
                    return false;
            controller.Update();
            if(sdl_context.powerwatch->buttonPress() || controller.GetButtonStatus(B))
                return false;
            if(confirm && controller.PeekButtonStatus(START) && controller.PeekButtonStatus(SELECT))
                return true;
            if(!confirm && controller.GetButtonStatus(A))
                return true;

            sdl_context.StartFrame();
            drawUnder();
            Dialog::DrawBackdrop(renderer);
            Dialog::DrawPanel(renderer, box);
            title.Draw(renderer);
            int y = box.y + 72;
            for(Texture & line : body)
            {
                line.rect.x = 640 - line.rect.w/2;
                line.rect.y = y;
                line.Draw(renderer);
                y += bodyPitch;
            }
            DrawLinesCentered(renderer, noteLines, 640, y + 6, 20);
            if(confirm)
                badges.DrawCentered({ &badgeCancel, &confirmPill }, box.x + box.w / 2, box.y + box.h - Dialog::BottomPad - UiTheme::BadgeOuterSize);
            else
                badges.DrawCentered({ &badgeOk }, box.x + box.w / 2, box.y + box.h - Dialog::BottomPad - UiTheme::BadgeOuterSize);
            SetDrawColor(renderer, bg);
            sdl_context.EndFrame();
        }
    };

    // Shows a "working" panel for one frame, then runs the blocking command.
    auto RunBusy = [&](const std::function<void()> & drawUnder, const std::string & command)
    {
        const SDL_Rect box = Dialog::PanelRect(420, 110);
        Texture working(Translate("SAVE_MANAGER_WORKING"), Dialog::TitleFontSize, renderer, 640, box.y + box.h/2 - 12, true, ToAbgr(UiTheme::Text), true);
        sdl_context.StartFrame();
        drawUnder();
        Dialog::DrawBackdrop(renderer);
        Dialog::DrawPanel(renderer, box);
        working.Draw(renderer);
        SetDrawColor(renderer, bg);
        sdl_context.EndFrame();
        system(command.c_str());
        controller.Update();
        controller.GetButtonStatus(A);
        controller.GetButtonStatus(B);
    };

    // Marked-list screen shared by "clean" (orphans, all pre-marked) and "restore deleted" (none marked).
    auto RunSaveList = [&](bool cleaning) -> int
    {
        SetTitle(cleaning ? "SAVE_MANAGER_CLEAN" : "SAVE_MANAGER_RESTORE_DELETED");
        Texture summaryHeader(Translate(cleaning ? "SAVE_MANAGER_TO_MOVE" : "SAVE_MANAGER_TO_RESTORE"), PanelHeaderFontSize, renderer, PanelInnerX, summaryBox.y + 14, false, ToAbgr(UiTheme::TextDim), true);
        Texture noneMarkedText(Translate("SAVE_MANAGER_NONE_MARKED"), PanelFontSize, renderer, PanelInnerX, SummaryListY, false, ToAbgr(UiTheme::Text), true);
        std::vector<Texture> noneHintLines = MakeLines(WrapToWidth(Translate("SAVE_MANAGER_NONE_HINT"), PanelFontSize, PanelInnerW), PanelFontSize, renderer, UiTheme::TextDim);
        std::vector<Texture> noteLines = NoteLines(cleaning ? "SAVE_MANAGER_CLEAN_NOTE" : "SAVE_MANAGER_RESTORE_NOTE");
        Badge actionPill = badges.MakePill("Start", "");

        std::vector<SaveEntry> entries;
        int listOffset = 0;
        int currentId = 0;
        int markedCount = 0;
        Texture summaryCount;
        Texture moreText;
        std::vector<Texture> summaryNames;

        const int selectAllRows = 1;
        Texture selectAllText(Translate("SAVE_MANAGER_SELECT_ALL"), ListFontSize, renderer, UiTheme::RowTextX, 0, false, ToAbgr(White), true);

        auto Marked = [&]()
        {
            std::vector<const SaveEntry *> marked;
            for(const SaveEntry & entry : entries)
                if(entry.marked)
                    marked.push_back(&entry);
            return marked;
        };

        auto RefreshSummary = [&]()
        {
            auto marked = Marked();
            const int count = static_cast<int>(marked.size());
            markedCount = count;
            summaryCount = Texture(std::to_string(count), PanelHeaderFontSize, renderer, 0, summaryBox.y + 14, false, ToAbgr(count ? UiTheme::Accent : UiTheme::TextDim), true);
            summaryCount.rect.x = summaryBox.x + summaryBox.w - PanelPad - summaryCount.rect.w;
            actionPill.label = Texture(Translate(cleaning ? "HINT_MOVE" : "HINT_RESTORE") + " (" + std::to_string(count) + ")", 16, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true);

            int shown = (count > SummaryMaxRows) ? SummaryMaxRows - 1 : count;
            summaryNames.clear();
            for(int i = 0; i < shown; ++i)
                summaryNames.emplace_back(TruncateToWidth(EntryLabel(marked[i]->code), PanelFontSize, PanelInnerW - BulletTextGap), PanelFontSize, renderer, PanelInnerX + BulletTextGap, 0, false, ToAbgr(UiTheme::Text), true);
            moreText = (shown < count)
                ? Texture("+" + std::to_string(count - shown), PanelFontSize, renderer, PanelInnerX + BulletTextGap, 0, false, ToAbgr(UiTheme::TextDim), true)
                : Texture();
        };

        auto Load = [&]()
        {
            entries.clear();
            for(const std::string & code : ReadLines(OrphanSavesCommand(cleaning ? "orphans" : "deleted")))
            {
                SaveEntry entry;
                entry.code = code;
                entry.text = Texture(TruncateToWidth(EntryLabel(code), ListFontSize, RowTextMaxW), ListFontSize, renderer, UiTheme::RowTextX, 0, false, ToAbgr(White), true);
                entry.marked = cleaning;
                entries.push_back(entry);
            }
            listOffset = 0;
            currentId = 0;
            RefreshSummary();
        };

        auto DrawScreen = [&]()
        {
            DrawChrome();
            const int entryCount = static_cast<int>(entries.size());
            const int rowCount = entryCount + selectAllRows;
            const int count = std::min(displayCount, rowCount - listOffset);
            for(int i = 0; i < count; ++i)
            {
                const int row = listOffset + i;
                SDL_Rect rowRect = DrawRowFrame(i, i == currentId);
                int centerY = rowRect.y + rowRect.h/2;
                Texture & text = row == 0 ? selectAllText : entries[row - selectAllRows].text;
                const bool marked = row == 0 ? markedCount == entryCount : entries[row - selectAllRows].marked;
                text.rect.y = centerY - text.rect.h/2;
                SetColorMod(text.texture.get(), row != 0 && marked ? UiTheme::TextDim : UiTheme::Text);
                text.Draw(renderer);
                Texture & checkbox = marked ? checkboxOn : checkboxOff;
                checkbox.rect.x = CheckboxX;
                checkbox.rect.y = centerY - checkbox.rect.h/2;
                checkbox.Draw(renderer);
            }
            if(listOffset != 0)
                scrollUp.Draw(renderer);
            if(listOffset + displayCount < rowCount)
                scrollDown.Draw(renderer, SDL_FLIP_VERTICAL);

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
            DrawNote(noteLines);

            int rightEdge = UiTheme::BadgeClusterRightX;
            const int row = listOffset + currentId;
            const bool rowMarked = row == 0 ? markedCount == entryCount : entries[row - selectAllRows].marked;
            rightEdge = DrawBadge(rowMarked ? badgeUnmark : badgeMark, rightEdge);
            rightEdge = DrawBadge(badgeBack, rightEdge);
            if(markedCount > 0)
                rightEdge = DrawBadge(actionPill, rightEdge);
            DrawFooterDivider(rightEdge);
        };

        Load();
        for(;;)
        {
            SDL_Event e;
            while(SDL_PollEvent(&e))
                if(e.type == SDL_QUIT)
                    return ExitLeaveMenu;

            controller.Update();
            if(sdl_context.powerwatch->buttonPress())
                return ExitLeaveMenu;

            const int entryCount = static_cast<int>(entries.size());
            const int rowCount = entryCount + selectAllRows;
            if(controller.HeldRepeat(UP))
            {
                if(currentId > 0)
                    --currentId;
                else if(listOffset > 0)
                    --listOffset;
            }
            else if(controller.HeldRepeat(DOWN))
            {
                if(currentId < displayCount-1 && listOffset+currentId+1 < rowCount)
                    ++currentId;
                else if(listOffset+displayCount < rowCount)
                    ++listOffset;
            }
            else if(controller.GetButtonStatus(A) && entryCount > 0)
            {
                const int row = listOffset + currentId;
                if(row == 0)
                {
                    const bool markAll = markedCount != entryCount;
                    for(SaveEntry & entry : entries)
                        entry.marked = markAll;
                }
                else
                    entries[row - selectAllRows].marked = !entries[row - selectAllRows].marked;
                RefreshSummary();
            }
            else if(controller.GetButtonStatus(B))
                return ExitBack;
            else if(controller.GetButtonStatus(START) && markedCount > 0)
            {
                auto marked = Marked();
                std::vector<std::string> lines;
                const int maxNames = 6;
                for(int i = 0; i < std::min(maxNames, static_cast<int>(marked.size())); ++i)
                    lines.push_back(EntryLabel(marked[i]->code));
                if(static_cast<int>(marked.size()) > maxNames)
                    lines.push_back("+" + std::to_string(marked.size() - maxNames));
                if(RunDialog(DrawScreen, cleaning ? "SAVE_MANAGER_CLEAN_CONFIRM" : "SAVE_MANAGER_RESTORE_DELETED_CONFIRM", lines,
                             cleaning ? "SAVE_MANAGER_CLEAN_NOTE" : "SAVE_MANAGER_RESTORE_NOTE", true))
                {
                    std::ofstream selection(SelectionFile);
                    for(const SaveEntry * entry : marked)
                        selection << entry->code << "\n";
                    selection.close();
                    RunBusy(DrawScreen, OrphanSavesCommand((cleaning ? "move " : "restore ") + SelectionFile));
                    std::remove(SelectionFile.c_str());
                    Load();
                }
            }

            sdl_context.StartFrame();
            DrawScreen();
            SetDrawColor(renderer, bg);
            sdl_context.EndFrame();
        }
    };

    struct Stats
    {
        int saves = 0;
        int orphans = 0;
        int deleted = 0;
        int backupSaves = -1;
    } stats;

    std::vector<Texture> actionTexts;
    for(const ActionInfo & info : Actions)
        actionTexts.emplace_back(Translate(info.titleKey), ListFontSize, renderer, UiTheme::RowTextX, 0, false, ToAbgr(White), true);
    std::vector<Texture> actionCounts(Actions.size());
    std::vector<Texture> statLabels;
    std::vector<Texture> statValues;
    std::vector<Texture> descriptionLines;
    Texture detailHeader;
    int currentAction = 0;

    auto RefreshStats = [&]()
    {
        const std::vector<std::string> values = ReadLines(OrphanSavesCommand("stats"));
        auto Value = [&](size_t i) { return i < values.size() ? std::atoi(values[i].c_str()) : 0; };
        stats.saves = Value(0);
        stats.backupSaves = values.size() > 1 ? Value(1) : -1;
        stats.orphans = Value(2);
        stats.deleted = Value(3);

        const int counts[] = { -1, -1, stats.orphans, stats.deleted };
        for(size_t i = 0; i < Actions.size(); ++i)
            actionCounts[i] = counts[i] < 0 ? Texture()
                : Texture(std::to_string(counts[i]), ListFontSize, renderer, 0, 0, false, ToAbgr(counts[i] ? UiTheme::Accent : UiTheme::TextDim), true);

        const std::vector<std::pair<std::string, std::string>> rows{
            { "SAVE_MANAGER_STAT_SAVES", std::to_string(stats.saves) },
            { "SAVE_MANAGER_STAT_BACKUP", stats.backupSaves >= 0 ? std::to_string(stats.backupSaves) : Translate("SAVE_MANAGER_STAT_NONE") },
            { "SAVE_MANAGER_STAT_ORPHANS", std::to_string(stats.orphans) },
            { "SAVE_MANAGER_STAT_KEPT", std::to_string(stats.deleted) },
        };
        statLabels.clear();
        statValues.clear();
        for(const auto & row : rows)
        {
            statValues.emplace_back(row.second, PanelFontSize, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true);
            const int labelMaxW = PanelInnerW - statValues.back().rect.w - 12;
            statLabels.emplace_back(TruncateToWidth(Translate(row.first), PanelFontSize, labelMaxW), PanelFontSize, renderer, PanelInnerX, 0, false, ToAbgr(UiTheme::TextDim), true);
        }
    };

    auto RefreshDetail = [&]()
    {
        const ActionInfo & info = Actions[currentAction];
        detailHeader = Texture(TruncateToWidth(Translate(info.titleKey), PanelHeaderFontSize, PanelInnerW), PanelHeaderFontSize, renderer, PanelInnerX, summaryBox.y + 14, false, ToAbgr(UiTheme::TextDim), true);
        descriptionLines = MakeLines(WrapToWidth(Translate(info.descriptionKey), PanelFontSize, PanelInnerW), PanelFontSize, renderer, UiTheme::Text);
    };

    auto DrawActions = [&]()
    {
        DrawChrome();
        for(int i = 0; i < static_cast<int>(Actions.size()); ++i)
        {
            SDL_Rect rowRect = DrawRowFrame(i, i == currentAction);
            int centerY = rowRect.y + rowRect.h/2;
            actionTexts[i].rect.y = centerY - actionTexts[i].rect.h/2;
            actionTexts[i].Draw(renderer);
            if(actionCounts[i].texture)
            {
                actionCounts[i].rect.x = ControlRightX - actionCounts[i].rect.w;
                actionCounts[i].rect.y = centerY - actionCounts[i].rect.h/2;
                actionCounts[i].Draw(renderer);
            }
        }

        DrawStrokeRect(renderer, descriptionBox, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BoxRadius);
        detailHeader.Draw(renderer);
        DrawHLine(renderer, PanelInnerX, PanelInnerX + PanelInnerW, SummaryDividerY, UiTheme::Border, 1);
        int y = SummaryListY;
        for(Texture & line : descriptionLines)
        {
            if(y + line.rect.h > descriptionBox.y + descriptionBox.h - 6)
                break;
            line.rect.x = PanelInnerX;
            line.rect.y = y;
            line.Draw(renderer);
            y += 22;
        }

        DrawStrokeRect(renderer, statsBox, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BoxRadius);
        for(size_t i = 0; i < statLabels.size(); ++i)
        {
            const int rowY = statsBox.y + PanelPad - 2 + static_cast<int>(i) * StatPitch;
            statLabels[i].rect.y = rowY;
            statLabels[i].Draw(renderer);
            statValues[i].rect.x = PanelInnerX + PanelInnerW - statValues[i].rect.w;
            statValues[i].rect.y = rowY;
            statValues[i].Draw(renderer);
        }

        int rightEdge = UiTheme::BadgeClusterRightX;
        rightEdge = DrawBadge(badgeSelect, rightEdge);
        rightEdge = DrawBadge(badgeBack, rightEdge);
        DrawFooterDivider(rightEdge);
    };

    auto ShowMessage = [&](const std::string & titleKey, const std::string & noteKey)
    {
        RunDialog(DrawActions, titleKey, {}, noteKey, false);
    };

    auto RunAction = [&](Action action) -> int
    {
        switch(action)
        {
        case Action::Backup:
            if(RunDialog(DrawActions, "SAVE_MANAGER_BACKUP_CONFIRM", {}, "SAVE_MANAGER_BACKUP_DESC", true))
            {
                RunBusy(DrawActions, "sh " + ScriptLocation + "BackupSavesUsb.sh >/dev/null 2>&1");
                RefreshStats();
                ShowMessage("BACKUP_SAVES_USB_DONE", "");
            }
            return -1;
        case Action::RestoreUsb:
            if(stats.backupSaves < 0)
            {
                ShowMessage("RESTORE_SAVES_USB_NOT_FOUND", "");
                return -1;
            }
            if(RunDialog(DrawActions, "SAVE_MANAGER_RESTORE_CONFIRM", { Translate("SAVE_MANAGER_STAT_BACKUP") + ": " + std::to_string(stats.backupSaves) }, "SAVE_MANAGER_RESTORE_DESC", true))
            {
                RunBusy(DrawActions, "sh " + ScriptLocation + "RestoreSavesUsb.sh >/dev/null 2>&1");
                RefreshStats();
                ShowMessage("RESTORE_SAVES_USB_DONE", "");
            }
            return -1;
        case Action::Clean:
        case Action::RestoreDeleted:
        {
            const bool cleaning = action == Action::Clean;
            if((cleaning ? stats.orphans : stats.deleted) == 0)
            {
                ShowMessage(cleaning ? "SAVE_MANAGER_NO_ORPHANS" : "SAVE_MANAGER_NO_DELETED", "");
                return -1;
            }
            const int result = RunSaveList(cleaning);
            SetTitle("SAVE_MANAGER");
            RefreshStats();
            return result == ExitLeaveMenu ? ExitLeaveMenu : -1;
        }
        }
        return -1;
    };

    SetTitle("SAVE_MANAGER");
    RefreshStats();
    RefreshDetail();
    for(;;)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
            if(e.type == SDL_QUIT)
                return ExitLeaveMenu;

        controller.Update();
        if(sdl_context.powerwatch->buttonPress())
            return ExitLeaveMenu;

        if(controller.HeldRepeat(UP) && currentAction > 0)
        {
            --currentAction;
            RefreshDetail();
        }
        else if(controller.HeldRepeat(DOWN) && currentAction + 1 < static_cast<int>(Actions.size()))
        {
            ++currentAction;
            RefreshDetail();
        }
        else if(controller.GetButtonStatus(A))
        {
            if(RunAction(Actions[currentAction].action) == ExitLeaveMenu)
                return ExitLeaveMenu;
        }
        else if(controller.GetButtonStatus(B))
            return ExitBack;

        sdl_context.StartFrame();
        DrawActions();
        SetDrawColor(renderer, bg);
        sdl_context.EndFrame();
    }
}
