//  Copyright (C) 2026 DefKorns
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.

#ifndef APPLICATION_LIFECYCLE_H_
#define APPLICATION_LIFECYCLE_H_

#include <cstdlib>
#include <string>
#include <utility>

// Resume: hand the screen back to the home menu. Leave: something else
// (a relaunched menu, a delete command) already owns it.
enum class ExitAction { Resume, Leave };

// Runs the resume command at most once, from the destructor so it happens
// after main()'s locals (SDL) are released. Undecided means Resume.
class ApplicationLifecycle
{
public:
    ApplicationLifecycle() = default;
    ApplicationLifecycle(const ApplicationLifecycle &) = delete;
    ApplicationLifecycle & operator=(const ApplicationLifecycle &) = delete;

    ~ApplicationLifecycle()
    {
        if(action == ExitAction::Resume && !resumed && !resumeCommand.empty())
        {
            resumed = true;
            std::system(resumeCommand.c_str());
        }
    }

    void SetResumeCommand(std::string command) { resumeCommand = std::move(command); }
    void Decide(ExitAction decision) { action = decision; }

private:
    std::string resumeCommand;
    ExitAction action = ExitAction::Resume;
    bool resumed = false;
};

#endif
