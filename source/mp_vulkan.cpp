#include "mp_vulkan.h"

static void CreateInstance(VulkanData* vkData)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Mariposa";
    appInfo.pEngineName = "Mariposa";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    if(vkData->enableValidationLayers)
    {
        createInfo.enabledLayerCount = ArrayCount(vkData->ValidationLayers);
        createInfo.ppEnabledLayerNames = vkData->ValidationLayers;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    
    VkResult result = vkCreateInstance(&createInfo, 0, &vkData->Instance);
    MP_ASSERT(result == VK_SUCCESS);
    OutputDebugStringA("Vulkan instance created\n");
    
    uint32 extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    
    // TODO: replace with dynamic array
    VkExtensionProperties extensionProperties[50] = {};
    MP_ASSERT(extensionsCount <= ArrayCount(extensionProperties));
    
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionProperties);
    
    OutputDebugStringA("Vulkan extensions:\n");
    for(uint32 i = 0; i < extensionsCount; i++)
    {
        OutputDebugStringA("    ");
        OutputDebugStringA(extensionProperties[i].extensionName);
        OutputDebugStringA("\n");
    }
}

static bool CheckValidationLayerSupport(VulkanData* vkData)
{
    uint32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    VkLayerProperties availableLayers[30] = {};
    MP_ASSERT(layerCount <= ArrayCount(availableLayers));
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
    
    bool32 layerFound = false;
    for(int i = 0; i < ArrayCount(vkData->ValidationLayers); i++)
    {
        for(int k = 0; k < ArrayCount(availableLayers); k++)
        {
            if(strcmp(vkData->ValidationLayers[i], availableLayers[k].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
    }
    
    return layerFound;
}

static QueueFamilyIndices FindQueueFamilyIndices(VkPhysicalDevice device) {
    QueueFamilyIndices indices = {};
    uint32 queueFamiltyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiltyCount, nullptr);
    VkQueueFamilyProperties queueFamilyProperties[30] = {};
    MP_ASSERT(queueFamiltyCount < ArrayCount(queueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiltyCount, queueFamilyProperties);
    
    int i = 0;
    for(uint32 k = 0; k < queueFamiltyCount; k++)
    {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.GraphicsFamily = i;
            if(!indices.HasGraphicsFamily)
                indices.HasGraphicsFamily = true;
        }
        
        if(indices.HasGraphicsFamily)
        {
            indices.IsComplete = true;
            break;
        }
        
        i++;
    }
    
    return indices;
}

static bool IsDeviceSuitable(VkPhysicalDevice device)
{
    // TODO: Implement a better suitability checker that selects the best device given a score
    QueueFamilyIndices indices = FindQueueFamilyIndices(device);
    
    return indices.IsComplete;
}

static void PickPhysicalDevice(VulkanData* vkData)
{
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(vkData->Instance, &deviceCount, nullptr);
    if(deviceCount == 0)    // TODO: replace with runtime error
        OutputDebugStringA("Failed to find GPUs with Vulkan Support");
    
    VkPhysicalDevice devices[10] = {};
    MP_ASSERT(deviceCount <= ArrayCount(devices));
    vkEnumeratePhysicalDevices(vkData->Instance, &deviceCount, devices);
    
    for(uint32 i = 0; i < deviceCount; i++)
    {
        if(IsDeviceSuitable(devices[i]))
        {
            vkData->PhysicalDevice = devices[i];
            break;
        }
    }
    
    if(vkData->PhysicalDevice == VK_NULL_HANDLE)
        OutputDebugStringA("Failed to find a suitable GPU!");
}

static void CreateLogicalDevice(VulkanData* vkData)
{
    QueueFamilyIndices indices = FindQueueFamilyIndices(vkData->PhysicalDevice);
    
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.HasGraphicsFamily;
    queueCreateInfo.queueCount = 1;
    
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledLayerCount = 0;
    
    VkResult result = vkCreateDevice(vkData->PhysicalDevice, &createInfo, nullptr, &vkData->Device);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create logical device!");
    
    vkGetDeviceQueue(vkData->Device, indices.HasGraphicsFamily, 0, &vkData->GraphicsQueue);
}

VulkanData* VulkanInit(MP_MEMORY* gameMemory)
{
    if(gameMemory->IsInitialised)
        gameMemory->IsInitialised = true;
        
    VulkanData* vkData = (VulkanData*)gameMemory->PermanentStorage;
    gameMemory->PermanentStorage = (VulkanData*)gameMemory->PermanentStorage + 1;
    
    vkData->ValidationLayers[0] = {"VK_LAYER_KHRONOS_validation"};
    #if MP_INTERNAL
    vkData->enableValidationLayers = true;
    #else
    vkData->enableValidationLayers = false;
    #endif
    OutputDebugStringA("Vulkan Data initialised\n");
    
    if(vkData->enableValidationLayers && !CheckValidationLayerSupport(vkData))
        OutputDebugStringA("Validation layers requested, but not available!\n");
    
    CreateInstance(vkData);
    PickPhysicalDevice(vkData);
    CreateLogicalDevice(vkData);
    
    return vkData;
}

void VulkanUpdate(void)
{
    
}

void VulkanCleanup(VulkanData* vkData)
{
    vkDestroyDevice(vkData->Device, nullptr);
    vkDestroyInstance(vkData->Instance, nullptr);
    OutputDebugStringA("Vulkan cleaned up\n");
}