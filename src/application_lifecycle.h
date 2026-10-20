//  Copyright (C) 2026 DefKorns
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.

#ifndef APPLICATION_LIFECYCLE_H_
#define APPLICATION_LIFECYCLE_H_

#include <csignal>
#include <cstdlib>
#include <string>
#include <utility>
#include <sys/types.h>
#include <unistd.h>

// Resume: hand the screen back to the home menu. Leave: something else
// (a relaunched menu, a delete command) already owns it.
enum class ExitAction { Resume, Leave };

// Runs the resume script at most once, from the destructor so it happens
// after main()'s locals (SDL) are released. Undecided means Resume.
class ApplicationLifecycle
{
public:
    ApplicationLifecycle() = default;
    ApplicationLifecycle(const ApplicationLifecycle &) = delete;
    ApplicationLifecycle & operator=(const ApplicationLifecycle &) = delete;

    ~ApplicationLifecycle()
    {
        if(action == ExitAction::Resume && !resumed && !script.empty())
        {
            resumed = true;
            std::system(("/bin/sh " + script).c_str());
        }
    }

    void SetResumeScript(std::string path)
    {
        script = std::move(path);
        State().script = script.c_str();
    }

    void Decide(ExitAction decision)
    {
        action = decision;
        State().resume = decision == ExitAction::Resume ? 1 : 0;
    }

    // SIGTERM/SIGHUP (killall, init stop): resume the UI, then die. Only
    // async-signal-safe calls run in the handler; SIGKILL and crashes stay uncovered.
    void InstallSignalHandlers()
    {
        struct sigaction sa;
        sa.sa_handler = OnTerminate;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGHUP, &sa, nullptr);
    }

private:
    static void OnTerminate(int sig)
    {
        SignalState & state = State();
        if(!state.fired && state.resume && state.script)
        {
            state.fired = 1;
            if(fork() == 0)
            {
                execl("/bin/sh", "sh", state.script, static_cast<char *>(nullptr));
                _exit(127);
            }
        }
        _exit(128 + sig);
    }

    struct SignalState
    {
        const char * script = nullptr;
        volatile sig_atomic_t resume = 1;
        volatile sig_atomic_t fired = 0;
    };

    static SignalState & State()
    {
        static SignalState state;
        return state;
    }

    std::string script;
    ExitAction action = ExitAction::Resume;
    bool resumed = false;
};

#endif
