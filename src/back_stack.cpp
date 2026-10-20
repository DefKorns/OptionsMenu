//  Copyright (C) 2026 DefKorns
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.

#include "back_stack.h"

#include <vector>

namespace back_stack
{
    std::string MakeEntry(const std::string & commandPath, const std::string & scriptPath,
                          const std::string & titleKey, const std::string & marker)
    {
        return commandPath + "," + scriptPath + "," + titleKey + (marker.empty() ? "" : "," + marker);
    }

    std::string PopIfLast(const std::string & stack, const std::string & entry)
    {
        const size_t lastSep = stack.find_last_of(';');
        const std::string last = stack.empty() ? "" : (lastSep == std::string::npos ? stack : stack.substr(lastSep + 1));
        if(last != entry)
            return stack;
        return lastSep == std::string::npos ? "" : stack.substr(0, lastSep);
    }

    std::string Push(const std::string & stack, const std::string & entry)
    {
        return stack.empty() ? entry : stack + ";" + entry;
    }

    std::string BackCommand(const std::string & stack, const std::string & optionsLocation)
    {
        if(stack.empty())
            return "";

        std::vector<std::string> entries;
        for(size_t start = 0; start < stack.size();)
        {
            size_t sep = stack.find(';', start);
            if(sep == std::string::npos)
                sep = stack.size();
            entries.push_back(stack.substr(start, sep - start));
            start = sep + 1;
        }
        const std::string lastEntry = entries.back();
        entries.pop_back();
        std::string remainingStack;
        for(size_t i = 0; i < entries.size(); ++i)
        {
            if(i)
                remainingStack += ";";
            remainingStack += entries[i];
        }

        const size_t c1 = lastEntry.find(',');
        const size_t c2 = c1 == std::string::npos ? c1 : lastEntry.find(',', c1 + 1);
        if(c2 == std::string::npos)
            return "";

        const std::string backPath = lastEntry.substr(0, c1);
        const std::string backScriptPath = lastEntry.substr(c1 + 1, c2 - c1 - 1);
        const size_t c3 = lastEntry.find(',', c2 + 1);
        const std::string backTitleKey = lastEntry.substr(c2 + 1, c3 == std::string::npos ? std::string::npos : c3 - c2 - 1);
        const std::string backMarker = c3 == std::string::npos ? "" : lastEntry.substr(c3 + 1);

        return "usleep 50000 && OM_BACK_STACK=\"" + remainingStack + "\" " + optionsLocation + "options --commandPath " + backPath
            + (backScriptPath.empty() ? "" : " --scriptPath " + backScriptPath)
            + " --title \"" + backTitleKey + "\""
            + (backMarker.empty() ? "" : " --selectedMarker \"" + backMarker + "\"") + " &";
    }
}
