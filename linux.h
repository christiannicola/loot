/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#pragma once

#include "platform.h"

#define LINUX_STATE_FILE_NAME_COUNT 512

struct linux_state
{
    char BinFilePath[LINUX_STATE_FILE_NAME_COUNT];
    char WorkingDirectory[LINUX_STATE_FILE_NAME_COUNT];
};

struct linux_game_code
{
    game_update_and_render* UpdateAndRender;
};

struct linux_offscreen_buffer
{
    uint32 Width;
    uint32 Height;
    uint32 Pitch;
    uint8* Memory;
};