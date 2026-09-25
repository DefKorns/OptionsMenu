/**
 * Copyright (C) 2017-2018 CompCom
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */
#ifndef CONTROLLER_H_
#define CONTROLLER_H_
#include <map>

enum GameButton : uint16_t { A=304, B=305, X=307, Y=308, L=310, R=311, SELECT=314, START=315, LEFT=704, RIGHT=705, UP=706, DOWN=707  };

struct ButtonEvent
{
    char unk0[8];
    unsigned short unk1;
    GameButton button;
    unsigned short pressed;
    unsigned short zero;
};

class Controller
{
private:
    int fd = -1;
    ButtonEvent buttonBuffer[10];
    std::map<GameButton,bool> buttons;
    std::map<GameButton,unsigned int> repeatPressedAt, repeatLastFired;
    std::map<GameButton,bool> repeatActive;
    std::map<GameButton,unsigned int> heldPressedAt;
    std::map<GameButton,bool> heldActive;
#ifndef __arm__
    unsigned char prevKeys[512] = {0}; // keyboard fallback for desktop testing builds
#endif
public:
    Controller(int id);
    ~Controller();
    bool PeekButtonStatus(GameButton button);
    bool GetButtonStatus(GameButton button);
    // fires once on press, then repeats after an initial delay while held
    bool HeldRepeat(GameButton button);
    // ms this button has been continuously held, 0 if not currently held
    unsigned int HeldMillis(GameButton button);
    void Update();
    void Reset();
};

#endif
