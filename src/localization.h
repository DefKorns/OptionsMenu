/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#ifndef LOCALIZATION_H_
#define LOCALIZATION_H_

#include <string>

// optionsRoot is the options_menu install directory, with a trailing slash
// (e.g. "/etc/options_menu/"). Loads <optionsRoot>/language/, then merges in
// <optionsRoot>/<plugin>/lang/ for every plugin subfolder that has one.
void LoadLanguage(const std::string & optionsRoot, const std::string & langCode);

// reads <optionsRoot>/language.cfg (en-US if missing or empty) and loads it
void LoadLanguageFromConfig(const std::string & optionsRoot);

// key itself when there is no translation for it
std::string Translate(const std::string & key);

#endif
