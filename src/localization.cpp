/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "localization.h"

#include <fstream>
#include <map>

static std::map<std::string, std::string> strings;

static void LoadLanguageFile(const std::string & path)
{
    std::ifstream in(path);
    std::string line;
    while(std::getline(in, line))
    {
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        if(line.empty() || line[0] == '#')
            continue;
        size_t index = line.find('=');
        if(index == std::string::npos)
            continue;
        strings[line.substr(0, index)] = line.substr(index+1);
    }
}

void LoadLanguage(const std::string & langDir, const std::string & langCode)
{
    strings.clear();
    LoadLanguageFile(langDir + "en-US.lang");
    if(langCode != "en-US")
        LoadLanguageFile(langDir + langCode + ".lang");
}

std::string Translate(const std::string & key)
{
    auto it = strings.find(key);
    return it == strings.end() ? key : it->second;
}
