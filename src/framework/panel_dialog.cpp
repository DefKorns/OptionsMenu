/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
  */

#include "panel_dialog.h"
#include "controller.h"
#include "dialog.h"
#include "draw_helpers.h"
#include "powerwatch.h"
#include "sdl_context.h"
#include "text_layout.h"
#include "uitheme.h"

#include <algorithm>
#include <utility>

namespace
{
    constexpr int BoxW = 600;
    constexpr int BaseH = 110;
    constexpr int TitleTopY = 36;
    constexpr int TitlePitch = 32;
    constexpr int WrapInset = 48;
    constexpr int BodyTopY = 72;
    constexpr int BodyFontSize = 16;
    constexpr int BodyPitch = 30;
    constexpr int BodyInset = 80;
    constexpr int BulletSize = 8;
    constexpr int BulletTextGap = 18;
    constexpr int NoteFontSize = 15;
    constexpr int NotePitch = 20;
    constexpr int NoteGap = 6;

    constexpr int BusyMinW = 420;
    constexpr int BusyPadW = 96;
    constexpr int BusyH = 110;
    constexpr int BusyTextLift = 12;

    DialogResult Finish(Controller & controller, DialogResult result)
    {
        DropPendingInput(controller);
        return result;
    }
}

PanelDialog::PanelDialog(SDL_Renderer * renderer, BadgePainter & painter, Kind kind, const PanelDialogText & text, std::vector<Badge *> badges)
    : renderer_(renderer)
    , painter_(&painter)
    , kind_(kind)
    , text_(text)
    , badges_(std::move(badges))
{
}

void PanelDialog::SetBulletColor(Color color)
{
    bullets_ = true;
    bulletColor_ = color;
}

void PanelDialog::Layout()
{
    const int bodyMaxW = BoxW - BodyInset - (bullets_ ? BulletTextGap : 0);
    body_.clear();
    for(const std::string & line : text_.body)
        body_.emplace_back(TruncateToWidth(line, BodyFontSize, bodyMaxW), BodyFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::Text), true);
    bulletRows_ = static_cast<int>(body_.size());
    if(text_.hiddenCount > 0)
        body_.emplace_back("+" + std::to_string(text_.hiddenCount), BodyFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::TextDim), true);

    note_.clear();
    if(!text_.note.empty())
        note_ = MakeLines(WrapToWidth(text_.note, NoteFontSize, BoxW - WrapInset), NoteFontSize, renderer_, UiTheme::TextDim);

    title_.clear();
    for(const std::string & line : WrapToWidth(text_.title, Dialog::TitleFontSize, BoxW - WrapInset))
        title_.emplace_back(line, Dialog::TitleFontSize, renderer_, 0, 0, false, ToAbgr(UiTheme::Text), true);
    titleExtraH_ = std::max(0, static_cast<int>(title_.size()) - 1) * TitlePitch;

    const int boxH = BaseH + titleExtraH_ + static_cast<int>(body_.size()) * BodyPitch + static_cast<int>(note_.size()) * NotePitch
        + Dialog::BadgeRowGap + UiTheme::BadgeOuterSize;
    box_ = Dialog::PanelRect(BoxW, boxH);
    const int centerX = box_.x + box_.w / 2;
    for(size_t i = 0; i < title_.size(); ++i)
    {
        title_[i].rect.x = centerX - title_[i].rect.w / 2;
        title_[i].rect.y = box_.y + TitleTopY + static_cast<int>(i) * TitlePitch - title_[i].rect.h / 2;
    }

    int widest = 0;
    for(int i = 0; i < bulletRows_; ++i)
        widest = std::max(widest, body_[i].rect.w);
    bulletX_ = centerX - (widest + BulletTextGap) / 2;
}

void PanelDialog::Draw()
{
    Dialog::DrawBackdrop(renderer_);
    Dialog::DrawPanel(renderer_, box_);
    for(Texture & line : title_)
        line.Draw(renderer_);

    const int centerX = box_.x + box_.w / 2;
    int y = box_.y + BodyTopY + titleExtraH_;
    for(int i = 0; i < static_cast<int>(body_.size()); ++i)
    {
        Texture & line = body_[i];
        if(bullets_)
        {
            if(i < bulletRows_)
                DrawRoundedFillRect(renderer_, { bulletX_, y + line.rect.h / 2 - BulletSize / 2, BulletSize, BulletSize }, bulletColor_, BulletSize / 2);
            line.rect.x = bulletX_ + BulletTextGap;
        }
        else
            line.rect.x = centerX - line.rect.w / 2;
        line.rect.y = y;
        line.Draw(renderer_);
        y += BodyPitch;
    }
    DrawLinesCentered(renderer_, note_, centerX, y + NoteGap, NotePitch);
    painter_->DrawCentered(badges_, centerX, box_.y + box_.h - Dialog::BottomPad - UiTheme::BadgeOuterSize);
}

DialogResult PanelDialog::Run(SDL_Context & sdl, Controller & controller, const std::function<void()> & drawUnder)
{
    Layout();
    DropPendingInput(controller);
    for(;;)
    {
        bool quit = false;
        SDL_Event e;
        while(SDL_PollEvent(&e))
            quit = quit || e.type == SDL_QUIT;
        controller.Update();
        if(quit || sdl.powerwatch->buttonPress())
            return Finish(controller, DialogResult::Quit);
        if(kind_ == Kind::Confirm)
        {
            if(controller.GetButtonStatus(B))
                return Finish(controller, DialogResult::Cancelled);
            if(controller.PeekButtonStatus(START) && controller.PeekButtonStatus(SELECT))
                return Finish(controller, DialogResult::Confirmed);
        }
        else if(controller.GetButtonStatus(A))
            return Finish(controller, DialogResult::Confirmed);

        sdl.StartFrame();
        drawUnder();
        Draw();
        SetDrawColor(renderer_, UiTheme::Bg);
        sdl.EndFrame();
    }
}

void ShowBusy(SDL_Context & sdl, const std::function<void()> & drawUnder, const std::string & message)
{
    SDL_Renderer * renderer = sdl.renderer;
    const SDL_Rect box = Dialog::PanelRect(std::max(BusyMinW, TextWidth(message, Dialog::TitleFontSize) + BusyPadW), BusyH);
    Texture text(message, Dialog::TitleFontSize, renderer, box.x + box.w / 2, box.y + box.h / 2 - BusyTextLift, true, ToAbgr(UiTheme::Text), true);
    sdl.StartFrame();
    drawUnder();
    Dialog::DrawBackdrop(renderer);
    Dialog::DrawPanel(renderer, box);
    text.Draw(renderer);
    SetDrawColor(renderer, UiTheme::Bg);
    sdl.EndFrame();
}

// Update() first: presses queued in evdev during blocking work would otherwise land on the next frame.
void DropPendingInput(Controller & controller)
{
    controller.Update();
    controller.Reset();
}
