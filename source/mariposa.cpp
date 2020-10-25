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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    PROFILE_BLOCK_START(GameUpdateAndRender);

    MP_GAMESTATE* gameState = (MP_GAMESTATE*)memory->PermanentStorage;
    MP_ASSERT(sizeof(gameState) <= memory->PermanentStorageSize);

    if(!memory->IsInitialised)
    {
        memory->IsInitialised = true;
        gameState->RenderData.CameraPosition = {2.0f, 2.0f, 2.0f};
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
                gameState->RenderData.CameraRotation.X += timestep;
            }
            else if(controller->Down.EndedDown)
            {
                gameState->RenderData.CameraRotation.X -= timestep;
            }
            if(controller->Left.EndedDown)
            {
                gameState->RenderData.CameraRotation.Y += timestep;
            }
            else if(controller->Right.EndedDown)
            {
                gameState->RenderData.CameraRotation.Y -= timestep;
            }
        }
    }
    
    if(input[0].Mouse.Wheel)
    {
        // mouse wheel
    }
    
    gameState->RenderData.CameraRotation.Z += timestep;
    
    PROFILE_BLOCK_END_POINTER(GameUpdateAndRender);
    
    return &gameState->RenderData;
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)memory->PermanentStorage;
    OutputSound(gameState, soundBuffer);
}
