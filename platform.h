/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#pragma once

#include <stdint.h>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global_variable static

#define BITMAP_BYTES_PER_PIXEL 4

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

#if ENABLE_ASSERTS
#if __has_builtin(__builtin_trap)
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    __builtin_trap();                                                          \
  }
#else
// NOTE (c.nicola): Writing to nullptr == ghetto __builtin_trap
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#endif
#endif

typedef struct game_memory {
  void *Memory;
} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
