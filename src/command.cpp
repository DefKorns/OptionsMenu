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
#include "localization.h"

#include <cctype>
#include <fstream>
#include <list>
#include <vector>
#include <poll.h>

// std::stoi throws on non-numeric input; avoid crashing on a bad command file
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
    }
    in.close();
}

void Command::RunCommand(SDL_Context & sdl_context, Controller * controller, Sprite & menuL, Sprite & menuU, bool modernUI, const NineSlice & frame, Uint8 bgR, Uint8 bgG, Uint8 bgB) const
{
    std::list<Texture> textList;
    FILE* pipe = popen(command.c_str(), "r");
    if(pipe)
    {
        auto renderer = sdl_context.renderer;
        if(modernUI)
            SDL_SetRenderDrawColor(renderer, UiTheme::BgR, UiTheme::BgG, UiTheme::BgB, SDL_ALPHA_OPAQUE);
        else
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        char buffer[128] = {0};
        const int textX = modernUI ? UiTheme::FrameRect.x + UiTheme::FrameInset : 30;
        const int textFirstY = modernUI ? UiTheme::HeaderDividerY + 24 : 100;

        auto render = [&](Texture * closeText = nullptr)
        {
            sdl_context.StartFrame();
            if(modernUI)
                frame.Draw(renderer, UiTheme::FrameRect);
            else
            {
                menuU.Draw(renderer);
                menuL.Draw(renderer);
            }
            int y = textFirstY;
            for(auto & t : textList)
            {
                t.rect.y = y;
                t.Draw(renderer);
                y+=10;
            }
            if(closeText)
                closeText->Draw(renderer);
            sdl_context.EndFrame();
        };

        int fd = fileno(pipe);
        while(!feof(pipe))
        {
            // poll with a short timeout instead of blocking fgets, so a script
            // that stalls without output doesn't also freeze input/rendering
            struct pollfd pfd{fd, POLLIN, 0};
            if(poll(&pfd, 1, 100) > 0 && fgets(buffer, 128, pipe) != nullptr)
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

            controller->Update();
            if(!ignoreInterrupt && controller->GetButtonStatus(B))
                break;

            render();
        }
        pclose(pipe);
        Texture closeText(Translate("PRESS_B_EXIT"), 12, renderer, textX, modernUI ? UiTheme::CreditY : 610);

        while (!controller->GetButtonStatus(B))
        {
            controller->Update();
            render(&closeText);
        }
        SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 0xFF);
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
    // state scripts are expected to be near-instant; don't let a stuck one block menu load
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
