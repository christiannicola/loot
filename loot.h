/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

struct game_offscreen_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
};

static void GameUpdateAndRender (game_offscreen_buffer* Buffer);