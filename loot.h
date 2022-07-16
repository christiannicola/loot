/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#ifndef LOOT_H
#define LOOT_H

#include <cstdint>
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

#ifndef LOOT_API
#define LOOT_API __declspec(dllexport)
#endif

#define Pi32 3.14159265359f
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

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

struct game_state
{
	int ToneHz;
	int GreenOffset;
	int BlueOffset;

	real32 tSine;
};

struct game_offscreen_buffer
{
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_memory
{
	bool IsInitialized;

	uint64 PermanentStorageSize;
	void *PermanentStorage;

	uint64 TransientStorageSize;
	void *TransientStorage;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRendererStub)
{
}

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{
}

#endif//LOOT_H
