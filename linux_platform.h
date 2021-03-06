/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

#include "loot.h"

#define LINUX_STATE_FILE_NAME_COUNT 512

struct linux_state
{
    char BinFilePath[LINUX_STATE_FILE_NAME_COUNT];
    char WorkingDirectory[LINUX_STATE_FILE_NAME_COUNT];
};

struct linux_game_code
{
    void* GameLibHandle;
    ino_t GameLibID;

    game_update_and_render* UpdateAndRender;

    bool IsValid;
};

struct linux_offscreen_buffer
{
    uint32 Width;
    uint32 Height;
    uint32 Pitch;
    uint8* Memory;
};

#endif//LINUX_PLATFORM_H