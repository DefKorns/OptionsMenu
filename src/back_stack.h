//  Copyright (C) 2026 DefKorns
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.

#ifndef BACK_STACK_H_
#define BACK_STACK_H_

#include <string>

// OM_BACK_STACK is ';'-separated entries of "commandPath,scriptPath,titleKey[,marker]".
// launch_save_manager.sh and launch_favorites_manager.sh read title_key as the
// rest after the second comma, so for 4-field entries it includes ",marker".
namespace back_stack
{
    std::string MakeEntry(const std::string & commandPath, const std::string & scriptPath,
                          const std::string & titleKey, const std::string & marker);

    // Drops the last entry when it equals entry (re-entering the same menu).
    std::string PopIfLast(const std::string & stack, const std::string & entry);

    std::string Push(const std::string & stack, const std::string & entry);

    // Shell command that reopens the last entry with the rest as its stack;
    // empty when the stack is empty or the last entry is malformed.
    std::string BackCommand(const std::string & stack, const std::string & optionsLocation);
}

#endif
