#pragma once

#include <Windows.h>

/*
    // BUILD OPTIONS:
    MP_INTERNAL:
        0 - release version
        1 - internal version
    MP_PERFORMANCE:
        0 - assertions/debug features enabled
        1 - assertions/debug features disabled
*/
#define MP_INTERNAL 1
#define MP_PERFORMANCE 0

#if MP_INTERNAL
    // THIS IS NOT FOR RELEASE VERSION, IT DOESN'T PROTECT AGAINST LOST DATA
    struct debug_read_file_result
    {
        void* data;
        uint32 dataSize;
    };

    internal debug_read_file_result DEBUG_PlatformReadEntireFile(char* filename);
    internal bool32 DEBUG_PlatformWriteEntireFile(char* fileName, debug_read_file_result* readData);
    internal void DEBUG_PlatformFreeFileMemory(void* memory);
#else
#endif

#if MP_PERFORMANCE
    #define MP_ASSERT(Expression)
#else
    #define MP_ASSERT(Expression) if(!(Expression)) {DebugBreak();}
#endif

#define KiloBytes(value) (value * 1024LL)
#define MegaBytes(value) (value * 1024LL * 1024LL)
#define GigaBytes(value) (value * 1024LL * 1024LL * 1024LL)
#define TeraBytes(value) (value * 1024LL * 1024LL * 1024LL * 1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

inline internal uint32 SafeTruncateUint32(uint64 value)
{
    // TODO: defines for max values
    MP_ASSERT(value < 0xffffffff);
    return (uint32)value;
}

struct MP_SOUNDOUTPUTBUFFER
{
    int SamplesPerSecond;
    uint32 SampleCount;
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
    bool32 IsConntected;
    bool32 IsAnalog;
    
    float StickAverageX;
    float StickAverageY;
    
    union
    {
        MP_BUTTON_STATE Buttons[13];
        // Make sure the buttons array matches the count of buttonstate struct
        struct 
        {
            MP_BUTTON_STATE Up;
            MP_BUTTON_STATE Down;
            MP_BUTTON_STATE Left;
            MP_BUTTON_STATE Right;
            MP_BUTTON_STATE Jump;
            MP_BUTTON_STATE Sprint;
            MP_BUTTON_STATE Crouch;
            MP_BUTTON_STATE Escape;
            MP_BUTTON_STATE Q;
            MP_BUTTON_STATE E;
            MP_BUTTON_STATE F;
            MP_BUTTON_STATE G;
            MP_BUTTON_STATE C;
        };
    };
};

// Controller[0] = Keyboard
struct MP_INPUT
{
    // float deltaTime;
    MP_CONTROLLER_INPUT Controllers[5];
};

// Bounds checking getter
inline internal MP_CONTROLLER_INPUT* GetController(MP_INPUT* input, int controllerIndex)
{
    MP_ASSERT(controllerIndex >= 0);
    MP_ASSERT(controllerIndex < ArrayCount(input->Controllers));
    
    return &input->Controllers[controllerIndex];
}

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

// TODO: Rename to update or something else
internal void GameUpdateAndRender(MP_MEMORY* gameMemory, MP_INPUT* input, MP_OFFSCREENBUFFER* buffer);
// NOTE: This function needs to be fast to keep audio latency low
internal void GetSoundSamples(MP_MEMORY* gameMemory, MP_SOUNDOUTPUTBUFFER* soundBuffer);