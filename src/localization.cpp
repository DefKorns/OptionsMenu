/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "localization.h"

#include <dirent.h>
#include <fstream>
#include <map>
#include <memory>

namespace
{
    const std::string DefaultLangCode = "en-US";

    // I.2 exception: Translate() is a free function called from every screen
    std::map<std::string, std::string> translations;

    using DirHandle = std::unique_ptr<DIR, decltype(&closedir)>;

    bool FileExists(const std::string & path)
    {
        return std::ifstream(path).good();
    }

    void LoadLanguageFile(const std::string & path)
    {
        std::ifstream in(path);
        std::string line;
        while(std::getline(in, line))
        {
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            if(line.empty() || line[0] == '#')
                continue;
            const size_t eq = line.find('=');
            if(eq == std::string::npos)
                continue;
            translations[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }

    // en-US first so keys missing from a partial translation still resolve
    void LoadLanguageDir(const std::string & langDir, const std::string & langCode)
    {
        LoadLanguageFile(langDir + DefaultLangCode + ".lang");
        if(langCode != DefaultLangCode)
            LoadLanguageFile(langDir + langCode + ".lang");
    }
}

void LoadLanguage(const std::string & optionsRoot, const std::string & langCode)
{
    translations.clear();
    LoadLanguageDir(optionsRoot + "language/", langCode);

    const DirHandle dir(opendir(optionsRoot.c_str()), &closedir);
    if(!dir)
        return;
    while(const dirent * entry = readdir(dir.get()))
    {
        if(entry->d_type != DT_DIR || entry->d_name[0] == '.')
            continue;
        const std::string pluginLangDir = optionsRoot + entry->d_name + "/lang/";
        if(FileExists(pluginLangDir + DefaultLangCode + ".lang"))
            LoadLanguageDir(pluginLangDir, langCode);
    }
}

void LoadLanguageFromConfig(const std::string & optionsRoot)
{
    std::string langCode;
    std::ifstream in(optionsRoot + "language.cfg");
    std::getline(in, langCode);
    if(!langCode.empty() && langCode.back() == '\r')
        langCode.pop_back();
    LoadLanguage(optionsRoot, langCode.empty() ? DefaultLangCode : langCode);
}

std::string Translate(const std::string & key)
{
    const auto it = translations.find(key);
    return it == translations.end() ? key : it->second;
}
