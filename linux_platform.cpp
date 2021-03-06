/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#include "linux_platform.h"

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

global_variable linux_state GlobalLinuxState;
global_variable bool GlobalIsRunning;
global_variable linux_offscreen_buffer GlobalBackBuffer;
global_variable int GlobalWindowPositionX;
global_variable int GlobalWindowPositionY;

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
    if(Handle)
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

internal inline ino_t
LinuxFileID(char* FileName)
{
    struct stat Attr = {};
    if (stat(FileName, &Attr))
    {
        Attr.st_ino = 0;
    }

    return Attr.st_ino;
}

internal bool
LinuxLoadGameCode(linux_game_code* GameCode, char* LibName, ino_t FileID)
{
    if(GameCode->GameLibID != FileID)
    {
        LinuxUnloadLibrary(GameCode->GameLibHandle);
        GameCode->GameLibID = FileID;
        GameCode->IsValid = false;

        GameCode->GameLibHandle = LinuxLoadLibrary(LibName);

        if(GameCode->GameLibHandle)
        {
            *(void**)(&GameCode->UpdateAndRender) = LinuxLoadFunction(GameCode->GameLibHandle, "GameUpdateAndRender");

            GameCode->IsValid = GameCode->UpdateAndRender != nullptr;
        }
    }

    if(!GameCode->IsValid)
    {
        LinuxUnloadLibrary(GameCode->GameLibHandle);
        GameCode->GameLibID = 0;
        GameCode->UpdateAndRender = nullptr;
    }

    return (GameCode->IsValid);
}

internal void
LinuxUnloadGameCode(linux_game_code* GameCode)
{
    LinuxUnloadLibrary(GameCode->GameLibHandle);

    GameCode->GameLibID = 0;
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = nullptr;
}

internal linux_offscreen_buffer
LinuxCreateOffscreenBuffer(uint32 Width, uint32 Height)
{
    linux_offscreen_buffer OffscreenBuffer = {};
    OffscreenBuffer.Width = Width;
    OffscreenBuffer.Height = Height;
    OffscreenBuffer.Pitch = Align16(OffscreenBuffer.Width * BYTES_PER_PIXEL);

    uint32 Size = OffscreenBuffer.Pitch * OffscreenBuffer.Height;
    OffscreenBuffer.Memory = (uint8*)mmap(NULL, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(OffscreenBuffer.Memory == MAP_FAILED)
    {
        // TODO(c.nicola): Replace with logging
        fprintf(stderr, "could not map offscreen buffer into memory");

        OffscreenBuffer.Width = 0;
        OffscreenBuffer.Height = 0;
        OffscreenBuffer.Pitch = 0;
        OffscreenBuffer.Memory = 0;
    }

    return OffscreenBuffer;
}

internal void
LinuxResizeOffscreenBuffer(linux_offscreen_buffer* Buffer, uint32 Width, uint32 Height)
{
    if(Buffer->Memory)
    {
        munmap(Buffer->Memory, Buffer->Pitch * Buffer->Height);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->Pitch = Buffer->Width * BYTES_PER_PIXEL;

    uint32 Size = Buffer->Pitch * Buffer->Height;
    Buffer->Memory = (uint8*)mmap(NULL, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(Buffer->Memory == MAP_FAILED)
    {
        // TODO(c.nicola): Replace with logging
        fprintf(stderr, "could not map offscreen buffer into memory");

        Buffer->Width = 0;
        Buffer->Height = 0;
        Buffer->Pitch = 0;
        Buffer->Memory = 0;
    }
}

internal int LinuxCheckOpenGL(void)
{
    Display *TempDisplay = XOpenDisplay(nullptr);
    int ErrorBase, EventBase;

    if (TempDisplay)
    {
        int ReturnCode = glXQueryExtension(TempDisplay, &ErrorBase, &EventBase);
        XCloseDisplay(TempDisplay);

        return ReturnCode;
    }

    return 0;
}

internal GLXFBConfig*
LinuxGetOpenGLFramebufferConfig(Display* InternalDisplay)
{
    // clang-format off
    int VisualAttribs[] =
    {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, True,
        None
    };
    // clang-format on

    int FrameBufferCount;
    GLXFBConfig* FramebufferConfig = glXChooseFBConfig(InternalDisplay, DefaultScreen(InternalDisplay), VisualAttribs, &FrameBufferCount);

    Assert(FrameBufferCount >= 1)

    return FramebufferConfig;
}

internal void
LinuxProcessPendingMessages(Display* InternalDisplay, Atom WmDeleteWindow)
{
    while(GlobalIsRunning && XPending(InternalDisplay))
    {
        XEvent Event;
        XNextEvent(InternalDisplay, &Event);

        switch (Event.type)
        {
        case DestroyNotify:
        {
            GlobalIsRunning = false;
        } break;
        case ClientMessage:
        {
            if ((Atom)Event.xclient.data.l[0] == WmDeleteWindow)
            {
                GlobalIsRunning = false;
            }
        }
        }
    }
}

int
main(int argc, char* argv[])
{
    GlobalIsRunning = true;
    GlobalWindowPositionX = 0;
    GlobalWindowPositionY = 0;
    GlobalBackBuffer = LinuxCreateOffscreenBuffer(1920, 1080);

    Display* InternalDisplay = XOpenDisplay(0);

    if(!InternalDisplay)
    {
        fprintf(stderr, "could not create XDisplay");
        return -1;
    }

    Assert(LinuxCheckOpenGL())

    GLXFBConfig* FramebufferConfigs = LinuxGetOpenGLFramebufferConfig(InternalDisplay);
    GLXFBConfig FrameBufferConfig = FramebufferConfigs[0];

    XFree(FramebufferConfigs);

    XVisualInfo* VisualInfo = glXGetVisualFromFBConfig(InternalDisplay, FrameBufferConfig);

    if(!VisualInfo)
    {
        fprintf(stderr, "could not create XVisualInfo");
        return -1;
    }

    VisualInfo->screen = DefaultScreen(InternalDisplay);

    Window Root = RootWindow(InternalDisplay, VisualInfo->screen);

    XSetWindowAttributes WindowAttribs = {};
    WindowAttribs.colormap = XCreateColormap(InternalDisplay, Root, VisualInfo->visual, AllocNone);
    WindowAttribs.border_pixel = 0;
    WindowAttribs.event_mask = (StructureNotifyMask | PropertyChangeMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask);

    Window GLWindow;
    GLWindow = XCreateWindow(InternalDisplay, Root, GlobalWindowPositionX, GlobalWindowPositionY, GlobalBackBuffer.Width, GlobalBackBuffer.Height, 0, VisualInfo->depth,
                             InputOutput, VisualInfo->visual, CWBorderPixel | CWColormap | CWEventMask, &WindowAttribs);

    if (!GLWindow)
    {
        fprintf(stderr, "could not create GLWindow");
        return -1;
    }

    XSizeHints SizeHints = {};
    SizeHints.x = GlobalWindowPositionX;
    SizeHints.y = GlobalWindowPositionY;
    SizeHints.width = GlobalBackBuffer.Width;
    SizeHints.height = GlobalBackBuffer.Height;
    SizeHints.flags = USSize | USPosition;

    XSetNormalHints(InternalDisplay, GLWindow, &SizeHints);
    XSetStandardProperties(InternalDisplay, GLWindow, "Loot", "glsync test", None, nullptr, 0, &SizeHints);

    Atom WMDeleteWindow = XInternAtom(InternalDisplay, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(InternalDisplay, GLWindow, &WMDeleteWindow, 1);
    XMapRaised(InternalDisplay, GLWindow);

	LinuxResizeOffscreenBuffer(&GlobalBackBuffer, 1280, 720);
    LinuxGetBinFileNameAndCWD(&GlobalLinuxState);

    char SourceGameCodeLibFullPath[LINUX_STATE_FILE_NAME_COUNT];
    LinuxBuildPathFromFileAndCWD(&GlobalLinuxState, "libgame.so", SourceGameCodeLibFullPath);

    linux_game_code Game = {};

    LinuxLoadGameCode(&Game, SourceGameCodeLibFullPath, LinuxFileID(SourceGameCodeLibFullPath));

    game_memory GameMemory = {};
	GameMemory.PermanentStorageSize = Megabytes(64);
	GameMemory.TransientStorageSize = Megabytes(64);

	uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

	GameMemory.PermanentStorage = mmap(nullptr, TotalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

	game_offscreen_buffer Buffer = {};
	Buffer.Memory = GlobalBackBuffer.Memory;
	Buffer.Width = GlobalBackBuffer.Width;
	Buffer.Height = GlobalBackBuffer.Height;
	Buffer.Pitch = GlobalBackBuffer.Pitch;

    while(GlobalIsRunning)
    {
        // NOTE (c.nicola): Check if game library needs to be reloaded
        ino_t GameLibID = LinuxFileID(SourceGameCodeLibFullPath);
        bool LibNeedsToBeReloaded = (GameLibID != Game.GameLibID);

        if (LibNeedsToBeReloaded)
        {
            bool IsValid = false;
            for (uint32 LoadTryIndex = 0; !IsValid && (LoadTryIndex < 100); ++LoadTryIndex)
            {
                fprintf(stdout, "try reloading lib, attempt %d\n", LoadTryIndex);
                IsValid = LinuxLoadGameCode(&Game, SourceGameCodeLibFullPath, GameLibID);
                usleep(100000);
            }
        }

        LinuxProcessPendingMessages(InternalDisplay, WMDeleteWindow);

        Game.UpdateAndRender(&GameMemory, &Buffer);
    }

    LinuxUnloadGameCode(&Game);
}
