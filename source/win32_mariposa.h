#pragma once

typedef unsigned int uint32;

struct win32OffscreenBuffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width, Height;
    int Pitch;
    int bytesPerPixel;
};

struct Win32WindowDimensions
{
    int width, height;
};

struct Win32SoundOutput{
    int SamplesPerSecond;
    int BytesPerSample;
    uint32 RunningSampleIndex;
    DWORD SecondaryBufferSize;
    DWORD SafetyBytes;
    // NOTE: Math might get simpler if we add bytesPerSecond field
    // NOTE: Perhaps runningsampleindex should be in bytes as well?
};

struct Win32DebugTimeMarker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;
    DWORD ExpectedFlipPlayCursor;
    
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};

struct Win32GameCode
{
    HMODULE DLL;
    FILETIME DLLLastWriteTime;
    // IMPORTANT: Callbacks need to be null-checked before usage
    game_update_and_render* UpdateAndRender;
    get_sound_samples* GetSoundSamples;
    bool32 IsValid;
};

struct Win32ReplayBuffer
{
    HANDLE FileHandle;
    HANDLE MapView;
    char Filename[MAX_PATH];
    void* Data;
};

struct Win32State
{
    uint64 TotalSize;
    void* GameMemoryBlock;
    Win32ReplayBuffer ReplayBuffers[4];
    
    HANDLE RecordingHandle;
    int InputRecordingIndex;
    
    HANDLE PlaybackHandle;
    int InputPlaybackIndex;
    
    char ExeFilename[MAX_PATH];
    char* OnePastLastSlash;
};