/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#pragma once

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32
CharLength(char* String)
{
    uint32 Count = 0;
    if(String)
    {
        while(*String++)
        {
            ++Count;
        }
    }

    return (Count);
}