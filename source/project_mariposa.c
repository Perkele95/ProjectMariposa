#include "project_mariposa.h"

#include <stdio.h>

static VkBool32 CheckLayers(uint32 checkCount, char* checkNames[], uint32 layerCount, VkLayerProperties *layers)
{
    for(uint32 i = 0; i < checkCount; i++)
    {
        VkBool32 found = 0;
        for(uint32 j = 0; i < layerCount; j++)
        {
            if(!strcmp(checkNames[i], layers[j].layerName))
            {
                found = true;
                break;
            }
        }
        
        if(!found)
        {
            OutputDebugStringA("Could not find layer: ");
            OutputDebugStringA(checkNames[i]);
            OutputDebugStringA("\n");
            return false;
        }
    }
    return true;
}

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
    
    // look for validation layers
    VkBool32 validationFound = false;
    if(renderer->Validate)
    {
        error = vkEnumerateInstanceLayerProperties(&instanceLayerCount, NULL);
        MP_ASSERT(!error);
        
        if(instanceLayerCount > 0)
        {
            // TODO: replace with custom allocator to work with preallocated memory
            VkLayerProperties *instanceLayers = malloc(sizeof(VkLayerProperties) * instanceLayerCount);
            error = vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers);
            MP_ASSERT(!error);
            
            validationFound = CheckLayers(ArrayCount(instanceValidationLayers), instanceValidationLayers, instanceLayerCount, instanceLayers);
            if(validationFound)
            {
                renderer->EnabledLayerCount = ArrayCount(instanceValidationLayers);
                renderer->EnabledLayers[0] = "VK_LAYER_KHRONOS_validation";
            }
            free(instanceLayers);
        }
        
        if(!validationFound)
        {
            // TODO: Log critical error
            OutputDebugStringA("vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n");
        }
    }
    
    VkBool32 surfaceExtensionFound = false;
    VkBool32 platformSurfaceExtensionFound = false;
    memset(renderer->ExtensionNames, 0, sizeof(renderer->ExtensionNames));
    
    error = vkEnumerateInstanceExtensionProperties(NULL, &instanceExtensionCount, NULL);
    MP_ASSERT(!error);
    
    if(instanceExtensionCount > 0)
    {
        // TODO: replace with custom allocator to work with preallocated memory
        VkExtensionProperties *instanceExtensions = malloc(sizeof(VkExtensionProperties) * instanceExtensionCount);
        error = vkEnumerateInstanceExtensionProperties(NULL, &instanceExtensionCount, instanceExtensions);
        MP_ASSERT(!error);
        
        for(uint32 i = 0; i < instanceExtensionCount; i++)
        {
            if(!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instanceExtensions[i].extensionName))
            {
                surfaceExtensionFound = true;
                renderer->ExtensionNames[renderer->EnabledExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
            }
            // NOTE: ONLY WDINWOS SUPPORT
            if(!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instanceExtensions[i].extensionName))
            {
                platformSurfaceExtensionFound = true;
                renderer->ExtensionNames[renderer->EnabledExtensionCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
            }
            MP_ASSERT(renderer->EnabledExtensionCount < 64);
        }
        free(instanceExtensions);
    }
    
    if(!surfaceExtensionFound)
        OutputDebugStringA("Failed to find the 'VK_KHR_surface' extension.\n\n");
    
    if(!platformSurfaceExtensionFound)
        OutputDebugStringA("Failed to find the 'VK_KHR_win32_surface' extension.\n\n");
    
    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "Project :: Mariposa",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Mariposa",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    
    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = renderer->EnabledLayerCount,
        .ppEnabledLayerNames = (const char *const *)instanceValidationLayers,
        .enabledExtensionCount = renderer->EnabledLayerCount,
        .ppEnabledExtensionNames = (const char *const *)renderer->ExtensionNames
    };
    
    uint32 gpuCount;
    
    error = vkCreateInstance(&instanceInfo, NULL, &renderer->Instance);
    if(error == VK_ERROR_INCOMPATIBLE_DRIVER)
        OutputDebugStringA("Incompatible driver!\n\n");
    else if(error == VK_ERROR_EXTENSION_NOT_PRESENT)
        OutputDebugStringA("Extension library not present!\n\n");
    else if(error)
        OutputDebugStringA("VkCreateInstance failed!\n\n");
    
    error = vkEnumeratePhysicalDevices(renderer->Instance, &gpuCount, NULL);
    MP_ASSERT(!error)
    
    if(gpuCount > 0)
    {
        VkPhysicalDevice *physicalDevices = malloc(sizeof(VkPhysicalDevice) * gpuCount);
        error = vkEnumeratePhysicalDevices(renderer->Instance, &gpuCount, physicalDevices);
        MP_ASSERT(!error);
        if(renderer->GpuNumber > gpuCount - 1)
        {
            OutputDebugStringA("Gpu specified is not present");
            OutputDebugStringA("Continuing with gpu 0\n");
            renderer->GpuNumber = 0;
        }
        renderer->Gpu = physicalDevices[renderer->GpuNumber];
        free(physicalDevices);
    }
    else
    {
        OutputDebugStringA("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n");
    }
    
    uint32 deviceExtensionCount = 0;
    VkBool32 SwapChainExtensionFound = false;
    renderer->EnabledExtensionCount = 0;
    memset(renderer->ExtensionNames, 0, sizeof(renderer->ExtensionNames));
    
    error = vkEnumerateDeviceExtensionProperties(renderer->Gpu, NULL, &deviceExtensionCount, NULL);
    MP_ASSERT(!error);
    
    if(deviceExtensionCount > 0)
    {
        VkExtensionProperties *deviceExtensions = malloc(sizeof(VkExtensionProperties) * deviceExtensionCount);
        error = vkEnumerateDeviceExtensionProperties(renderer->Gpu, NULL, &deviceExtensionCount, deviceExtensions);
        MP_ASSERT(!error);
        
        for(uint32 i = 0; i < deviceExtensionCount; i++)
        {
            if(!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, deviceExtensions[i].extensionName))
            {
                SwapChainExtensionFound = true;
                renderer->ExtensionNames[renderer->EnabledExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            }
            MP_ASSERT(renderer->EnabledExtensionCount < 64);
        }
        free(deviceExtensions);        
    }
    
    if(!SwapChainExtensionFound)
        OutputDebugStringA("vkEnumerateDeviceExtensionProperties failed to find the 'VK_KHR_swapchain' extension.\n\n");
    
    vkGetPhysicalDeviceProperties(renderer->Gpu, &renderer->GpuProps);
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->Gpu, &renderer->QueueFamilyCount, renderer->QueueProps);
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