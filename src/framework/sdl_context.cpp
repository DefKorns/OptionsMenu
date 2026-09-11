/**
  * Copyright (C) 2017-2018 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "sdl_context.h"
#include "powerwatch.h"

#include <algorithm>
#include <iostream>
#include <thread>
#include <SDL.h>
#include <SDL_ttf.h>

SDL_Context::SDL_Context(std::chrono::milliseconds fpsTime, bool powerButtonExit) : fpsTime(fpsTime), powerButtonExit(powerButtonExit)
{
    powerwatch = new PowerWatch();
    window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_FULLSCREEN);
    // vsync makes SDL_RenderPresent block for the next vertical blank instead
    // of returning immediately, so a post-pause frame burst (see EndFrame)
    // can't outrun the display and tear/flicker the way it could unthrottled
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if(renderer == NULL) {
        std::cerr << "Cannot create renderer.\n";
        SDL_DestroyWindow(window);
        exit(1);
    }
    TTF_Init();
    nextFrameTime = std::chrono::system_clock::now()+fpsTime;
}

SDL_Context::~SDL_Context()
{
    Shutdown();
    delete powerwatch;
}

void SDL_Context::Shutdown()
{
    if(shutDown)
        return;
    shutDown = true;
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDL_Context::StartFrame()
{
    if(powerButtonExit && powerwatch->buttonPress())
        exit(1);
    SDL_RenderClear(renderer);
}

void SDL_Context::EndFrame()
{
    SDL_RenderPresent(renderer);
    std::this_thread::sleep_until(nextFrameTime);
    // after a long stall (e.g. hakchi's "uipause" SIGSTOPs this process
    // while it reads /dev/fb0 for a screenshot), nextFrameTime is left far
    // in the past - clamp to now so resuming doesn't burn through a burst
    // of back-to-back unthrottled frames trying to catch up, which is what
    // caused the flicker after taking a screenshot
    nextFrameTime = std::max(nextFrameTime, std::chrono::system_clock::now()) + fpsTime;
}
