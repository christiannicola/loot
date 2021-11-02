/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#include "linux.h"
#include "platform.h"

#include <dlfcn.h> // dlopen, dlsym, dlclose
#include <stdio.h> // fprintf, stderr

internal void *LinuxLoadLibrary(const char *LibName) {
  void *Handle = nullptr;

  Handle = dlopen(LibName, RTLD_NOW | RTLD_LOCAL);

  if (!Handle) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
  }

  return Handle;
}

internal void *LinuxLoadFunction(void *LibHandle, const char *Name) {
  void *Symbol = dlsym(LibHandle, Name);
  if (!Symbol) {
    fprintf(stderr, "dlsym failed: %s\n", dlerror());
  }

  return Symbol;
}

int main(int argc, char *argv[]) {
  void *LibraryHandle = LinuxLoadLibrary("liblibloot.so.so");

  Assert(LibraryHandle)

      game_update_and_render *UpdateAndRender = nullptr;

  *(void **)UpdateAndRender =
      LinuxLoadFunction(LibraryHandle, "GameUpdateAndRender");

  Assert(UpdateAndRender)

      game_memory Memory = {};
  UpdateAndRender(&Memory);

  return 0;
}
