/**
  * Copyright (C) 2017-2018 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "controller.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <ctime>
#ifndef __arm__
#include <SDL.h>
#endif

namespace
{
    const std::string CloverconNamePrefix = "Nintendo Clovercon";
    constexpr unsigned int RescanIntervalMs = 1000;
}

static unsigned int MonotonicMillis()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<unsigned int>(ts.tv_sec)*1000 + ts.tv_nsec/1000000;
}

Controller::Controller(int)
{
#ifdef __arm__
    // the input node can appear a few seconds after boot
    const int maxAttempts = 50;
    const useconds_t retryDelay = 200000;
    for(int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        ScanCloverconPads();
        if(!fds.empty())
            break;
        usleep(retryDelay);
    }
    if(fds.empty())
        std::cerr << "No controller yet, waiting for one.\n";
#endif

    Reset();
}
Controller::~Controller()
{
#ifdef __arm__
    for(int fd : fds)
        close(fd);
#endif
}
void Controller::OpenNode(const std::string & node)
{
    if(std::find(openNodes.begin(), openNodes.end(), node) != openNodes.end())
        return;
    const int fd = open(("/dev/input/" + node).c_str(), O_RDONLY | O_NONBLOCK);
    if(fd == -1)
        return;
    fds.push_back(fd);
    openNodes.push_back(node);
}
void Controller::ScanCloverconPads()
{
    lastScan = MonotonicMillis();
    DIR * dir = opendir("/sys/class/input");
    if(!dir)
        return;
    while(dirent * entry = readdir(dir))
    {
        const std::string node = entry->d_name;
        if(node.compare(0, 5, "event") != 0)
            continue;
        std::ifstream nameFile("/sys/class/input/" + node + "/device/name");
        std::string name;
        std::getline(nameFile, name);
        if(name.compare(0, CloverconNamePrefix.size(), CloverconNamePrefix) == 0)
            OpenNode(node);
    }
    closedir(dir);
}
bool Controller::PeekButtonStatus(GameButton button)
{
    return buttons[button];
}
bool Controller::GetButtonStatus(GameButton button)
{
    bool result = buttons[button];
    if(result) buttons[button] = 0;
    return result;
}
void Controller::Update()
{
#ifdef __arm__
    for(size_t device = 0; device < fds.size();)
    {
        int len;
        while((len = read(fds[device], &buttonBuffer, sizeof(ButtonEvent)*10)) > 0)
        {
            len/=sizeof(ButtonEvent);
            for(int i = 0; i < len; ++i)
            {
                auto & buttonEvent = buttonBuffer[i];
                // unk1 == 1 marks a button event
                if(buttonEvent.unk1 == 1)
                {
                    buttons[buttonEvent.button] = buttonEvent.pressed;
                }
            }
        }
        if(len == -1 && errno == ENODEV)
        {
            close(fds[device]);
            fds.erase(fds.begin() + device);
            openNodes.erase(openNodes.begin() + device);
            continue;
        }
        ++device;
    }
    if(MonotonicMillis() - lastScan >= RescanIntervalMs)
        ScanCloverconPads();
#else
    SDL_PumpEvents();
    const Uint8 * keys = SDL_GetKeyboardState(nullptr);
    auto press = [&](SDL_Scancode sc, GameButton button)
    {
        bool now = keys[sc] != 0;
        if(now != (prevKeys[sc] != 0))
            buttons[button] = now;
        prevKeys[sc] = now;
    };
    press(SDL_SCANCODE_UP, UP);
    press(SDL_SCANCODE_DOWN, DOWN);
    press(SDL_SCANCODE_LEFT, LEFT);
    press(SDL_SCANCODE_RIGHT, RIGHT);
    press(SDL_SCANCODE_RETURN, A);
    press(SDL_SCANCODE_Z, A);
    press(SDL_SCANCODE_BACKSPACE, B);
    press(SDL_SCANCODE_X, B);
    press(SDL_SCANCODE_SPACE, START);
    press(SDL_SCANCODE_TAB, SELECT);
    press(SDL_SCANCODE_Q, L);
    press(SDL_SCANCODE_E, R);
#endif
}
bool Controller::HeldRepeat(GameButton button)
{
    const unsigned int repeatDelay = 350, repeatInterval = 90;
    if(!PeekButtonStatus(button))
    {
        repeatActive[button] = false;
        return false;
    }
    unsigned int now = MonotonicMillis();
    if(!repeatActive[button])
    {
        repeatActive[button] = true;
        repeatPressedAt[button] = now;
        repeatLastFired[button] = now;
        return true;
    }
    if(now - repeatPressedAt[button] >= repeatDelay && now - repeatLastFired[button] >= repeatInterval)
    {
        repeatLastFired[button] = now;
        return true;
    }
    return false;
}
unsigned int Controller::HeldMillis(GameButton button)
{
    if(!PeekButtonStatus(button))
    {
        heldActive[button] = false;
        return 0;
    }
    unsigned int now = MonotonicMillis();
    if(!heldActive[button])
    {
        heldActive[button] = true;
        heldPressedAt[button] = now;
    }
    return now - heldPressedAt[button];
}
void Controller::Reset()
{
    buttons[A] = 0;
    buttons[B] = 0;
    buttons[X] = 0;
    buttons[Y] = 0;
    buttons[L] = 0;
    buttons[R] = 0;
    buttons[SELECT] = 0;
    buttons[START] = 0;
    buttons[LEFT] = 0;
    buttons[RIGHT] = 0;
    buttons[UP] = 0;
    buttons[DOWN] = 0;
}
