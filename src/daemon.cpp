/**
  * Copyright (C) 2017-2018 CompCom
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 3
  * of the License, or (at your option) any later version.
  */

#include "framework/controller.h"

#include <thread>
#include <chrono>
#include <fstream>
#include <string>

int main()
{
    Controller c(1);
    GameButton button1 = L, button2 = R, button3 = R;
    bool haveCustomCombo = false;

    std::ifstream in("/etc/options_menu/button.cfg");
    if(in.is_open())
    {
        int b1, b2, b3;
        // all 3 must parse - a truncated/empty file falls through to auto-detect below
        if(in >> b1 >> b2 >> b3)
        {
            button1 = (GameButton)b1;
            button2 = (GameButton)b2;
            button3 = (GameButton)b3;
            haveCustomCombo = true;
        }
        in.close();
    }

    if(!haveCustomCombo)
    {
        // no valid user-set combo - NES/Famicom pads have no L/R, fall back to B+Down.
        // matches softwareCheck()'s own dp-shvc check in /etc/preinit.d/b0010_functions
        std::ifstream boardtypeFile("/var/squashfs/etc/clover/boardtype");
        std::string boardtype;
        std::getline(boardtypeFile, boardtype);
        if(boardtype != "dp-shvc")
        {
            button1 = B;
            button2 = DOWN;
            button3 = DOWN;
        }
    }

    const auto waitTime = std::chrono::milliseconds(100);
    const short holdThreshold = 10;
    short holdCount = 0;
    
    for(;;)
    {
        c.Update();
        if(c.PeekButtonStatus(button1) && c.PeekButtonStatus(button2) && c.PeekButtonStatus(button3))
        {
            ++holdCount;
            if(holdCount >= holdThreshold)
            {
                if(system("[ ! -f \"/tmp/options.flag\" ] && [ $(free -m | awk 'NR==3 { print $4; }') -gt 35 ] && /bin/sh /etc/options_menu/scripts/PauseUI.sh") == 0)
                    system("exec /etc/options_menu/options");
                holdCount = 0;
                c.Update();
                c.Reset();
            }
        }
        else if(holdCount)
            holdCount = 0;
        std::this_thread::sleep_for(waitTime);
    }
    
    return 0;
}
