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

extern char ** environ;

enum class ExitAction { Resume, Leave };

// Resume runs from the destructor, after main()'s locals (SDL) are released.
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
            State().resume = 0;
            std::system(("/bin/sh " + script).c_str());
        }
    }

    void SetResumeScript(std::string path)
    {
        script = std::move(path);
        State().argv[1] = script.c_str();
    }

    void Decide(ExitAction decision)
    {
        action = decision;
        State().resume = decision == ExitAction::Resume ? 1 : 0;
    }

    // handler may only call async-signal-safe functions; vfork skips the atfork handlers
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
        if(!state.fired && state.resume && state.argv[1])
        {
            state.fired = 1;
            if(vfork() == 0)
            {
                execve("/bin/sh", const_cast<char * const *>(state.argv), environ);
                _exit(127);
            }
        }
        _exit(128 + sig);
    }

    struct SignalState
    {
        const char * argv[3] = { "sh", nullptr, nullptr };
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
