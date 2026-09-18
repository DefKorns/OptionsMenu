/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
  */

#include "framework/controller.h"
#include "localization.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <unistd.h>

static const GameButton AllButtons[] = { A, B, X, Y, L, R, SELECT, START, LEFT, RIGHT, UP, DOWN };

static const char * ButtonName(GameButton button)
{
    switch(button)
    {
        case A: return "A";
        case B: return "B";
        case X: return "X";
        case Y: return "Y";
        case L: return "L";
        case R: return "R";
        case SELECT: return "SELECT";
        case START: return "START";
        case LEFT: return "LEFT";
        case RIGHT: return "RIGHT";
        case UP: return "UP";
        default: return "DOWN";
    }
}

// drops any button already held, so the next WaitForPress doesn't catch the
// tail of a press that was meant for the previous step
static void ConsumeHeldButtons(Controller & c)
{
    c.Update();
    for(GameButton b : AllButtons)
        c.GetButtonStatus(b);
}

static GameButton WaitForPress(Controller & c)
{
    for(;;)
    {
        c.Update();
        for(GameButton b : AllButtons)
            if(c.GetButtonStatus(b))
                return b;
        usleep(50000);
    }
}

int main()
{
    // RunCommand's popen pipe isn't a tty - stdio defaults to fully buffered
    // there, so lines can sit unflushed instead of appearing as they're printed
    setvbuf(stdout, nullptr, _IONBF, 0);

    LoadLanguageFromConfig("/etc/options_menu/");

    Controller c(1);

    static const char * const intro[] = {
        "CHANGE_COMBO_INTRO_1", "CHANGE_COMBO_INTRO_2", "CHANGE_COMBO_INTRO_3", "CHANGE_COMBO_CONTINUE"
    };
    for(const char * key : intro)
        std::cout << Translate(key) << std::endl;
    ConsumeHeldButtons(c);
    GameButton gate;
    do { gate = WaitForPress(c); } while(gate != A && gate != B);
    if(gate == B)
        return 0;

    static const char * const prompts[3] = {
        "CHANGE_COMBO_PRESS_1",
        "CHANGE_COMBO_PRESS_2",
        "CHANGE_COMBO_PRESS_3"
    };

    for(;;)
    {
        GameButton chosen[3];
        for(int i = 0; i < 3; ++i)
        {
            std::cout << Translate(prompts[i]) << std::endl;
            ConsumeHeldButtons(c);
            chosen[i] = WaitForPress(c);
        }

        // button names (A, B, X...) stay untranslated, same as the footer badges
        std::cout << Translate("CHANGE_COMBO_CHOSEN") << " " << ButtonName(chosen[0]) << " + " << ButtonName(chosen[1]) << " + " << ButtonName(chosen[2]) << std::endl;
        std::cout << Translate("CHANGE_COMBO_CONFIRM") << std::endl;
        ConsumeHeldButtons(c);
        if(WaitForPress(c) == A)
        {
            // write to a temp file and rename into place - an interrupted write
            // must never leave button.cfg truncated/empty for daemon.cpp to trip over
            const char * const path = "/etc/options_menu/button.cfg";
            const std::string tmpPath = std::string(path) + ".tmp";
            std::ofstream out(tmpPath);
            if(out << (int)chosen[0] << " " << (int)chosen[1] << " " << (int)chosen[2])
            {
                out.close();
                std::rename(tmpPath.c_str(), path);
                std::cout << Translate("CHANGE_COMBO_DONE") << std::endl;
            }
            else
                std::cerr << Translate("CHANGE_COMBO_WRITE_ERROR") << std::endl;
            break;
        }
    }

    return 0;
}
