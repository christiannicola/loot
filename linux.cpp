/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#include "linux.h"
#include "platform.h"
#include "types.h"

#include <dlfcn.h> // dlopen, dlsym, dlclose
#include <stdio.h> // fprintf, stderr
#include <sys/mman.h>
#include <unistd.h>

global_variable linux_state GlobalLinuxState;

internal void*
LinuxLoadLibrary(const char* LibName)
{
    void* Handle = NULL;

    Handle = dlopen(LibName, RTLD_NOW | RTLD_LOCAL);

    if(!Handle)
    {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
    }

    return Handle;
}

internal void*
LinuxLoadFunction(void* LibHandle, const char* Name)
{
    void* Symbol = dlsym(LibHandle, Name);

    if(!Symbol)
    {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
    }

    return Symbol;
}

internal void
LinuxUnloadLibrary(void* Handle)
{
    if(!Handle)
    {
        dlclose(Handle);
        Handle = NULL;
    }
}

internal void
LinuxGetBinFileNameAndCWD(linux_state* State)
{
    readlink("/proc/self/cwd", State->WorkingDirectory, ArrayCount(State->WorkingDirectory) - 1);
    readlink("/proc/self/exe", State->BinFilePath, ArrayCount(State->BinFilePath) - 1);
}

internal void
LinuxBuildPathFromFileAndCWD(linux_state* State, const char* FileName, char* Dest)
{
    sprintf(Dest, "%s/%s", State->WorkingDirectory, FileName);
}

int
main(int argc, char* argv[])
{
    linux_state* State = &GlobalLinuxState;
    LinuxGetBinFileNameAndCWD(State);

    char SourceGameCodeLibFullPath[LINUX_STATE_FILE_NAME_COUNT];
    LinuxBuildPathFromFileAndCWD(State, "libloot.so", SourceGameCodeLibFullPath);

    void* LibraryHandle = LinuxLoadLibrary(SourceGameCodeLibFullPath);

    linux_game_code GameCode;
    *(void**)(&GameCode.UpdateAndRender) = LinuxLoadFunction(LibraryHandle, "GameUpdateAndRender");

    game_memory Memory = {};
    GameCode.UpdateAndRender(&Memory);

    LinuxUnloadLibrary(LibraryHandle);
}
