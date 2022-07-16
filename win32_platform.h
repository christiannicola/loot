/*
 * Copyright (c) 2022, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

#include "loot.h"

#include <Xinput.h>
#include <cstdio>
#include <dsound.h>
#include <malloc.h>
#include <windows.h>

struct win32_offscreen_buffer {
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension {
	int Width;
	int Height;
};

struct win32_sound_output {
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
};

struct win32_debug_time_marker {
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD ExpectedFlipPlayCursor;
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};

struct win32_game_code {
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;

	game_update_and_render* UpdateAndRender;

	bool IsValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

struct win32_replay_buffer {
	HANDLE FileHandle;
	HANDLE MemoryMap;
	char FileName[WIN32_STATE_FILE_NAME_COUNT];
	void* MemoryBlock;
};

struct win32_state {
	uint64 TotalSize;
	void* GameMemoryBlock;
	win32_replay_buffer ReplayBuffers[4];

	HANDLE RecordingHandle;
	int InputRecordingIndex;

	HANDLE PlaybackHandle;
	int InputPlayingIndex;

	char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
	char* OnePastLastEXEFileNameSlash;
};

#endif//WIN32_PLATFORM_H
