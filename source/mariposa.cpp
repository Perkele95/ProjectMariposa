#include "mariposa.h"

#include <windows.h>

internal void OutputSound(MP_GAMESTATE* gameState, MP_SOUNDOUTPUTBUFFER* soundBuffer)
{
    int16* sampleOut = soundBuffer->Samples;
    for(DWORD sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; sampleIndex++)
    {
        #if 0
        float sineValue = sinf(gameState->tSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        #else
        int16 sampleValue = 0;
        #endif
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        #if 0
        gameState->tSine += 2.0f * PI32 * 1.0f / (float)wavePeriod;
        if(gameState->tSine >= 2.0f * PI32)
            gameState->tSine -= 2.0f * PI32;
        #endif
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)gameMemory->PermanentStorage;
    MP_ASSERT(sizeof(gameState) <= gameMemory->PermanentStorageSize);
    
    if(!gameMemory->IsInitialised)
    {
        // TODO: Perhaps put this into platform layer
        gameMemory->IsInitialised = true;
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
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)gameMemory->PermanentStorage;
    OutputSound(gameState, soundBuffer);
}