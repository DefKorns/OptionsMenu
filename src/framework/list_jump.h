//  Copyright (C) 2026 DefKorns
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.

#ifndef LIST_JUMP_H_
#define LIST_JUMP_H_

#include <string>
#include <vector>

class Controller;

enum class ListJump { None, PageUp, PageDown, LetterPrev, LetterNext };

// L/R walk a list by initial letter; D-pad Left/Right page on a tap and walk
// the letters while held. Poll every frame, it keeps the held timers of the
// buttons it reads.
class ListJumpInput
{
public:
    ListJump Poll(Controller & controller, unsigned int nowMs);

private:
    unsigned int lastLetterJumpMs = 0;
};

// Row (an index into the whole list) of the previous or next initial letter.
// labels[i] is row firstRow+i; rows before firstRow are not entries (select-all).
int LetterJumpRow(const std::vector<std::string> & labels, int firstRow, int row, int direction);

// Moves listOffset and currentId (a row inside the window of displayCount rows).
void ApplyListJump(ListJump jump, const std::vector<std::string> & labels, int firstRow, int displayCount, int & listOffset, int & currentId);

#endif
