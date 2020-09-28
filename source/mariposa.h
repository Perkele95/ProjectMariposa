#pragma once

// MACROS
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

/*
    NOTE: Services that the platform layer provides to the game
*/

/*
    NOTE: Services that the game provides to the platform layer
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
    MP_CONTROLLER_INPUT Controllers[4];
};

// Requires: timestep, input, bitmap buffer and sound buffer
internal void GameUpdateAndRender(MP_INPUT* input, MP_SOUNDOUTPUTBUFFER* soundBuffer, MP_OFFSCREENBUFFER* buffer);
