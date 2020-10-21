#include "project_mariposa.h"

static void RendererVKInit(Renderer *renderer)
{
    VkResult error;
    uint32 instanceExtensionCount = 0;
    uint32 instanceLayerCount = 0;
    char* instanceValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    renderer->EnabledExtensionCount = 0;
    renderer->EnabledLayerCount = 0;
    renderer->IsMinimised = false;
    renderer->CommandPool = VK_NULL_HANDLE;
}

static void RendererInit(Renderer *renderer)
{
    Vec3 eye = {0.0f, 3.0f, 5.0f};
    Vec3 center = {0, 0, 0};
    Vec3 up = {0.0f, 1.0f, 0.0};
    
    renderer->PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    renderer->FrameCount = INT32_MAX;
    renderer->GpuNumber = 0;
    
    RendererVKInit(renderer);
    
    renderer->Width = 1280;
    renderer->Height = 720;
    renderer->SpinAngle = 4.0f;
    
    float aspectRatio = (float)renderer->Width / (float)renderer->Height;
    renderer->ProjectionMatrix = Mat4x4Perspective(PI32 / 4.0f, aspectRatio, 0.1f, 50.0f);
    renderer->ViewMatrix = Mat4x4LookAt(eye, center, up);
    renderer->ModelMatrix = Mat4x4Identity();
}

static void InitMemory(GameMemory *memory)
{
    memory->PermanentStorageSize = MegaBytes(64);
    memory->TransientStorageSize = GigaBytes(1);
    
    memory->PermanentStorage = VirtualAlloc(0, memory->PermanentStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

inline internal LARGE_INTEGER Win32GetClockValue(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline internal float Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end, LARGE_INTEGER perfCountFrequency)
{
    float result = (float)(end.QuadPart - start.QuadPart) / (float)perfCountFrequency.QuadPart;
    return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    GameMemory memory = {0};
    InitMemory(&memory);
    
    Renderer *renderer = (Renderer*)memory.PermanentStorage;
    
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    
    bool32 running = true;
    bool32 pause = false;
    
    if(memory.PermanentStorage)
    {
        LARGE_INTEGER lastCounter = Win32GetClockValue();
        float timestep = 0.001f;
    
        RendererInit(renderer);
        
        while(running)
        {
            
            
            LARGE_INTEGER endCounter = Win32GetClockValue();
            timestep = Win32GetSecondsElapsed(lastCounter, endCounter, perfCountFrequencyResult);
            lastCounter = endCounter;
        }
    }
    
    return 0;
}