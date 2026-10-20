//  Copyright (C) 2026 DefKorns
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.

#include "list_jump.h"
#include "controller.h"

#include <algorithm>
#include <cctype>

namespace
{
    const unsigned int LetterHoldMs = 450;
    const unsigned int LetterRepeatMs = 200;
    // only the press fires a page, not the key repeat that follows
    const unsigned int PageTapMs = 200;

    // first ASCII letter, upper-cased; digits and anything else share '#'
    char Initial(const std::string & label)
    {
        for(char c : label)
            if(std::isalpha(static_cast<unsigned char>(c)))
                return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            else if(std::isdigit(static_cast<unsigned char>(c)))
                break;
        return '#';
    }

    int GroupStart(const std::vector<std::string> & labels, int index)
    {
        const char initial = Initial(labels[index]);
        while(index > 0 && Initial(labels[index - 1]) == initial)
            --index;
        return index;
    }
}

ListJump ListJumpInput::Poll(Controller & controller, unsigned int nowMs)
{
    // every timer is read each frame: they only reset when sampled while released
    const bool lPressed = controller.HeldRepeat(L);
    const bool rPressed = controller.HeldRepeat(R);
    const bool leftStep = controller.HeldRepeat(LEFT);
    const bool rightStep = controller.HeldRepeat(RIGHT);
    const unsigned int leftHeldMs = controller.HeldMillis(LEFT);
    const unsigned int rightHeldMs = controller.HeldMillis(RIGHT);

    if(lPressed)
        return ListJump::LetterPrev;
    if(rPressed)
        return ListJump::LetterNext;

    auto Dpad = [&](bool step, unsigned int heldMs, ListJump page, ListJump letter)
    {
        if(heldMs >= LetterHoldMs)
        {
            if(nowMs - lastLetterJumpMs < LetterRepeatMs)
                return ListJump::None;
            lastLetterJumpMs = nowMs;
            return letter;
        }
        return (step && heldMs < PageTapMs) ? page : ListJump::None;
    };
    const ListJump left = Dpad(leftStep, leftHeldMs, ListJump::PageUp, ListJump::LetterPrev);
    return left != ListJump::None ? left : Dpad(rightStep, rightHeldMs, ListJump::PageDown, ListJump::LetterNext);
}

int LetterJumpRow(const std::vector<std::string> & labels, int firstRow, int row, int direction)
{
    const int count = static_cast<int>(labels.size());
    if(count == 0)
        return row;

    if(direction > 0)
    {
        if(row < firstRow)
            return firstRow;
        const int index = row - firstRow;
        const char initial = Initial(labels[index]);
        for(int i = index + 1; i < count; ++i)
            if(Initial(labels[i]) != initial)
                return firstRow + i;
        return firstRow + count - 1;
    }

    if(row <= firstRow)
        return 0;
    const int index = row - firstRow;
    const int start = GroupStart(labels, index);
    if(index > start)
        return firstRow + start;
    return start == 0 ? 0 : firstRow + GroupStart(labels, start - 1);
}

void ApplyListJump(ListJump jump, const std::vector<std::string> & labels, int firstRow, int displayCount, int & listOffset, int & currentId)
{
    const int totalRows = static_cast<int>(labels.size()) + firstRow;
    if(jump == ListJump::None || totalRows <= 0 || displayCount <= 0)
        return;
    const int maxOffset = std::max(0, totalRows - displayCount);
    const int row = listOffset + currentId;

    if(jump == ListJump::LetterPrev || jump == ListJump::LetterNext)
    {
        const int target = LetterJumpRow(labels, firstRow, row, jump == ListJump::LetterNext ? 1 : -1);
        // a target already on screen only moves the cursor, otherwise it goes to the top
        if(target < listOffset || target >= listOffset + displayCount)
            listOffset = std::min(maxOffset, target);
        currentId = target - listOffset;
        return;
    }

    const int delta = jump == ListJump::PageDown ? displayCount : -displayCount;
    const int target = std::min(totalRows - 1, std::max(0, row + delta));
    listOffset = std::min(maxOffset, std::max(0, listOffset + delta));
    currentId = target - listOffset;
    if(currentId < 0)
    {
        listOffset = target;
        currentId = 0;
    }
    else if(currentId >= displayCount)
    {
        listOffset = target - displayCount + 1;
        currentId = displayCount - 1;
    }
}
