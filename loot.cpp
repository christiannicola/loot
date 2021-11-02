/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#include "loot.h"

static void
GameUpdateAndRender (game_offscreen_buffer* Buffer)
{
  // NOTE (c.nicola): debug code for platform architecture
  uint8* Row = (uint8*)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y)
  {
    uint8* Pixel = reinterpret_cast<uint8*> ((uint32*)Row);
    for (int X = 0; X < Buffer->Width; ++X)
    {
      uint8 Blue = X;
      uint8 Green = Y;

      *Pixel++ = ((Green << 8) | Blue);
    }

    Row += Buffer->Pitch;
  }
}
