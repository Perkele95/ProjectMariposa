#include "mariposa.h"

#include <windows.h>

internal void OutputSound(MP_GAMESTATE* gameState, MP_SOUNDOUTPUTBUFFER* soundBuffer)
{
    int16* sampleOut = soundBuffer->Samples;
    for(DWORD sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; sampleIndex++)
    {
        int16 sampleValue = 0;

        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
    }
}

internal void TranslateCube(Cube* cube, vec3 newPos)
{
    for(uint32 i = 0; i < ArrayCount(cube->Vertices); i++)
    {
        
    }
}

internal void BuildCubes(Cube* cubes, uint32 cubeCount)
{
    float scale = 0.5f;
    for(uint32 i = 0; i < cubeCount; i++)
    {
        cubes[i].Vertices[0]  = {{-scale, -scale, scale}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}; // TOP
        cubes[i].Vertices[1]  = {{scale, -scale, scale}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
        cubes[i].Vertices[2]  = {{scale, scale, scale}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
        cubes[i].Vertices[3]  = {{-scale, scale, scale}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};
        cubes[i].Vertices[4]  = {{-scale, scale, -scale}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}; // BOTTOM
        cubes[i].Vertices[5]  = {{scale, scale, -scale}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
        cubes[i].Vertices[6]  = {{scale, -scale, -scale}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
        cubes[i].Vertices[7]  = {{-scale, -scale, -scale}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};
        cubes[i].Vertices[8]  = {{scale, -scale, scale}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}; // NORTH
        cubes[i].Vertices[9]  = {{scale, -scale, -scale}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
        cubes[i].Vertices[10] = {{scale, scale, -scale}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
        cubes[i].Vertices[11] = {{scale, scale, scale}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};
        cubes[i].Vertices[12] = {{-scale, scale, scale}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}; // SOUTH
        cubes[i].Vertices[13] = {{-scale, scale, -scale}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
        cubes[i].Vertices[14] = {{-scale, -scale, -scale}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
        cubes[i].Vertices[15] = {{-scale, -scale, scale}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};
        cubes[i].Vertices[16] = {{scale, scale, scale}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}; // EAST
        cubes[i].Vertices[17] = {{scale, scale, -scale}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
        cubes[i].Vertices[18] = {{-scale, scale, -scale}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
        cubes[i].Vertices[19] = {{-scale, scale, scale}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};
        cubes[i].Vertices[20] = {{-scale, -scale, scale}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}; // WEST
        cubes[i].Vertices[21] = {{-scale, -scale, -scale}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
        cubes[i].Vertices[22] = {{scale, -scale, -scale}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
        cubes[i].Vertices[23] = {{scale, -scale, scale}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};
        
        for(uint16 row = 0; row < 6; row++)
        {
            uint16 rowOffset = row * 6;
            uint16 indexOffset = 4 * row + 24 * (uint16)i;
            cubes[i].Indices[rowOffset]     = 0 + indexOffset;
            cubes[i].Indices[rowOffset + 1] = 1 + indexOffset;
            cubes[i].Indices[rowOffset + 2] = 2 + indexOffset;
            cubes[i].Indices[rowOffset + 3] = 2 + indexOffset;
            cubes[i].Indices[rowOffset + 4] = 3 + indexOffset;
            cubes[i].Indices[rowOffset + 5] = 0 + indexOffset;
        }
    }
}

MP_API BUILD_WORLD(BuildWorld)
{
    MP_RENDERDATA* renderData = (MP_RENDERDATA*) memory->PermanentStorage;
    memory->PermanentStorage = (MP_RENDERDATA*) memory->PermanentStorage + 1;
    
    renderData->CameraPosition = {2.0f, 2.0f, 2.0f};
    
    BuildCubes(renderData->Cubes, ArrayCount(renderData->Cubes));
    TranslateCube(&renderData->Cubes[1], {3.0f, 3.0f, 3.0f});
    TranslateCube(&renderData->Cubes[2], {5.0f, 5.0f, 5.0f});
    
    return renderData;
}

MP_API GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{    
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)memory->PermanentStorage;
    MP_ASSERT(sizeof(gameState) <= memory->PermanentStorageSize);

    if(!memory->IsInitialised)
    {
        memory->IsInitialised = true;
    }
    
    for(int index = 0; index < ArrayCount(input->Controllers); index++)
    {
        MP_CONTROLLER_INPUT* controller = GetController(input, index);
        controller->IsAnalog = false;
        if(controller->IsAnalog)
        {
            // Analog input
            // I have no controller so I cannot verify this
        }
        else
        {
            if(controller->Up.EndedDown)
            {
                renderData->CameraRotation.X += timestep;
            }
            else if(controller->Down.EndedDown)
            {
                renderData->CameraRotation.X -= timestep;
            }
            if(controller->Left.EndedDown)
            {
                renderData->CameraRotation.Y += timestep;
            }
            else if(controller->Right.EndedDown)
            {
                renderData->CameraRotation.Y -= timestep;
            }
        }
    }
    
    if(input[0].Mouse.Wheel)
    {
        // mouse wheel
    }
}

MP_API GET_SOUND_SAMPLES(GetSoundSamples)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)memory->PermanentStorage;
    OutputSound(gameState, soundBuffer);
}
