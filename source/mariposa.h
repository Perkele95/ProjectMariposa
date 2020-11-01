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
    vec3 Position;
    vec3 Colour;
    vec2 TexCoord;
};

struct Cube
{
    Vertex Vertices[24];
    uint16 Indices[36];
};

struct MP_RENDERDATA
{
    // TODO: Put into Camera struct sort of thing
    vec3 CameraPosition;
    vec3 CameraRotation;
    
    Cube* Cubes;
    uint32 CubeCount;
};

#define MP_API extern"C" __declspec(dllexport)

#define GAME_UPDATE_AND_RENDER(name) void name(MP_THREAD_CONTEXT* thread, MP_MEMORY* memory, MP_INPUT* input, MP_RENDERDATA* renderData, float timestep)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE: This function needs to be fast to keep audio latency low
#define GET_SOUND_SAMPLES(name) void name(MP_THREAD_CONTEXT* thread, MP_MEMORY* memory, MP_SOUNDOUTPUTBUFFER* soundBuffer)
typedef GET_SOUND_SAMPLES(get_sound_samples);

#define BUILD_WORLD(name) MP_RENDERDATA* name(MP_THREAD_CONTEXT* thread, MP_MEMORY* memory)
typedef BUILD_WORLD(build_world);

struct MP_GAMESTATE
{
    MP_RENDERDATA* RenderData;
};