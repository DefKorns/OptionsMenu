/**
 * Copyright (C) 2017-2018 CompCom
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#include "command.h"
#include "framework/controller.h"
#include "framework/draw_helpers.h"
#include "localization.h"

#include <cctype>
#include <fstream>
#include <list>
#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

struct CommandProcess
{
    FILE * out;
    pid_t pid;
};

static CommandProcess StartCommand(const std::string & command)
{
    int fds[2];
    if(pipe(fds) != 0)
        return { nullptr, -1 };
    const pid_t pid = fork();
    if(pid < 0)
    {
        close(fds[0]);
        close(fds[1]);
        return { nullptr, -1 };
    }
    if(pid == 0)
    {
        setpgid(0, 0);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char *>(nullptr));
        _exit(127);
    }
    setpgid(pid, pid);
    close(fds[1]);
    return { fdopen(fds[0], "r"), pid };
}

static void FinishCommand(const CommandProcess & process, bool abort)
{
    if(abort)
        kill(-process.pid, SIGTERM);
    fclose(process.out);
    waitpid(process.pid, nullptr, 0);
}

static int SafeStoi(const std::string & value, int fallback = 0)
{
    try
    {
        return std::stoi(value);
    }
    catch(...)
    {
        return fallback;
    }
}

Command::Command() {}

Command::Command(std::ifstream & in)
{
    std::string temp;
    while(in.good())
    {
        std::getline(in, temp);
        if(!temp.empty() && temp.back() == '\r')
            temp.pop_back();
        if(temp[0]=='#')
            continue;
        int index = temp.find('=');
        std::string param = temp.substr(0, index), value = temp.substr(index+1);
        if(param.compare("COMMAND_NAME")==0)
            name = value;
        else if(param.compare("COMMAND_TYPE")==0)
            runInternal = (value == "INTERNAL");
        else if(param.compare("RESTART_UI")==0)
            restartUI = (value == "TRUE");
        else if(param.compare("IGNORE_INTERRUPT")==0)
            ignoreInterrupt = (value == "TRUE");
        else if(param.compare("COMMAND_STR")==0)
            command = value;
        else if(param.compare("DELETE_STR")==0)
            deleteCommand = value;
        else if(param.compare("DELETE_CONFIRM_KEY")==0)
            deleteConfirmKey = value;
        else if(param.compare("ENABLE_IF")==0)
            enableIfCommand = value;
        else if(param.compare("STATE_STR")==0)
        {
            stateCommand = value;
            isToggle = true;
        }
        else if(param.compare("USB_ONLY")==0)
            usbOnly = (value == "TRUE");
        else if(param.compare("CHILD")==0)
            child = (value == "TRUE");
        else if(param.compare("PREVIEW_IMAGE")==0)
            previewImage = value;
        else if(param.compare("PREVIEW_IMAGE_X")==0)
            previewImageX = SafeStoi(value, 0);
        else if(param.compare("PREVIEW_IMAGE_Y")==0)
            previewImageY = SafeStoi(value, 0);
        else if(param.compare("PREVIEW_IMAGE_WIDTH")==0)
            previewImageWidth = SafeStoi(value, -1);
        else if(param.compare("PREVIEW_IMAGE_HEIGHT")==0)
            previewImageHeight = SafeStoi(value, -1);
        else if(param.compare("PREVIEW_NEAREST")==0)
            previewNearest = (value == "TRUE");
        else if(param.compare("PREVIEW_SQUARE")==0)
            previewSquare = (value == "TRUE");
        else if(param.compare("PREVIEW_GRID_COLS")==0)
            previewGridCols = SafeStoi(value, -1);
        else if(param.compare("PREVIEW_FIT_CONTAIN")==0)
            previewFitContain = (value == "TRUE");
        else if(param.compare("PREVIEW_HIDE_LABEL")==0)
            previewHideLabel = (value == "TRUE");
        else if(param.compare("SUBMENU")==0)
            hasSubmenu = (value == "TRUE");
    }
    if(isToggle)
        hasSubmenu = false;
    in.close();
}

void Command::RunCommand(SDL_Context & sdl_context, Controller * controller, const ModernChrome & chrome, Color bg) const
{
    std::list<Texture> textList;
    const CommandProcess process = StartCommand(command);
    FILE * out = process.out;
    if(out)
    {
        auto renderer = sdl_context.renderer;
        SetDrawColor(renderer, UiTheme::Bg);
        char buffer[128] = {0};
        const int textX = UiTheme::FrameRect.x + UiTheme::FrameInset;
        const int textFirstY = UiTheme::HeaderDividerY + 24;

        Texture exitLetter("B", 16, renderer, 0, 0, false, ToAbgr(UiTheme::BadgeLetter), true);
        Texture exitLabel(Translate("EXIT"), 16, renderer, 0, 0, false, ToAbgr(UiTheme::Text), true);
        int badgeGroupW = UiTheme::BadgeOuterSize + UiTheme::BadgeLabelGap + exitLabel.rect.w;
        SDL_Rect exitBadge{ UiTheme::BadgeClusterRightX - badgeGroupW, UiTheme::BadgeBandY, UiTheme::BadgeOuterSize, UiTheme::BadgeOuterSize };
        exitLetter.rect.x = exitBadge.x + (exitBadge.w - exitLetter.rect.w) / 2;
        exitLetter.rect.y = exitBadge.y + (exitBadge.h - exitLetter.rect.h) / 2;
        exitLabel.rect.x = exitBadge.x + exitBadge.w + UiTheme::BadgeLabelGap;
        exitLabel.rect.y = exitBadge.y + (exitBadge.h - exitLabel.rect.h) / 2;

        auto render = [&]()
        {
            sdl_context.StartFrame();
            DrawStrokeRect(renderer, UiTheme::OuterRect, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BorderRadius);
            chrome.gearIcon.Draw(renderer);
            chrome.appTitleText.Draw(renderer);
            chrome.appVersionText.Draw(renderer);
            DrawHLine(renderer, UiTheme::HeaderDividerX, UiTheme::HeaderDividerX + UiTheme::HeaderDividerW, UiTheme::HeaderDividerY, UiTheme::Border, UiTheme::BorderWidth);
            DrawHLine(renderer, UiTheme::OuterRect.x, UiTheme::OuterRect.x + UiTheme::OuterRect.w, UiTheme::FooterDividerY, UiTheme::Border, UiTheme::BorderWidth);
            chrome.creditText.Draw(renderer);
            DrawRoundedFillRect(renderer, exitBadge, UiTheme::BadgeB, exitBadge.w/2);
            exitLetter.Draw(renderer);
            exitLabel.Draw(renderer);
            int y = textFirstY;
            for(auto & t : textList)
            {
                t.rect.y = y;
                t.Draw(renderer);
                y+=10;
            }
            SetDrawColor(renderer, UiTheme::Bg); // must be the last color call, or it leaks into the next frame's clear
            sdl_context.EndFrame();
        };

        int fd = fileno(out);
        fcntl(fd, F_SETFL, O_NONBLOCK);
        const int PollTimeoutMs = 20;
        bool aborted = false;
        while(!feof(out))
        {
            // stdio can hold lines poll() never sees, so fgets drains every pass
            struct pollfd pfd{fd, POLLIN, 0};
            poll(&pfd, 1, PollTimeoutMs);
            while(fgets(buffer, 128, out) != nullptr)
            {
                std::string sBuffer(buffer);
                int pos = sBuffer.find('\n');
                if(pos > 0)
                    sBuffer[pos] = '\0';
                textList.push_back(Texture(sBuffer, 8, renderer, textX));

                if(textList.size() > 40)
                {
                    textList.erase(textList.begin());
                }
            }
            if(!feof(out))
                clearerr(out);

            // ignoreInterrupt commands read the controller themselves
            if(!ignoreInterrupt)
            {
                controller->Update();
                if(controller->GetButtonStatus(B))
                {
                    aborted = true;
                    break;
                }
            }

            if(!textList.empty())
                render();
        }
        FinishCommand(process, aborted);

        if(textList.empty() || aborted)
        {
            SetDrawColor(renderer, bg);
            return;
        }
        while (!controller->GetButtonStatus(B))
        {
            controller->Update();
            render();
        }
        SetDrawColor(renderer, bg);
    }
}

void Command::UpdateState()
{
    stateOn = false;
    if(stateCommand.empty())
        return;

    FILE * pipe = popen(stateCommand.c_str(), "r");
    if(!pipe)
        return;

    char buffer[64] = {0};
    std::string result;
    struct pollfd pfd{fileno(pipe), POLLIN, 0};
    if(poll(&pfd, 1, 1000) > 0 && fgets(buffer, sizeof(buffer), pipe))
        result = buffer;
    pclose(pipe);

    size_t start = 0;
    while(start < result.size() && std::isspace(static_cast<unsigned char>(result[start])))
        ++start;
    size_t end = result.size();
    while(end > start && std::isspace(static_cast<unsigned char>(result[end-1])))
        --end;
    result = result.substr(start, end-start);
    for(char & ch : result)
        ch = std::tolower(static_cast<unsigned char>(ch));

    stateOn = (result == "1" || result == "on" || result == "y" || result == "yes" || result == "true");
}
