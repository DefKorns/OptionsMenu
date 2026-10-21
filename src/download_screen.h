/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef DOWNLOAD_SCREEN_H_
#define DOWNLOAD_SCREEN_H_

#include "command.h"
#include "framework/controller.h"
#include "framework/sdl_helper.h"

#include <string>

// Runs a downloader command with a progress bar and its messages; B cancels it.
// The screen closes on its own a moment after the command exits 0 and waits for B otherwise.
// heading is shown before the title ("Installing"), author under it; both may be empty.
bool RunDownloadScreen(const std::string & command, const std::string & heading, const std::string & title,
                       const std::string & author, SDL_Context & sdlContext, Controller & controller,
                       ScreenChrome & chrome);

#endif
