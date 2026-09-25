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

#include <array>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <string>
#include <unistd.h>

namespace
{
    const std::string OptionsRoot = "/etc/options_menu/";
    const std::string ButtonConfigPath = OptionsRoot + "button.cfg";
    constexpr useconds_t PollIntervalUs = 50000;
    constexpr size_t ComboLength = 3;

    const std::array<GameButton, 12> AllButtons{ { A, B, X, Y, L, R, SELECT, START, LEFT, RIGHT, UP, DOWN } };

    const char * ButtonName(GameButton button)
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
            case DOWN: return "DOWN";
        }
        return "?";
    }

    // drops any button already held, so the next WaitForPress doesn't catch the
    // tail of a press that was meant for the previous step
    void ConsumeHeldButtons(Controller & controller)
    {
        controller.Update();
        for(const GameButton button : AllButtons)
            controller.GetButtonStatus(button);
    }

    GameButton WaitForPress(Controller & controller)
    {
        for(;;)
        {
            controller.Update();
            for(const GameButton button : AllButtons)
                if(controller.GetButtonStatus(button))
                    return button;
            usleep(PollIntervalUs);
        }
    }

    void PrintLines(const std::initializer_list<const char *> keys)
    {
        for(const char * key : keys)
            std::cout << Translate(key) << std::endl;
    }

    // temp file + rename: an interrupted write must never leave button.cfg
    // truncated for daemon.cpp to trip over
    bool SaveCombo(const std::array<GameButton, ComboLength> & combo)
    {
        const std::string tmpPath = ButtonConfigPath + ".tmp";
        {
            std::ofstream out(tmpPath);
            if(!(out << static_cast<int>(combo[0]) << " " << static_cast<int>(combo[1]) << " " << static_cast<int>(combo[2])))
                return false;
        }
        return std::rename(tmpPath.c_str(), ButtonConfigPath.c_str()) == 0;
    }
}

int main()
{
    // RunCommand's popen pipe isn't a tty - stdio defaults to fully buffered
    // there, so lines can sit unflushed instead of appearing as they're printed
    setvbuf(stdout, nullptr, _IONBF, 0);

    LoadLanguageFromConfig(OptionsRoot);

    Controller controller(1);

    PrintLines({ "CHANGE_COMBO_INTRO_1", "CHANGE_COMBO_INTRO_2", "CHANGE_COMBO_INTRO_3", "CHANGE_COMBO_CONTINUE" });
    ConsumeHeldButtons(controller);
    GameButton choice = WaitForPress(controller);
    while(choice != A && choice != B)
        choice = WaitForPress(controller);
    if(choice == B)
        return 0;

    const std::array<const char *, ComboLength> prompts{ { "CHANGE_COMBO_PRESS_1", "CHANGE_COMBO_PRESS_2", "CHANGE_COMBO_PRESS_3" } };

    for(;;)
    {
        std::array<GameButton, ComboLength> combo{};
        for(size_t i = 0; i < ComboLength; ++i)
        {
            std::cout << Translate(prompts[i]) << std::endl;
            ConsumeHeldButtons(controller);
            combo[i] = WaitForPress(controller);
        }

        // button names (A, B, X...) stay untranslated, same as the footer badges
        std::cout << Translate("CHANGE_COMBO_CHOSEN") << " " << ButtonName(combo[0]) << " + " << ButtonName(combo[1]) << " + " << ButtonName(combo[2]) << std::endl;
        std::cout << Translate("CHANGE_COMBO_CONFIRM") << std::endl;
        ConsumeHeldButtons(controller);
        if(WaitForPress(controller) == A)
        {
            if(SaveCombo(combo))
                std::cout << Translate("CHANGE_COMBO_DONE") << std::endl;
            else
                std::cerr << Translate("CHANGE_COMBO_WRITE_ERROR") << std::endl;
            break;
        }
    }

    return 0;
}
