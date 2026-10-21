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
#include <vector>

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

    std::string FirstLine(const std::string & path)
    {
        std::string line;
        std::ifstream in(path);
        std::getline(in, line);
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        return line;
    }

    // the xx-YY.lang whose language is the first two letters of code (ja_JPN, pt, pt_br...)
    std::string LangCodeFor(const std::string & optionsRoot, const std::string & code)
    {
        if(code.size() < 2)
            return "";
        const std::string prefix = code.substr(0, 2) + "-";
        const DirHandle dir(opendir((optionsRoot + "language/").c_str()), &closedir);
        if(!dir)
            return "";
        while(const dirent * entry = readdir(dir.get()))
        {
            const std::string name = entry->d_name;
            if(name.compare(0, prefix.size(), prefix) == 0 && name.size() > 5 && name.compare(name.size() - 5, 5, ".lang") == 0)
                return name.substr(0, name.size() - 5);
        }
        return "";
    }

    // Console Language shows its language in the Japanese slot, so lang.txt still says ja then
    std::string ConsoleLangCode(const std::string & optionsRoot)
    {
        const std::string console = FirstLine("/var/lib/clover/profiles/0/lang.txt");
        const std::string replaced = FirstLine("/var/console_language_ui");
        if(console.compare(0, 2, "ja") == 0 && !replaced.empty())
            return LangCodeFor(optionsRoot, replaced);
        return LangCodeFor(optionsRoot, console);
    }
}

void LoadLanguage(const std::string & optionsRoot, const std::string & langCode)
{
    std::vector<std::string> langDirs{ optionsRoot + "language/" };
    const DirHandle dir(opendir(optionsRoot.c_str()), &closedir);
    while(const dirent * entry = dir ? readdir(dir.get()) : nullptr)
    {
        if(entry->d_name[0] == '.' || (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN))
            continue;
        const std::string pluginLangDir = optionsRoot + entry->d_name + "/lang/";
        if(FileExists(pluginLangDir + DefaultLangCode + ".lang"))
            langDirs.push_back(pluginLangDir);
    }

    // every English fallback first, so an add-on without this language can't override the core's strings
    translations.clear();
    for(const auto & langDir : langDirs)
        LoadLanguageFile(langDir + DefaultLangCode + ".lang");
    if(langCode != DefaultLangCode)
        for(const auto & langDir : langDirs)
            LoadLanguageFile(langDir + langCode + ".lang");
}

void LoadLanguageFromConfig(const std::string & optionsRoot)
{
    // language.cfg only exists once a language was picked in the menu
    std::string langCode = FirstLine(optionsRoot + "language.cfg");
    if(langCode.empty())
        langCode = ConsoleLangCode(optionsRoot);
    LoadLanguage(optionsRoot, langCode.empty() ? DefaultLangCode : langCode);
}

std::string Translate(const std::string & key)
{
    const auto it = translations.find(key);
    return it == translations.end() ? key : it->second;
}
