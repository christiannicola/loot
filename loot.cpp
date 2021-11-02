/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#include "loot.h"
#include <stdio.h>

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  fprintf (stdout, "does this work?\n");
}