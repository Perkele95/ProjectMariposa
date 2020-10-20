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

struct MP_GAMESTATE
{
    
};
