/**
  * Copyright (C) 2026 DefKorns
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "localization.h"

#include <cctype>
#include <dirent.h>
#include <fstream>
#include <map>
#include <memory>
#include <set>
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

    std::string Upper(std::string text)
    {
        for(char & c : text)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return text;
    }

    // the xx-YY.lang for a console language code (ja_JPN, pt_br, pt_EUR, sv...): the region the code
    // names (pt_br is pt-BR), else the language's own (pt-PT), else the first
    std::string LangCodeFor(const std::string & optionsRoot, const std::string & code)
    {
        if(code.size() < 2)
            return "";
        const std::string lang = code.substr(0, 2);
        const std::string prefix = lang + "-";
        const DirHandle dir(opendir((optionsRoot + "language/").c_str()), &closedir);
        if(!dir)
            return "";
        std::set<std::string> found;
        while(const dirent * entry = readdir(dir.get()))
        {
            const std::string name = entry->d_name;
            if(name.compare(0, prefix.size(), prefix) == 0 && name.size() > 5 && name.compare(name.size() - 5, 5, ".lang") == 0)
                found.insert(name.substr(0, name.size() - 5));
        }
        if(found.empty())
            return "";
        const size_t sep = code.find('_');
        if(sep != std::string::npos && found.count(prefix + Upper(code.substr(sep + 1))) != 0)
            return prefix + Upper(code.substr(sep + 1));
        if(found.count(prefix + Upper(lang)) != 0)
            return prefix + Upper(lang);
        return *found.begin();
    }

    // Console Language's language when it has set one, else the console's
    std::string ConsoleLangCode(const std::string & optionsRoot)
    {
        const std::string replaced = FirstLine("/var/console_language_ui");
        return LangCodeFor(optionsRoot, replaced.empty() ? FirstLine("/var/lib/clover/profiles/0/lang.txt") : replaced);
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
