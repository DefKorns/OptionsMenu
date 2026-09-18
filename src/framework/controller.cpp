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
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <ctime>
#ifndef __arm__
#include <SDL.h>
#endif

// avoids linking SDL just for a millisecond timer - optiond (Controller's
// other caller) doesn't otherwise need it
static unsigned int MonotonicMillis()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<unsigned int>(ts.tv_sec)*1000 + ts.tv_nsec/1000000;
}

Controller::Controller(int id)
{
#ifdef __arm__
    std::stringstream s;
    s << "/dev/input/by-path/platform-twi." << id << "-event-joystick";

    //Right after boot this node sometimes doesn't exist yet (input driver
    //still enumerating) and optiond, started from init.d, hits it before
    //it's there. Retry for ~10s instead of exiting immediately.
    const int maxAttempts = 50;
    const useconds_t retryDelay = 200000;
    for(int attempt = 0; fd == -1 && attempt < maxAttempts; ++attempt)
    {
        fd = open(s.str().c_str(), O_RDONLY | O_NONBLOCK);
        if(fd == -1)
            usleep(retryDelay);
    }
    if(fd == -1)
    {
      std::cerr << "Cannot access controller.\n";
      exit(1);
    }
#endif // __arm__

    Reset();
}
Controller::~Controller()
{
#ifdef __arm__
    if(fd != -1)
        close(fd);
#endif // __arm__
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
    int len;
    while((len = read(fd, &buttonBuffer, sizeof(ButtonEvent)*10)) > 0)
    {
        len/=sizeof(ButtonEvent);
        for(int i = 0; i < len; ++i)
        {
            auto & buttonEvent = buttonBuffer[i];
            //Unk1 equals 1 when displaying button press status
            if(buttonEvent.unk1 == 1)
            {
                buttons[buttonEvent.button] = buttonEvent.pressed;
            }
        }
    }
#else
    // keyboard fallback for desktop builds - only write buttons[] on an edge,
    // matching evdev's own press/release-only stream
    SDL_PumpEvents(); // nested loops (confirm dialog) don't otherwise pump
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
#endif // __arm__
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
