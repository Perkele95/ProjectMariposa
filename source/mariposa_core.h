#pragma once

#include <windows.h>

/*
    // BUILD OPTIONS:
    MP_INTERNAL:
        0 - release version
        1 - internal version
    MP_PERFORMANCE:
        0 - assertions/debug features enabled
        1 - assertions/debug features disabled
*/

#define MP_INTERNAL 1
#define MP_PERFORMANCE 0

#define internal static
#define local_persist static
#define global_variable static

#define UINT64MAX 0xFFFFFFFFFFFFFFFF
#define PI32 3.14159265359f

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;
typedef int32 bool32;

#define MP_SCREEN_WIDTH 1280
#define MP_SCREEN_HEIGHT 720

#if MP_PERFORMANCE
    #define MP_ASSERT(Expression)
#else
    #define MP_ASSERT(Expression) if(!(Expression)) {DebugBreak();}
#endif

#define KiloBytes(value) (value * 1024LL)
#define MegaBytes(value) (value * 1024LL * 1024LL)
#define GigaBytes(value) (value * 1024LL * 1024LL * 1024LL)
#define TeraBytes(value) (value * 1024LL * 1024LL * 1024LL * 1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

struct MP_THREAD_CONTEXT
{
    int Placeholder;
};

#if MP_INTERNAL
// THIS IS NOT FOR RELEASE VERSION, IT DOESN'T PROTECT AGAINST LOST DATA
struct debug_read_file_result
{
    void* data;
    uint32 dataSize;
};

#if MP_INTERNAL
enum
{
    CycleCounter_GameUpdateAndRender,
    CycleCounter_VulkanUpdate,
    CycleCounter_Max
};

struct debug_cycle_counter
{
    uint64 CycleCount;
    uint32 HitCount;
};
#endif

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(MP_THREAD_CONTEXT* thread, char* filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(MP_THREAD_CONTEXT* thread, char* filename, debug_read_file_result* readData)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(MP_THREAD_CONTEXT* thread, void* memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#if _MSC_VER
#define PROFILE_BLOCK_START(ID) uint64 startCycleCount##ID = __rdtsc();
#define PROFILE_BLOCK_END(ID) memory.CycleCounters[CycleCounter_##ID].CycleCount = __rdtsc() - startCycleCount##ID; memory.CycleCounters[CycleCounter_##ID].HitCount++;
#define PROFILE_BLOCK_END_POINTER(ID) memory->CycleCounters[CycleCounter_##ID].CycleCount = __rdtsc() - startCycleCount##ID; memory->CycleCounters[CycleCounter_##ID].HitCount++;
#else
#define PROFILE_BLOCK_START(ID)
#define PROFILE_BLOCK_END(ID)
#define PROFILE_BLOCK_END_POINTER(ID)
#endif

#endif

struct MP_SOUNDOUTPUTBUFFER
{
    int SamplesPerSecond;
    uint32 SampleCount;
    int16* Samples;
};

struct MP_BUTTON_STATE
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct MP_CONTROLLER_INPUT
{
    bool32 IsConntected;
    bool32 IsAnalog;
    
    float StickAverageX;
    float StickAverageY;
    
    union
    {
        MP_BUTTON_STATE Buttons[13];
        // Make sure the buttons array matches the count of buttonstate struct
        struct 
        {
            MP_BUTTON_STATE Up;
            MP_BUTTON_STATE Down;
            MP_BUTTON_STATE Left;
            MP_BUTTON_STATE Right;
            MP_BUTTON_STATE Jump;
            MP_BUTTON_STATE Sprint;
            MP_BUTTON_STATE Crouch;
            MP_BUTTON_STATE Escape;
            MP_BUTTON_STATE Q;
            MP_BUTTON_STATE E;
            MP_BUTTON_STATE F;
            MP_BUTTON_STATE G;
            MP_BUTTON_STATE C;
        };
    };
};

struct MP_MOUSE_BUTTON_STATE
{
    bool32 Up;
    bool32 Down;
};

struct MP_MOUSE_INPUT
{
    int X, Y, Wheel;
    bool32 ShowCursor;
    
    union
    {
        MP_MOUSE_BUTTON_STATE Buttons[3];
        // Make sure the buttons array matches the count of buttonstate struct
        struct 
        {
            MP_MOUSE_BUTTON_STATE LeftClick;
            MP_MOUSE_BUTTON_STATE RightClick;
            MP_MOUSE_BUTTON_STATE Middle;
        };
    };
};

// Controller[0] = Keyboard
struct MP_INPUT
{
    MP_MOUSE_INPUT Mouse;
    MP_CONTROLLER_INPUT Controllers[5];
};

struct MP_MEMORY
{
    bool32 IsInitialised;
    
    uint64 PermanentStorageSize;
    void* PermanentStorage; // NOTE: Set to zero before allocation!
    
    uint64 TransientStorageSize;
    void* TransientStorage; // NOTE: Set to zero before allocation!
    
    debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
    debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
    
    #if MP_INTERNAL
    debug_cycle_counter CycleCounters[CycleCounter_Max];
    #endif
};

#define GAME_UPDATE_AND_RENDER(name) void name(MP_THREAD_CONTEXT* thread, MP_MEMORY* memory, MP_INPUT* input, float timestep)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE: This function needs to be fast to keep audio latency low
#define GET_SOUND_SAMPLES(name) void name(MP_THREAD_CONTEXT* thread, MP_MEMORY* memory, MP_SOUNDOUTPUTBUFFER* soundBuffer)
typedef GET_SOUND_SAMPLES(get_sound_samples);
