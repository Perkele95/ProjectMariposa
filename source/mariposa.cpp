#include "mariposa.h"

#include <Windows.h>
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
            uint8 blue = x + xOffset;
            uint8 green = y + yOffset;
            
            *pixel++ = blue | (green << 8);
        }
        
        row += buffer->Pitch;
    }
}

internal void GameUpdateAndRender(MP_INPUT* input, MP_SOUNDOUTPUTBUFFER* soundBuffer, MP_OFFSCREENBUFFER* buffer)
{
    local_persist int blueOffset = 0;
    local_persist int greenOffset = 0;
    local_persist int16 toneFrequency = 256;
    
    MP_CONTROLLER_INPUT* input0 = &input->Controllers[0];
    if(input0->IsAnalog)
    {
        // I have no controller so I cannot verify this
        blueOffset += (int)(4.8f * input0->EndX);
        toneFrequency = 256 + (int)(128.0f * input0->EndY);
    }
    else
    {
        // Digital movement tuning
    }
    
    if(input0->Down.EndedDown)
    {
        greenOffset += 1;
    }
    
    OutputSound(soundBuffer, toneFrequency);
    RenderGradient(buffer, blueOffset, greenOffset);
}