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
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <sys/wait.h>

namespace
{
    const std::string OptionsLocation = "/etc/options_menu/";
    const std::string ScriptLocation = OptionsLocation + "scripts/";
    const std::string SelectionFile = "/tmp/save_manager_selection";
    const std::string RetroArchPrefix = "retroarch/";

    // narrower than the shared theme to fit the detail panels
    const int ListW = 600;
    const int ListRightX = UiTheme::ListX + ListW - 40;
    const int ControlRightX = ListRightX - 16;
    const int DetailX = UiTheme::ListX + ListW + 16;
    const int DetailW = UiTheme::FrameX + UiTheme::FrameW - 16 - DetailX;

    const int ListFontSize = 16;
    const int PanelFontSize = 16;
    const int PanelHeaderFontSize = 18;

    const int ExitBack = 0;
    const int ExitLeaveMenu = 1;

    const Color White{ 255, 255, 255 };

    enum class Action { Backup, RestoreUsb, Clean, RestoreDeleted };

    struct ListInfo
    {
        std::string titleKey;
        std::string headerKey;
        std::string noteKey;
        std::string confirmKey;
        std::string actionHintKey;
        std::string listArgs;
        std::string actionArgs;
        bool preMarked;
        bool destructive;
    };

    const ListInfo CleanList{ "SAVE_MANAGER_CLEAN", "SAVE_MANAGER_TO_MOVE", "SAVE_MANAGER_CLEAN_NOTE", "SAVE_MANAGER_CLEAN_CONFIRM", "HINT_MOVE", "orphans", "move ", true, true };
    const ListInfo RestoreDeletedList{ "SAVE_MANAGER_RESTORE_DELETED", "SAVE_MANAGER_TO_RESTORE", "SAVE_MANAGER_RESTORE_NOTE", "SAVE_MANAGER_RESTORE_DELETED_CONFIRM", "HINT_RESTORE", "deleted", "restore ", false, false };
    const ListInfo RestoreBackupList{ "RESTORE_SAVES_USB", "SAVE_MANAGER_TO_RESTORE", "SAVE_MANAGER_RESTORE_USB_NOTE", "SAVE_MANAGER_RESTORE_CONFIRM", "HINT_RESTORE", "backup", "restore-backup ", false, true };

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

    using GameNames = std::map<std::string, std::string>;

    struct SaveEntry
    {
        std::string code;
        std::string label;
        bool named = false;
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
        char buf[1024];
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

    // entry<TAB>name lines from the name cache
    GameNames LoadGameNames()
    {
        GameNames names;
        for(const std::string & line : ReadLines(OrphanSavesCommand("cached-names")))
        {
            const size_t tab = line.find('\t');
            if(tab == std::string::npos || tab == 0 || tab + 1 == line.size())
                continue;
            names.emplace(line.substr(0, tab), line.substr(tab + 1));
        }
        return names;
    }

    // the entry stays the identifier, a known name only changes what is shown
    std::string EntryLabel(const std::string & entry, const GameNames & names)
    {
        const auto found = names.find(entry);
        const bool retroArch = entry.compare(0, RetroArchPrefix.size(), RetroArchPrefix) == 0;
        const std::string shown = found != names.end() ? found->second : retroArch ? entry.substr(RetroArchPrefix.size()) : entry;
        return retroArch ? "RetroArch: " + shown : shown;
    }

    void DrawDot(SDL_Renderer * renderer, int x, int centerY)
    {
        DrawRoundedFillRect(renderer, { x, centerY - 4, 8, 8 }, UiTheme::Accent, 4);
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

    ScreenChrome chrome(OptionsLocation, renderer, "OptionsMenu", MOD_VERSION, "Save Manager - by DefKorns");
    auto SetTitle = [&](const std::string & key)
    {
        chrome.SetSectionTitle(Translate(key));
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

    ScrollArrows scrollArrows(OptionsLocation, renderer);

    const int PanelPad = 16;
    const int PanelInnerX = DetailX + PanelPad;
    const int PanelInnerW = DetailW - 2*PanelPad;
    const SDL_Rect summaryBox{ DetailX, UiTheme::RowFirstY, DetailW, 260 };
    const int SummaryDividerY = summaryBox.y + 46;
    const int SummaryListY = summaryBox.y + 62;
    const int SummaryRowPitch = 28;
    const int SummaryMaxRows = (summaryBox.y + summaryBox.h - PanelPad - SummaryListY) / SummaryRowPitch;
    const int BulletTextGap = 18;
    NoteBox note(renderer, { DetailX, summaryBox.y + summaryBox.h + 16, DetailW, 72 });
    const int StatPitch = 26;
    const int StatsBoxH = 4*StatPitch + 2*PanelPad - 6;
    const int DetailBottomY = UiTheme::FooterDividerY - UiTheme::ListBottomMargin;
    const SDL_Rect statsBox{ DetailX, DetailBottomY - StatsBoxH, DetailW, StatsBoxH };
    const SDL_Rect descriptionBox{ DetailX, UiTheme::RowFirstY, DetailW, statsBox.y - 16 - UiTheme::RowFirstY };

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
    Badge badgeConfirm = MakeBadge("A", "HINT_CONFIRM", UiTheme::BadgeADark, UiTheme::BadgeA);
    Badge confirmPill = badges.MakePill("Start + Select", Translate("HINT_CONFIRM"));
    auto DrawBadge = [&](Badge & badge, int rightEdgeX) -> int
    {
        return badges.Draw(badge, rightEdgeX, UiTheme::BadgeBandY);
    };
    auto Confirm = [&](const std::function<void()> & drawUnder, const PanelDialogText & text, bool destructive)
    {
        if(destructive)
            return PanelDialog(renderer, badges, PanelDialog::Kind::ConfirmDestructive, text, { &badgeCancel, &confirmPill }).Run(sdl_context, controller, drawUnder);
        return PanelDialog(renderer, badges, PanelDialog::Kind::Confirm, text, { &badgeCancel, &badgeConfirm }).Run(sdl_context, controller, drawUnder);
    };

    auto ShowMessage = [&](const std::function<void()> & drawUnder, const std::string & titleKey)
    {
        PanelDialogText text;
        text.title = Translate(titleKey);
        return PanelDialog(renderer, badges, PanelDialog::Kind::Acknowledge, text, { &badgeOk }).Run(sdl_context, controller, drawUnder);
    };

    auto RunBusy = [&](const std::function<void()> & drawUnder, const std::string & command) -> bool
    {
        ShowBusy(sdl_context, drawUnder, Translate("SAVE_MANAGER_WORKING"));
        const int status = system(command.c_str());
        DropPendingInput(controller);
        return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    };

    auto RunSaveList = [&](const ListInfo & list) -> int
    {
        SetTitle(list.titleKey);
        Texture summaryHeader(Translate(list.headerKey), PanelHeaderFontSize, renderer, PanelInnerX, summaryBox.y + 14, false, ToAbgr(UiTheme::TextDim), true);
        Texture noneMarkedText(Translate("SAVE_MANAGER_NONE_MARKED"), PanelFontSize, renderer, PanelInnerX, SummaryListY, false, ToAbgr(UiTheme::Text), true);
        std::vector<Texture> noneHintLines = MakeLines(WrapToWidth(Translate("SAVE_MANAGER_NONE_HINT"), PanelFontSize, PanelInnerW), PanelFontSize, renderer, UiTheme::TextDim);
        note.SetText(Translate(list.noteKey));
        Badge actionPill = badges.MakePill("Start", "");
        GameNames gameNames = LoadGameNames();

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
            actionPill.label = Texture(Translate(list.actionHintKey) + " (" + std::to_string(count) + ")", 16, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true);

            int shown = (count > SummaryMaxRows) ? SummaryMaxRows - 1 : count;
            summaryNames.clear();
            for(int i = 0; i < shown; ++i)
                summaryNames.emplace_back(TruncateToWidth(marked[i]->label, PanelFontSize, PanelInnerW - BulletTextGap), PanelFontSize, renderer, PanelInnerX + BulletTextGap, 0, false, ToAbgr(UiTheme::Text), true);
            moreText = (shown < count)
                ? Texture("+" + std::to_string(count - shown), PanelFontSize, renderer, PanelInnerX + BulletTextGap, 0, false, ToAbgr(UiTheme::TextDim), true)
                : Texture();
        };

        // named entries first, alphabetically; the rest keep their code order
        auto ApplyNames = [&]()
        {
            for(SaveEntry & entry : entries)
            {
                entry.named = gameNames.count(entry.code) != 0;
                entry.label = EntryLabel(entry.code, gameNames);
                entry.text = Texture(TruncateToWidth(entry.label, ListFontSize, RowTextMaxW), ListFontSize, renderer, UiTheme::RowTextX, 0, false, ToAbgr(White), true);
            }
            std::stable_sort(entries.begin(), entries.end(), [](const SaveEntry & a, const SaveEntry & b)
            {
                if(a.named != b.named)
                    return a.named;
                return a.named && a.label < b.label;
            });
            RefreshSummary();
        };

        auto Load = [&]()
        {
            entries.clear();
            for(const std::string & code : ReadLines(OrphanSavesCommand(list.listArgs)))
            {
                SaveEntry entry;
                entry.code = code;
                entry.marked = list.preMarked;
                entries.push_back(entry);
            }
            listOffset = 0;
            currentId = 0;
            ApplyNames();
        };

        auto DrawScreen = [&]()
        {
            chrome.Draw();
            const int entryCount = static_cast<int>(entries.size());
            const int rowCount = entryCount + selectAllRows;
            const int count = std::min(displayCount, rowCount - listOffset);
            for(int i = 0; i < count; ++i)
            {
                const int row = listOffset + i;
                SDL_Rect rowRect = DrawListRow(renderer, i, ListRightX, RowBoxH, i == currentId);
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
            scrollArrows.Draw(listOffset != 0, listOffset + displayCount < rowCount);

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
            note.Draw();

            int rightEdge = UiTheme::BadgeClusterRightX;
            const int row = listOffset + currentId;
            const bool rowMarked = row == 0 ? markedCount == entryCount : entries[row - selectAllRows].marked;
            rightEdge = DrawBadge(rowMarked ? badgeUnmark : badgeMark, rightEdge);
            rightEdge = DrawBadge(badgeBack, rightEdge);
            if(markedCount > 0)
                rightEdge = DrawBadge(actionPill, rightEdge);
            DrawFooterDivider(renderer, rightEdge);
        };

        auto Labels = [&]()
        {
            std::vector<std::string> labels;
            for(const SaveEntry & entry : entries)
                labels.push_back(entry.label);
            return labels;
        };
        ListJumpInput jumpInput;

        Load();
        if(entries.empty())
            return ExitBack;
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
            else if(jump != ListJump::None)
                ApplyListJump(jump, Labels(), selectAllRows, displayCount, listOffset, currentId);
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
                const int maxNames = 6;
                PanelDialogText text;
                text.title = Translate(list.confirmKey);
                for(int i = 0; i < std::min(maxNames, static_cast<int>(marked.size())); ++i)
                    text.body.push_back(marked[i]->label);
                text.hiddenCount = std::max(0, static_cast<int>(marked.size()) - maxNames);
                text.note = Translate(list.noteKey);
                const DialogResult answer = Confirm(DrawScreen, text, list.destructive);
                if(answer == DialogResult::Quit)
                    return ExitLeaveMenu;
                if(answer == DialogResult::Confirmed)
                {
                    const bool fromBackup = &list == &RestoreBackupList;
                    bool ok;
                    // all marked: full restore, which also covers what the list hides
                    if(fromBackup && markedCount == entryCount)
                        ok = RunBusy(DrawScreen, "sh " + ScriptLocation + "RestoreSavesUsb.sh >/dev/null 2>&1");
                    else
                    {
                        std::ofstream selection(SelectionFile);
                        for(const SaveEntry * entry : marked)
                            selection << entry->code << "\n";
                        selection.close();
                        ok = RunBusy(DrawScreen, OrphanSavesCommand(list.actionArgs + SelectionFile));
                        std::remove(SelectionFile.c_str());
                    }
                    Load();
                    if(fromBackup && ShowMessage(DrawScreen, ok ? "RESTORE_SAVES_USB_DONE" : "RESTORE_SAVES_USB_FAILED") == DialogResult::Quit)
                        return ExitLeaveMenu;
                    // an empty list would still show a checked "select all"
                    if(entries.empty())
                        return ExitBack;
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
        chrome.Draw();
        for(int i = 0; i < static_cast<int>(Actions.size()); ++i)
        {
            SDL_Rect rowRect = DrawListRow(renderer, i, ListRightX, RowBoxH, i == currentAction);
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
        DrawFooterDivider(renderer, rightEdge);
    };

    auto RunAction = [&](Action action) -> int
    {
        switch(action)
        {
        case Action::Backup:
        {
            PanelDialogText text;
            text.title = Translate("SAVE_MANAGER_BACKUP_CONFIRM");
            text.note = Translate("SAVE_MANAGER_BACKUP_DESC");
            const DialogResult answer = Confirm(DrawActions, text, true);
            if(answer == DialogResult::Quit)
                return ExitLeaveMenu;
            if(answer == DialogResult::Confirmed)
            {
                const bool ok = RunBusy(DrawActions, "sh " + ScriptLocation + "BackupSavesUsb.sh >/dev/null 2>&1");
                RefreshStats();
                if(ShowMessage(DrawActions, ok ? "BACKUP_SAVES_USB_DONE" : "BACKUP_SAVES_USB_FAILED") == DialogResult::Quit)
                    return ExitLeaveMenu;
            }
            return -1;
        }
        case Action::RestoreUsb:
        case Action::Clean:
        case Action::RestoreDeleted:
        {
            const int available = action == Action::RestoreUsb ? stats.backupSaves : action == Action::Clean ? stats.orphans : stats.deleted;
            if(available <= 0)
            {
                const std::string titleKey = action == Action::RestoreUsb ? "RESTORE_SAVES_USB_NOT_FOUND" : action == Action::Clean ? "SAVE_MANAGER_NO_ORPHANS" : "SAVE_MANAGER_NO_DELETED";
                return ShowMessage(DrawActions, titleKey) == DialogResult::Quit ? ExitLeaveMenu : -1;
            }
            const int result = RunSaveList(action == Action::RestoreUsb ? RestoreBackupList : action == Action::Clean ? CleanList : RestoreDeletedList);
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
