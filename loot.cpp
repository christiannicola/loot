/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#include "loot.h"
#include <cstdio>

internal void GameOutputSound(game_state* GameState, game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16* SampleOut = SoundBuffer->Samples;

	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		real32 SineValue = sinf(GameState->tSine);
		auto SampleValue = (int16)((int16)SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
		if (GameState->tSine > 2.0f * Pi32)
		{
			GameState->tSine -= 2.0f * Pi32;
		}
	}
}

internal void RenderWeirdGradient(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
	auto* Row = (uint8*)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		auto* Pixel = (uint32*)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			auto Blue = (uint8)(X + BlueOffset);
			auto Green = (uint8)(Y + GreenOffset);

			*Pixel++ = ((Green << 16) | Blue);
		}

		Row += Buffer->Pitch;
	}
}

extern "C" LOOT_API GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	auto* GameState = (game_state*)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		GameState->ToneHz = 512;
		GameState->tSine = 0.0f;
		Memory->IsInitialized = true;
	}

	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

extern "C" LOOT_API GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	auto* GameState = (game_state*)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}