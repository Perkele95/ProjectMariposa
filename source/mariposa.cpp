#include "mariposa.h"

#include <math.h>

internal void OutputSound(MP_GAMESTATE* gameState, MP_SOUNDOUTPUTBUFFER* soundBuffer, int16 toneFrequency)
{
    int16 toneVolume = 2000;
    int wavePeriod = soundBuffer->SamplesPerSecond / toneFrequency;
    
    int16* sampleOut = soundBuffer->Samples;
    for(DWORD sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; sampleIndex++)
    {
        float sineValue = sinf(gameState->tSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
             
        gameState->tSine += 2.0f * PI32 * 1.0f / (float)wavePeriod;
        if(gameState->tSine >= 2.0f * PI32)
            gameState->tSine -= 2.0f * PI32;
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
            
            *pixel++ = blue | (green << 16);
        }
        
        row += buffer->Pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)gameMemory->PermanentStorage;
    MP_ASSERT(sizeof(gameState) <= gameMemory->PermanentStorageSize);
    
    if(!gameMemory->IsInitialised)
    {
        char* fileName = __FILE__;
        
        debug_read_file_result file = gameMemory->DEBUGPlatformReadEntireFile(thread, fileName);
        if(file.data)
        {
            //gameMemory->DEBUGPlatformWriteEntireFile(thread, "../data/test.out", &file);
            //gameMemory->DEBUGPlatformFreeFileMemory(thread, file.data);
        }
        
        gameState->toneFrequency = 312;
        gameState->tSine = 0.0f;
        
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
    gameState->blueOffset = -input->Mouse.X;
    gameState->greenOffset = -input->Mouse.Y;
    
    RenderGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples)
{
    MP_GAMESTATE* gameState = (MP_GAMESTATE*)gameMemory->PermanentStorage;
    OutputSound(gameState, soundBuffer, gameState->toneFrequency);
}