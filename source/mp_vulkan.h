#pragma once

#include "..\Vulkan\Include\vulkan\vulkan.h"

#include "mariposa_core.h"
#include <stdint.h>

const uint32 MP_VK_FORMAT_MAX = 10;
const uint32 MP_VK_PRESENTMODE_MAX = 10;
const uint32 MP_VK_SWAP_IMAGE_MAX = 10;
const uint32 MP_VK_SWAP_CHAIN_BUFFER_COUNT = 10;
const uint32 MP_VK_FRAMES_IN_FLIGHT_MAX = 2;

const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#if MP_INTERNAL
const bool32 enableValidationLayers = true;
#else
const bool32 enableValidationLayers = false;
#endif

struct VulkanData
{
    VkInstance Instance;
    
    VkSurfaceKHR Surface;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    
    VkSwapchainKHR SwapChain;
    VkImage SwapChainImages[MP_VK_SWAP_IMAGE_MAX];
    VkImageView SwapChainImageViews[MP_VK_SWAP_IMAGE_MAX];
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
    
    VkRenderPass RenderPass;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;
    
    VkFramebuffer Framebuffers[MP_VK_SWAP_CHAIN_BUFFER_COUNT];
    uint32 FramebufferCount;
    uint32 currentFrame;
    bool32* FramebufferResized;
    
    VkCommandPool CommandPool;
    VkCommandBuffer Commandbuffers[MP_VK_SWAP_CHAIN_BUFFER_COUNT];
    
    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderFinishedSemaphore;
    VkSemaphore ImageAvailableSemaphores[MP_VK_FRAMES_IN_FLIGHT_MAX];
    VkSemaphore RenderFinishedSemaphores[MP_VK_FRAMES_IN_FLIGHT_MAX];
    VkFence InFlightFences[MP_VK_FRAMES_IN_FLIGHT_MAX];
    VkFence InFlightImages[MP_VK_SWAP_IMAGE_MAX];
    
    debug_read_file_result VertexShader;
    debug_read_file_result FragmentShader;
};

struct QueueFamilyIndices {
    uint32 GraphicsFamily;
    bool32 HasGraphicsFamily;
    
    uint32 PresentFamily;
    bool32 HasPresentFamily;
    
    bool32 IsComplete;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities;
    VkSurfaceFormatKHR Formats[MP_VK_FORMAT_MAX];
    VkPresentModeKHR PresentModes[MP_VK_PRESENTMODE_MAX];
    
    bool32 IsAdequate;
};

static uint32 Uint32Clamp(uint32 value, uint32 min, uint32 max)
{
    if(value < min)
        return min;
    else if(value > max)
        return max;
    
    return value;    
}