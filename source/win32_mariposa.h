#pragma once

typedef unsigned int uint32;

#include <windows.h>

struct win32OffscreenBuffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width, Height;
    int Pitch;
};

struct Win32WindowDimensions
{
    int width, height;
};

struct Win32SoundOutput{
    int SamplesPerSecond;
    int BytesPerSample;
    uint32 RunningSampleIndex;
    int SecondaryBufferSize;
    float tSine;
    int LatencySampleCount;
};