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
        // TODO: Perhaps put this into platform layer
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
            // Digital keyboard input goes here
        }
    }

    PROFILE_BLOCK_END_POINTER(GameUpdateAndRender);
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)memory->PermanentStorage;
    OutputSound(gameState, soundBuffer);
}
