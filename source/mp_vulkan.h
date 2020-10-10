#pragma once

#include "..\Vulkan\Include\vulkan\vulkan.h"

#include "mariposa_core.h"

struct VulkanData
{
    VkInstance Instance;
    char* ValidationLayers[1];
    bool32 enableValidationLayers;
    
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    
    VkQueue GraphicsQueue;
};

struct QueueFamilyIndices {
    uint32 GraphicsFamily;
    bool32 HasGraphicsFamily;
    bool32 IsComplete;
};