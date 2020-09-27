#pragma once

/*
    NOTE: Services that the platform layer provides to the game
*/

/*
    NOTE: Services that the game provides to the platform layer
*/

struct MP_OFFSCREENBUFFER
{
    void* Memory;
    int Width, Height, Pitch;
};

// Requires: timestep, input, bitmap buffer and sound buffer
internal void GameUpdateAndRender(MP_OFFSCREENBUFFER* buffer);
