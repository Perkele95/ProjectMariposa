#include "mariposa.h"

#include <math.h>

internal void OutputSound(MP_SOUNDOUTPUTBUFFER* soundBuffer, int16 toneFrequency)
{
    local_persist float tSine;
    int16 toneVolume = 3000;
    int wavePeriod = soundBuffer->SamplesPerSecond / toneFrequency;
    
    int16* sampleOut = soundBuffer->Samples;
    for(DWORD sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; sampleIndex++)
    {
        float sineValue = sinf(tSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        
        tSine += 2.0f * PI32 / (float)wavePeriod;
        if(tSine >= 2.0f * PI32)
            tSine -= 2.0f * PI32;
    }
}

internal void RenderGradient(MP_OFFSCREENBUFFER* buffer, int xOffset, int yOffset)
{    
    uint8* row = (uint8*)buffer->Memory;
    for(int y = 0; y < buffer->Height; y++)
    {
        uint32* pixel = (uint32*)row;
        for(int x= 0; x < buffer->Width; x++)
        {
            // pixel in memory: BB GG RR CC
            uint8 blue = (uint8)(x + xOffset);
            uint8 green = (uint8)(y + yOffset);
            
            *pixel++ = blue | (green << 8);
        }
        
        row += buffer->Pitch;
    }
}

internal void GameUpdateAndRender(MP_MEMORY* gameMemory, MP_INPUT* input, MP_OFFSCREENBUFFER* buffer)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)gameMemory->PermanentStorage;
    MP_ASSERT(sizeof(gameState) <= gameMemory->PermanentStorageSize)
    
    if(!gameMemory->IsInitialised)
    {
        char* fileName = __FILE__;
        
        debug_read_file_result file = DEBUG_PlatformReadEntireFile(fileName);
        if(file.data)
        {
            DEBUG_PlatformWriteEntireFile("test.out", &file);
            DEBUG_PlatformFreeFileMemory(file.data);
        }
        
        gameState->toneFrequency = 256;
        
        // TODO: Perhaps put this into platform layer
        gameMemory->IsInitialised = true;
    }
    
    for(int index = 0; index < ArrayCount(input->Controllers); index++)
    {
        MP_CONTROLLER_INPUT* controller = GetController(input, index);
        controller->IsAnalog = false;
        if(controller->IsAnalog)
        {
            // I have no controller so I cannot verify this
            gameState->blueOffset += (int)(4.8f * controller->StickAverageX);
            gameState->toneFrequency = 256 + (int16)(128.0f * controller->StickAverageY);
        }
        else
        {
            if(controller->Down.EndedDown)
            {
                gameState->greenOffset += 1;
            }
            if(controller->Up.EndedDown)
            {
                gameState->greenOffset -= 1;
            }
            if(controller->Left.EndedDown)
            {
                gameState->blueOffset -= 1;
            }
            if(controller->Right.EndedDown)
            {
                gameState->blueOffset += 1;
            }
        }
    }
    
    RenderGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}

internal void GetSoundSamples(MP_MEMORY* gameMemory, MP_SOUNDOUTPUTBUFFER* soundBuffer)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)gameMemory->PermanentStorage;
    OutputSound(soundBuffer, gameState->toneFrequency);
}