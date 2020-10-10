#include "mp_vulkan.h"
#include "..\Vulkan\Include\vulkan\vulkan_win32.h"

#define MP_VK_EXT_COUNT 2
inline static void GetRequiredExtensions(char** extensions)
{
    extensions[0] = "VK_KHR_surface";
    extensions[1] = "VK_KHR_win32_surface";
}

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
    
    uint32 extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    
    // TODO: replace with dynamic array
    VkExtensionProperties extensionProperties[30] = {};
    MP_ASSERT(extensionsCount <= ArrayCount(extensionProperties));
    
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionProperties);
    
    char* extensions[MP_VK_EXT_COUNT] = {};
    GetRequiredExtensions(extensions);
    createInfo.enabledExtensionCount = MP_VK_EXT_COUNT;
    createInfo.ppEnabledExtensionNames = extensions;
    
    OutputDebugStringA("Vulkan extensions:\n");
    for(uint32 i = 0; i < extensionsCount; i++)
    {
        OutputDebugStringA("    ");
        OutputDebugStringA(extensionProperties[i].extensionName);
        OutputDebugStringA("\n");
    }
            
    VkResult result = vkCreateInstance(&createInfo, 0, &vkData->Instance);
    MP_ASSERT(result == VK_SUCCESS);
    OutputDebugStringA("Vulkan instance created\n");
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

static QueueFamilyIndices FindQueueFamilyIndices(VulkanData* vkData, VkPhysicalDevice* checkedPhysDevice) {
    QueueFamilyIndices indices = {};
    uint32 queueFamiltyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamiltyCount, nullptr);
    VkQueueFamilyProperties queueFamilyProperties[30] = {};
    MP_ASSERT(queueFamiltyCount < ArrayCount(queueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamiltyCount, queueFamilyProperties);
    
    int i = 0;
    for(uint32 k = 0; k < queueFamiltyCount; k++)
    {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.GraphicsFamily = i;
            if(!indices.HasGraphicsFamily)
                indices.HasGraphicsFamily = true;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(*checkedPhysDevice, i, vkData->Surface, &presentSupport);
        if(presentSupport)
        {
            indices.PresentFamily = i;
            if(!indices.HasPresentFamily)
                indices.HasPresentFamily = true;
        }
        
        if(indices.HasGraphicsFamily && indices.HasPresentFamily)
        {
            indices.IsComplete = true;
            break;
        }
        
        i++;
    }
    
    return indices;
}

static bool IsDeviceSuitable(VulkanData* vkData, VkPhysicalDevice* checkedPhysDevice)
{
    // TODO: Implement a better suitability checker that selects the best device given a score
    QueueFamilyIndices indices = FindQueueFamilyIndices(vkData, checkedPhysDevice);
    
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
        if(IsDeviceSuitable(vkData, &devices[i]))
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
    // TODO: This call has been made before, it could be stored into vkData and passed here for less computation
    QueueFamilyIndices indices = FindQueueFamilyIndices(vkData, &vkData->PhysicalDevice);
    
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
    int uniqueQueueFamilies[2] = {};
    uniqueQueueFamilies[0] = indices.HasGraphicsFamily;
    uniqueQueueFamilies[1] = indices.HasPresentFamily;
    
    float queuePriority = 1.0f;
    for(int i = 0; i < ArrayCount(uniqueQueueFamilies); i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }
    
    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = ArrayCount(queueCreateInfos);
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    if(vkData->enableValidationLayers)
    {
        createInfo.enabledLayerCount = ArrayCount(vkData->ValidationLayers);
        createInfo.ppEnabledLayerNames = vkData->ValidationLayers;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }
    
    VkResult result = vkCreateDevice(vkData->PhysicalDevice, &createInfo, nullptr, &vkData->Device);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create logical device!");
    
    vkGetDeviceQueue(vkData->Device, indices.HasGraphicsFamily, 0, &vkData->GraphicsQueue);
    vkGetDeviceQueue(vkData->Device, indices.HasPresentFamily, 0, &vkData->PresentQueue);
}

static void CreateWindowSurface(VulkanData* vkData, HINSTANCE* hInstance, HWND* window)
{
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = *window;
    createInfo.hinstance = *hInstance;
    
    VkResult result = vkCreateWin32SurfaceKHR(vkData->Instance, &createInfo, nullptr, &vkData->Surface);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create window surface!");
}

VulkanData* VulkanInit(MP_MEMORY* gameMemory, HINSTANCE* hInstance, HWND* window)
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
    
    // IMPORTANT: Order matters
    CreateInstance(vkData);
    CreateWindowSurface(vkData, hInstance, window);
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
    vkDestroySurfaceKHR(vkData->Instance, vkData->Surface, nullptr);
    vkDestroyInstance(vkData->Instance, nullptr);
    OutputDebugStringA("Vulkan cleaned up\n");
}