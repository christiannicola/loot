/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#pragma once

#include <stdint.h>
#include <cstdlib>

// clang-format off
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if ENABLE_ASSERTS
#if __has_builtin(__builtin_trap)
#define Assert(Expression) if(!(Expression)) {__builtin_trap();}
#else
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#endif

#else
#define Assert(Expression)
#endif

#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global_variable static

#define BYTES_PER_PIXEL 4
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)
// clang-format on

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

typedef struct game_memory
{
    void* Memory;
} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
