#pragma once

#include "mariposa_core.h"
#include "mp_maths.h"

inline internal uint32 SafeTruncateUint32(uint64 value)
{
    // TODO: defines for max values
    MP_ASSERT(value < 0xffffffff);
    return (uint32)value;
}

// Bounds checking getter
inline internal MP_CONTROLLER_INPUT* GetController(MP_INPUT* input, int controllerIndex)
{
    MP_ASSERT(controllerIndex >= 0);
    MP_ASSERT(controllerIndex < ArrayCount(input->Controllers));
    
    return &input->Controllers[controllerIndex];
}

struct Vertex
{
    Vector2 Position;
    Vector3 Colour;
    Vector2 TexCoord;
};

struct MP_RENDERDATA
{
    Vector3 CameraPosition;
    Vector3 CameraRotation;
};

#define GAME_UPDATE_AND_RENDER(name) MP_RENDERDATA* name(MP_THREAD_CONTEXT* thread, MP_MEMORY* memory, MP_INPUT* input, float timestep)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE: This function needs to be fast to keep audio latency low
#define GET_SOUND_SAMPLES(name) void name(MP_THREAD_CONTEXT* thread, MP_MEMORY* memory, MP_SOUNDOUTPUTBUFFER* soundBuffer)
typedef GET_SOUND_SAMPLES(get_sound_samples);

struct MP_GAMESTATE
{
    MP_RENDERDATA RenderData;
};
