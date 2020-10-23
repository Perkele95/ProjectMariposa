#pragma once

#include <windows.h>

#include "..\Vulkan\Include\vulkan\vulkan.h"
#include "..\Vulkan\Include\vulkan\vulkan_win32.h"

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

#define true 1
#define false 0

#include "pm_maths.h"

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

#define FRAME_LAG 2
#define TEXTURE_COUNT 1

// --------------------
// --------------------

typedef struct GameMemory
{
    uint64 PermanentStorageSize;
    void* PermanentStorage;
    
    uint64 DynamicStorageSize;
    void* DynamicStorage;
    uint64 DynamicStorageCurrentSize;
    void* DynamicStorageCurrent;
} GameMemory;

typedef struct {
    VkImage Image;
    VkCommandBuffer Cmd;
    VkCommandBuffer GraphicsToPresentCmd;
    VkImageView View;
    VkBuffer Uniformbuffer;
    VkDeviceMemory UniformMemory;
    void *UniformMemoryPtr;
    VkFramebuffer Framebuffer;
    VkDescriptorSet DescriptorSet;
} SwapchainImageResources;

typedef struct {
    VkSampler Sampler;

    VkImage Image;
    VkBuffer Buffer;
    VkImageLayout ImageLayout;

    VkMemoryAllocateInfo MemAlloc;
    VkDeviceMemory Memory;
    VkImageView View;
    int32_t TexWidth, TexHeight;
} TextureObject;

typedef struct Renderer
{
    HINSTANCE HInstance;
    HWND Window;
    POINT MinSize;
    
    VkSurfaceKHR Surface;
    bool32 Prepared;
    bool32 UseStagingBuffer;
    bool32 SeparatePresentQueue;
    bool32 IsMinimised;
    uint32 GpuNumber;
    
    bool32 SyncedWithActualPresents;
    
    uint64 RefreshDuration;
    uint64 RefreshDurationMultiplier;
    float Timestep;  // image present duration (inverse of frame rate)
    uint64 PrevDesiredPresentTime;
    uint32 NextPresentID;
    uint32 LastEarlyID;  // 0 if no early images
    uint32 LastLateID;   // 0 if no late images
    
    VkInstance Instance;
    VkPhysicalDevice Gpu;
    VkDevice Device;
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    uint32_t GraphicsQueueFamilyIndex;
    uint32_t PresentQueueFamilyIndex;
    VkSemaphore ImageAcquiredSemaphores[FRAME_LAG];
    VkSemaphore DrawCompleteSemaphores[FRAME_LAG];
    VkSemaphore ImageOwnershipSemaphores[FRAME_LAG];
    VkPhysicalDeviceProperties GpuProps;
    VkQueueFamilyProperties *QueueProps;
    VkPhysicalDeviceMemoryProperties MemoryProps;
    
    uint32 EnabledExtensionCount;
    uint32 EnabledLayerCount;
    char *ExtensionNames[64];
    char *EnabledLayers[64];
    
    int Width, Height;
    VkFormat Format;
    VkColorSpaceKHR ColorSpace;
    
    uint32 SwapChainImageCount;
    
    VkSwapchainKHR Swapchain;
    SwapchainImageResources *SwapchainImageResources;
    VkPresentModeKHR PresentMode;
    VkFence Fences[FRAME_LAG];
    int FrameIndex;

    VkCommandPool CommandPool;
    VkCommandPool PresentCommandPool;
    
    struct {
        VkFormat Format;

        VkImage Image;
        VkMemoryAllocateInfo MemAlloc;
        VkDeviceMemory Memory;
        VkImageView View;
    } Depth;
    
    TextureObject Textures[TEXTURE_COUNT];
    TextureObject StagingTexture;

    VkCommandBuffer Commandbuffer;  // Buffer for initialization commands
    VkPipelineLayout PipelineLayout;
    VkDescriptorSetLayout DescLayout;
    VkPipelineCache PipelineCache;
    VkRenderPass RenderPass;
    VkPipeline Pipeline;

    Mat4x4 ProjectionMatrix;
    Mat4x4 ViewMatrix;
    Mat4x4 ModelMatrix;

    float SpinAngle;

    VkShaderModule VertShaderModule;
    VkShaderModule FragShaderModule;

    VkDescriptorPool DescPool;

    int32 CurrentFrame;
    int32 FrameCount;
    bool32 Validate;
    bool32 ValidateChecksDisabled;
    bool32 UseBreak;
    bool32 SuppressPopups;

    uint32 CurrentBuffer;
    uint32 QueueFamilyCount;
} Renderer;

typedef struct Win32WindowInfo
{
    int32 Width;
    int32 Height;
    bool32 Running;
    bool32 WindowResized;
} Win32WindowInfo;

// Returns a pointer to a some block of memory in 'storage' and increments storage past this block
inline static void* PushToStorage(GameMemory* memory, uint64 allocSize)
{
    // TODO: implement good dynamic memory pushing/popping
    void* allocation = memory->DynamicStorageCurrent;
    memory->DynamicStorageCurrent = (uint8*)memory->DynamicStorageCurrent + allocSize;
    memory->DynamicStorageCurrentSize += allocSize;
    
    return allocation;
}

// Zeroes out the storage memory and resets the current pointer
inline static void ResetStorage(GameMemory* memory)
{
    memset(memory->DynamicStorage, 0, memory->DynamicStorageCurrentSize);
    memory->DynamicStorageCurrent = memory->DynamicStorage;
    memory->DynamicStorageCurrentSize = 0;
}