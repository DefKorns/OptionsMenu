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
namespace back_stack
{
    std::string MakeEntry(const std::string & commandPath, const std::string & scriptPath,
                          const std::string & titleKey, const std::string & marker);

    std::string PopIfLast(const std::string & stack, const std::string & entry);

    std::string Push(const std::string & stack, const std::string & entry);

    // empty when the stack is empty or the last entry is malformed
    std::string BackCommand(const std::string & stack, const std::string & optionsLocation);
}

#endif
