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

void LoadLanguage(const std::string & langDir, const std::string & langCode);
std::string Translate(const std::string & key);

#endif
