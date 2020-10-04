#pragma once

typedef unsigned int uint32;

#include <windows.h>

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
    float tSine;
    int LatencySampleCount;
};

struct Win32DebugTimeMarker
{
    DWORD PlayCursor;
    DWORD WriteCursor;
};