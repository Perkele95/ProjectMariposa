#include "mariposa.h"

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

internal void GameUpdateAndRender(MP_OFFSCREENBUFFER* buffer, int blueOffset, int greenOffset)
{
    RenderGradient(buffer, blueOffset, greenOffset);
}