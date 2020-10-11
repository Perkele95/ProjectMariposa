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
    
    if(enableValidationLayers)
    {
        createInfo.enabledLayerCount = ArrayCount(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;
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
    for(int i = 0; i < ArrayCount(validationLayers); i++)
    {
        for(uint32 k = 0; k < layerCount; k++)
        {
            if(strcmp(validationLayers[i], availableLayers[k].layerName) == 0)
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
    
    for(uint32 i = 0; i < queueFamiltyCount; i++)
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
    }
    
    return indices;
}

static bool CheckDeviceExtensionSupport(VkPhysicalDevice physDevice)
{
    uint32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);
    VkExtensionProperties availableExtensions[150] = {};
    MP_ASSERT(extensionCount <= ArrayCount(availableExtensions));
    vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions);
    // TODO: replace the nested for loops with a function, it is similar to those in checkvalidationlayersupport
    bool32 extensionFound = false;
    for(int i = 0; i < ArrayCount(deviceExtensions); i++)
    {
        for(uint32 k = 0; k < extensionCount; k++)
        {
            if(strcmp(deviceExtensions[i], availableExtensions[k].extensionName) == 0)
            {
                extensionFound = true;
                break;
            }
        }
    }
    
    return extensionFound;
}

static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &details.Capabilities);
    
    uint32 formatCount = 0;
    bool32 formatsSet = false;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);
    if(formatCount > 0)
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, details.Formats);
        formatsSet = true;
    }
    
    uint32 presentModeCount = 0;
    bool32 presentModesSet = false;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
    if(presentModeCount > 0)
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, details.PresentModes);
        presentModesSet = true;
    }
    
    if(formatsSet && presentModesSet)
        details.IsAdequate = true;
    
    return details;
}

static bool IsDeviceSuitable(VulkanData* vkData, VkPhysicalDevice* checkedPhysDevice)
{
    // TODO: Implement a better suitability checker that selects the best device given a score
    QueueFamilyIndices indices = FindQueueFamilyIndices(vkData, checkedPhysDevice);
    
    bool extensionsSupported = CheckDeviceExtensionSupport(*checkedPhysDevice);
    bool swapChainAdequate = false;
    if(extensionsSupported)
    {
        SwapChainSupportDetails swapChainDetails = QuerySwapChainSupport(*checkedPhysDevice, vkData->Surface);
        swapChainAdequate = swapChainDetails.IsAdequate;
    }
    
    return indices.IsComplete && extensionsSupported && swapChainAdequate;
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
    uint32 uniqueQueueFamilies[] = { indices.GraphicsFamily, indices.PresentFamily };
    
    float queuePriority = 1.0f;
    for(uint32 i = 0; i < ArrayCount(uniqueQueueFamilies); i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = i;//uniqueQueueFamilies[i];
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
    createInfo.enabledExtensionCount = ArrayCount(deviceExtensions);
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    
    if(enableValidationLayers)
    {
        createInfo.enabledLayerCount = ArrayCount(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;
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

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(VkSurfaceFormatKHR availableFormats[])
{
    for(int i = 0; i < ArrayCount(availableFormats); i++)
    {
        if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormats[i];
    }
    
    return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(VkPresentModeKHR availablePresentModes[])
{
    /* NOTE:
        VK_PRESENT_MODE_IMMEDIATE_KHR   == Vsync OFF
        VK_PRESENT_MODE_FIFO_KHR        == Vsync ON, double buffering
        VK_PRESENT_MODE_MAILBOX_KHR     == Vsync ON, triple buffering
    */
    for(int i = 0; i < ArrayCount(availablePresentModes); i++)
    {
        if(availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentModes[i];
    }
    
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities)
{
    if(capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { MP_SCREEN_WIDTH, MP_SCREEN_HEIGHT };
        
        
        actualExtent.width = Uint32Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = Uint32Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        return actualExtent;
    }
}

static void CreateSwapChain(VulkanData* vkData)
{
    SwapChainSupportDetails swapChainDetails = QuerySwapChainSupport(vkData->PhysicalDevice, vkData->Surface);
    
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainDetails.Formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainDetails.PresentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainDetails.Capabilities);
    
    uint32 imageCount = swapChainDetails.Capabilities.minImageCount + 1;
    if(swapChainDetails.Capabilities.maxImageCount > 0 && imageCount > swapChainDetails.Capabilities.maxImageCount)
    {
        imageCount = swapChainDetails.Capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkData->Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    // TODO: Cache this value instead of calling it 3 bloody times
    QueueFamilyIndices indices = FindQueueFamilyIndices(vkData, &vkData->PhysicalDevice);
    uint32 queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily };
    
    if(indices.GraphicsFamily != indices.PresentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    
    createInfo.preTransform = swapChainDetails.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    VkResult result = vkCreateSwapchainKHR(vkData->Device, &createInfo, nullptr, &vkData->SwapChain);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create swap chain!");
    
    MP_ASSERT(imageCount <= ArrayCount(vkData->SwapChainImages));
    vkGetSwapchainImagesKHR(vkData->Device, vkData->SwapChain, &imageCount, nullptr);
    vkGetSwapchainImagesKHR(vkData->Device, vkData->SwapChain, &imageCount, vkData->SwapChainImages);
    
    vkData->SwapChainImageFormat = surfaceFormat.format;
    vkData->SwapChainExtent = extent;
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
    
    OutputDebugStringA("Vulkan Data initialised\n");
    
    if(enableValidationLayers && !CheckValidationLayerSupport(vkData))
        OutputDebugStringA("Validation layers requested, but not available!\n");
    
    // IMPORTANT: Order matters
    CreateInstance(vkData);
    CreateWindowSurface(vkData, hInstance, window);
    PickPhysicalDevice(vkData);
    CreateLogicalDevice(vkData);
    CreateSwapChain(vkData);
    
    return vkData;
}

void VulkanUpdate(void)
{
    
}

void VulkanCleanup(VulkanData* vkData)
{
    vkDestroySwapchainKHR(vkData->Device, vkData->SwapChain, nullptr);
    vkDestroyDevice(vkData->Device, nullptr);
    vkDestroySurfaceKHR(vkData->Instance, vkData->Surface, nullptr);
    vkDestroyInstance(vkData->Instance, nullptr);
    OutputDebugStringA("Vulkan cleaned up\n");
}