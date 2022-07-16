/*
 * Copyright (c) 2021, Christian Nicola <hi@chrisnicola.de>
 * All rights reserved.
 */

#include "win32_platform.h"
#include "loot.h"

global_variable bool GlobalRunning;
global_variable bool GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (XInputLibrary == nullptr)
	{
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

	if (XInputLibrary == nullptr)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		if (XInputGetState == nullptr)
		{
			XInputGetState = XInputGetStateStub;
		}

		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
		if (XInputSetState == nullptr)
		{
			XInputSetState = XInputSetStateStub;
		}
	}
	else
	{
		// NOTE (c.nicola): Logging
	}
}

internal void Win32InitDirectSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	Assert(DSoundLibrary != nullptr);

	auto* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
	LPDIRECTSOUND DirectSound = {};

	Assert(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(nullptr, &DirectSound, nullptr)));

	WAVEFORMATEX WaveFormat = {};
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.nChannels = 2;
	WaveFormat.nSamplesPerSec = SamplesPerSecond;
	WaveFormat.wBitsPerSample = 16;
	WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
	WaveFormat.cbSize = 0;

	if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
	{
		DSBUFFERDESC BufferDescription = {};
		BufferDescription.dwSize = sizeof(BufferDescription);
		BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

		LPDIRECTSOUNDBUFFER PrimaryBuffer = {};

		if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, nullptr)))
		{
			HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
			Assert(SUCCEEDED(Error));
		}
	}

	DSBUFFERDESC BufferDescription = {};
	BufferDescription.dwSize = sizeof(BufferDescription);
	BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
	BufferDescription.dwBufferBytes = BufferSize;
	BufferDescription.lpwfxFormat = &WaveFormat;
	HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, nullptr);

	Assert(SUCCEEDED(Error));
}

internal void Win32GetEXEFileName(win32_state* State)
{
	State->OnePastLastEXEFileNameSlash = State->EXEFileName;

	for (char* Scan = State->EXEFileName; *Scan; ++Scan)
	{
		if (*Scan == '\\')
		{
			State->OnePastLastEXEFileNameSlash = Scan + 1;
		}
	}
}

internal void CatStrings(size_t SourceACount, char* SourceA, size_t SourceBCount, char* SourceB, size_t DestCount, char* Dest)
{
	for (int Index = 0; Index < SourceACount; ++Index)
	{
		*Dest++ = *SourceA++;
	}

	for (int Index = 0; Index < SourceBCount; ++Index)
	{
		*Dest++ = *SourceB++;
	}

	*Dest++ = 0;
}

internal int StringLength(char* String)
{
	int Count = 0;
	while (*String++)
	{
		++Count;
	}

	return (Count);
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;

	Buffer->Memory = VirtualAlloc(nullptr, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void Win32BuildEXEPathFileName(win32_state* State, char* FileName, int DestCount, char* Dest)
{
	CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName, StringLength(FileName), FileName, DestCount, Dest);
}

inline FILETIME Win32GetLastWriteTime(char* FileName)
{
	FILETIME LastWriteTime = {};
	WIN32_FILE_ATTRIBUTE_DATA Data;

	if (GetFileAttributesEx(FileName, GetFileExInfoStandard, &Data))
	{
		LastWriteTime = Data.ftLastWriteTime;
	}

	return (LastWriteTime);
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result = {};

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return (Result);
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	StretchDIBits(DeviceContext,
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal void Win32ClearBuffer(win32_sound_output* SoundOuput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOuput->SecondaryBufferSize,
											  &Region1, &Region1Size,
											  &Region2, &Region2Size,
											  0)))
	{
		auto* DestSample = (uint8*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (uint8*)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer* SourceBuffer)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
											  &Region1, &Region1Size,
											  &Region2, &Region2Size,
											  0)))
	{
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		auto* DestSample = (int16*)Region1;
		int16* SourceSample = SourceBuffer->Samples;

		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16*)Region2;

		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;
	PAINTSTRUCT Paint = {};
	HDC DeviceContext;
	win32_window_dimension Dimension = {};

	switch (Message)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		GlobalRunning = false;
		break;

	case WM_ACTIVATEAPP:
		if (WParam == TRUE)
		{
			SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
		}
		else
		{
			SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
		}
		break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
		Assert(!"Keyboard input came in through non-dispatch message!");
		break;

	case WM_PAINT:
		DeviceContext = BeginPaint(Window, &Paint);
		Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
		EndPaint(Window, &Paint);
		break;

	default:
		Result = DefWindowProcA(Window, Message, WParam, LParam);
		break;
	}

	return (Result);
}

internal win32_game_code Win32LoadGameCode(char* SourceDLLName, char* TempDLLName)
{
	win32_game_code Result = {};

	Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
	CopyFile(SourceDLLName, TempDLLName, FALSE);
	Result.GameCodeDLL = LoadLibraryA(TempDLLName);

	if (Result.GameCodeDLL)
	{
		Result.UpdateAndRender = (game_update_and_render*)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
		Result.IsValid = Result.UpdateAndRender != nullptr;
	}

	if (!Result.IsValid)
	{
		Result.UpdateAndRender = nullptr;
	}

	return (Result);
}

internal void Win32UnloadGameCode(win32_game_code* GameCode)
{
	if (GameCode->GameCodeDLL != nullptr)
	{
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = nullptr;
	}

	GameCode->IsValid = false;
	GameCode->UpdateAndRender = nullptr;
}

internal void Win32ProcessPendingMessages(void)
{
	MSG Message;
	while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch (Message.message)
		{
		case WM_QUIT:
			GlobalRunning = false;
			break;

		default:
			TranslateMessage(&Message);
			DispatchMessageA(&Message);
			break;
		}
	}
}

inline LARGE_INTEGER Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return (Result);
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart)) / (real32)GlobalPerfCountFrequency;

	return (Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
	win32_state Win32State = {};

	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	GlobalPerfCountFrequency = PerfCounterFrequencyResult.QuadPart;

	Win32GetEXEFileName(&Win32State);

	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "game.dll", sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "game_temp.dll", sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

	WNDCLASSA WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "LootWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Loot",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			nullptr,
			nullptr,
			Instance,
			nullptr);

		if (Window)
		{
			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

			Win32InitDirectSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);

			HDC RefreshDC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);

			if (Win32RefreshRate == 0)
			{
				Win32RefreshRate = 60;
			}

			real32 GameUpdateHz = (real32(Win32RefreshRate) / 2.0f);
			real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

			GlobalRunning = true;
			win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Megabytes(64);

			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

			GameMemory.PermanentStorage = VirtualAlloc(nullptr, (size_t)TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			LARGE_INTEGER LastCounter = Win32GetWallClock();

			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			while (GlobalRunning)
			{
				FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
				if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
				{
					Win32UnloadGameCode(&Game);
					Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
				}

				Win32ProcessPendingMessages();

				Assert(Game.UpdateAndRender != nullptr);

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackBuffer.Memory;
				Buffer.Width = GlobalBackBuffer.Width;
				Buffer.Height = GlobalBackBuffer.Height;
				Buffer.Pitch = GlobalBackBuffer.Pitch;

				Game.UpdateAndRender(&GameMemory, &Buffer);

				LARGE_INTEGER WorkCounter = Win32GetWallClock();
				real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

				real32 SecondsElapsedForFrame = WorkSecondsElapsed;

				if (SecondsElapsedForFrame < TargetSecondsPerFrame)
				{
					auto SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
					if (SleepMS > 0)
					{
						Sleep(SleepMS);
					}

					while (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
					}
				}

				LARGE_INTEGER EndCounter = Win32GetWallClock();
				LastCounter = EndCounter;
			}
		}
	}
}