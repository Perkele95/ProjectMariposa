#pragma once

/*
    MP_PERFORMANCE:
        0 - assertions/debug features enabled
        1 - assertions/debug features disabled
    
    MP_INTERNAL:
        0 - release version
        1 - internal version
*/

#if MP_PERFORMANCE
#define MP_ASSERT(Expression)
#else
#define MP_ASSERT(Expression) if(!(Expression)) {DebugBreak();}
#endif

#define KiloBytes(value) (value * 1024)
#define MegaBytes(value) (value * 1024 * 1024)
#define GigaBytes(value) (value * 1024 * 1024 * 1024)
#define TeraBytes(value) (value * 1024 * 1024 * 1024 * 1024)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

/*
    NOTE: Services that the platform layer provides to the game
*/

/*
    NOTE: Services that the game provides to the platform layer
*/

/*  Memory block allocated on the heap which stores all game states
    This is freed at the end. DO NOT read/write this after it has been freed.
*/

struct MP_SOUNDOUTPUTBUFFER
{
    int SamplesPerSecond;
    int SampleCount;
    int16* Samples;
};

struct MP_OFFSCREENBUFFER
{
    void* Memory;
    int Width, Height, Pitch;
};

struct MP_BUTTON_STATE
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct MP_CONTROLLER_INPUT
{
    bool32 IsAnalog;
    
    float StartX;
    float StartY;
    
    float MinX;
    float MinY;
    
    float MaxX;
    float MaxY;
    
    float EndX;
    float EndY;
    
    union
    {
        MP_BUTTON_STATE Buttons[6];
        
        struct 
        {
        MP_BUTTON_STATE Up;
        MP_BUTTON_STATE Down;
        MP_BUTTON_STATE Left;
        MP_BUTTON_STATE Right;
        MP_BUTTON_STATE LeftShoulder;
        MP_BUTTON_STATE RightShoulder;
        };
    };
    
};

struct MP_INPUT
{
    // float deltaTime;
    MP_CONTROLLER_INPUT Controllers[4];
};

struct MP_GAMESTATE
{
    int blueOffset;
    int greenOffset;
    int16 toneFrequency;
};

struct MP_MEMORY
{
    bool32 IsInitialised;
    
    uint64 PermanentStorageSize;
    void* PermanentStorage; // NOTE: Set to zero before allocation!
    
    uint64 TransientStorageSize;
    void* TransientStorage; // NOTE: Set to zero before allocation!
};

// Requires: timestep, input, bitmap buffer and sound buffer
internal void GameUpdateAndRender(MP_MEMORY* memory, MP_INPUT* input, MP_SOUNDOUTPUTBUFFER* soundBuffer, MP_OFFSCREENBUFFER* buffer);
