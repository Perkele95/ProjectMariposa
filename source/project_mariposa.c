#include "project_mariposa.h"

#include <stdio.h>

#include "win32_bmp_loader.h"

static Win32WindowInfo GlobalWindowInfo = {0};

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

static void RendererVKInit(Renderer *renderer, GameMemory* memory)
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
            VkLayerProperties* instanceLayers = (VkLayerProperties*)PushToStorage(memory, sizeof(VkLayerProperties) * instanceLayerCount);
            error = vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers);
            MP_ASSERT(!error);
            
            validationFound = CheckLayers(ArrayCount(instanceValidationLayers), instanceValidationLayers, instanceLayerCount, instanceLayers);
            if(validationFound)
            {
                renderer->EnabledLayerCount = ArrayCount(instanceValidationLayers);
                renderer->EnabledLayers[0] = "VK_LAYER_KHRONOS_validation";
            }
            //free(instanceLayers);
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
        VkExtensionProperties* instanceExtensions = (VkExtensionProperties*)PushToStorage(memory, sizeof(VkExtensionProperties) * instanceExtensionCount);
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
        .enabledExtensionCount = renderer->EnabledExtensionCount,
        .ppEnabledExtensionNames = (const char *const *)renderer->ExtensionNames
    };
    
    error = vkCreateInstance(&instanceInfo, NULL, &renderer->Instance);
    if(error == VK_ERROR_INCOMPATIBLE_DRIVER)
        OutputDebugStringA("Incompatible driver!\n\n");
    else if(error == VK_ERROR_EXTENSION_NOT_PRESENT)
        OutputDebugStringA("Extension library not present!\n\n");
    else if(error)
        OutputDebugStringA("VkCreateInstance failed!\n\n");
    
    uint32 gpuCount;
    
    error = vkEnumeratePhysicalDevices(renderer->Instance, &gpuCount, NULL);
    MP_ASSERT(!error)
    
    if(gpuCount > 0)
    {
        VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*)PushToStorage(memory, sizeof(VkPhysicalDevice) * gpuCount);
        error = vkEnumeratePhysicalDevices(renderer->Instance, &gpuCount, physicalDevices);
        MP_ASSERT(!error);
        if(renderer->GpuNumber > gpuCount - 1)
        {
            OutputDebugStringA("Gpu specified is not present");
            OutputDebugStringA("Continuing with gpu 0\n");
            renderer->GpuNumber = 0;
        }
        renderer->Gpu = physicalDevices[renderer->GpuNumber];
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
        VkExtensionProperties* deviceExtensions = (VkExtensionProperties*)PushToStorage(memory, sizeof(VkExtensionProperties) * deviceExtensionCount);
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
    }
    
    if(!SwapChainExtensionFound)
        OutputDebugStringA("vkEnumerateDeviceExtensionProperties failed to find the 'VK_KHR_swapchain' extension.\n\n");
    
    vkGetPhysicalDeviceProperties(renderer->Gpu, &renderer->GpuProps);
    
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->Gpu, &renderer->QueueFamilyCount, NULL);
    MP_ASSERT(renderer->QueueFamilyCount >= 1);
    
    renderer->QueueProps = (VkQueueFamilyProperties*) PushToStorage(memory, renderer->QueueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->Gpu, &renderer->QueueFamilyCount, renderer->QueueProps);
    
    VkPhysicalDeviceFeatures physDevFeatures;
    vkGetPhysicalDeviceFeatures(renderer->Gpu, &physDevFeatures);
}

static void RendererInit(Renderer *renderer, GameMemory* memory)
{
    Vec3 eye = {0.0f, 3.0f, 5.0f};
    Vec3 center = {0, 0, 0};
    Vec3 up = {0.0f, 1.0f, 0.0};
    
    renderer->PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    renderer->FrameCount = INT32_MAX;
    renderer->GpuNumber = 0;
    renderer->Validate = true;
    
    RendererVKInit(renderer, memory);
    
    renderer->Width = 1280;
    renderer->Height = 720;
    renderer->SpinAngle = 4.0f;
    
    float aspectRatio = (float)renderer->Width / (float)renderer->Height;
    renderer->ProjectionMatrix = Mat4x4Perspective(PI32 / 4.0f, aspectRatio, 0.1f, 50.0f);
    renderer->ViewMatrix = Mat4x4LookAt(eye, center, up);
    renderer->ModelMatrix = Mat4x4Identity();
}

static void CreateWin32Surface(Renderer* renderer)
{
    VkResult error;
    
    VkWin32SurfaceCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = renderer->HInstance;
    createInfo.hwnd = renderer->Window;
    
    error = vkCreateWin32SurfaceKHR(renderer->Instance, &createInfo, NULL, &renderer->Surface);
    MP_ASSERT(!error);
}

static void CreateDevice(Renderer* renderer)
{
    VkResult error;
    float queuePriorities[1] = {0.0f};
    VkDeviceQueueCreateInfo queueInfos[2] = {0};
    queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfos[0].queueFamilyIndex = renderer->GraphicsQueueFamilyIndex;
    queueInfos[0].queueCount = 1;
    queueInfos[0].pQueuePriorities = queuePriorities;
    
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = queueInfos,
        .enabledExtensionCount = renderer->EnabledExtensionCount,
        .ppEnabledExtensionNames = (const char *const *)renderer->ExtensionNames,
        .ppEnabledLayerNames = NULL,
        .pNext = NULL,
        .pEnabledFeatures = NULL,
        .flags = 0
    };
    
    if(renderer->SeparatePresentQueue)
    {
        queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[1].queueFamilyIndex = renderer->PresentQueueFamilyIndex;
        queueInfos[1].queueCount = 1;
        queueInfos[1].pQueuePriorities = queuePriorities;
        deviceInfo.queueCreateInfoCount = 2;
    }
    
    error = vkCreateDevice(renderer->Gpu, &deviceInfo, NULL, &renderer->Device);
    MP_ASSERT(!error);
}

static VkSurfaceFormatKHR PickSurfaceFormat(const VkSurfaceFormatKHR* surfaceFormats, uint32 formatCount)
{
    for(uint32 i = 0; i < formatCount; i++)
    {
        const VkFormat format = surfaceFormats[i].format;
        if(format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM ||
            format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
            format == VK_FORMAT_R16G16B16A16_SFLOAT)
            return surfaceFormats[i];
    }
    OutputDebugStringA("Can't find our preferred formats... Falling back to first exposed format. Rendering may be incorrect.\n");
    MP_ASSERT(formatCount >= 1);
    return surfaceFormats[0];
}

static void InitVkSwapChain(Renderer* renderer, GameMemory* memory)
{
    VkResult error;
    
    CreateWin32Surface(renderer);
    
    VkBool32* supportsPresent = (VkBool32*)PushToStorage(memory, renderer->QueueFamilyCount * sizeof(VkBool32));
    for(uint32 i = 0; i < renderer->QueueFamilyCount; i++)
        vkGetPhysicalDeviceSurfaceSupportKHR(renderer->Gpu, i, renderer->Surface, &supportsPresent[i]);
    
    uint32 graphicsQueueFamilyIndex = UINT32_MAX;
    uint32 presentQueueFamilyIndex = UINT32_MAX;
    for(uint32 i = 0; i < renderer->QueueFamilyCount; i++)
    {
        if((renderer->QueueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != false)
        {
            if(graphicsQueueFamilyIndex == UINT32_MAX)
                graphicsQueueFamilyIndex = i;
            
            if(supportsPresent[i] == VK_TRUE)
            {
                graphicsQueueFamilyIndex = i;
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }
    
    if(presentQueueFamilyIndex == UINT32_MAX)
    {
        for(uint32 i = 0; i < renderer->QueueFamilyCount; i++)
        {
            if(supportsPresent[i] == VK_TRUE)
            {
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }
    
    if(graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX)
    {
        OutputDebugStringA("Could not find graphics and present queues\n\n");
        GlobalWindowInfo.Running = false;
    }
    
    renderer->GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    renderer->PresentQueueFamilyIndex = presentQueueFamilyIndex;
    renderer->SeparatePresentQueue = (graphicsQueueFamilyIndex != presentQueueFamilyIndex);
    
    CreateDevice(renderer);
    
    vkGetDeviceQueue(renderer->Device, renderer->GraphicsQueueFamilyIndex, 0, &renderer->GraphicsQueue);
    if(renderer->SeparatePresentQueue)
        vkGetDeviceQueue(renderer->Device, renderer->PresentQueueFamilyIndex, 0, &renderer->PresentQueue);
    else
        renderer->PresentQueue = renderer->GraphicsQueue;
    
    uint32 formatCount = 0;
    error = vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->Gpu, renderer->Surface, &formatCount, NULL);
    MP_ASSERT(!error);
    VkSurfaceFormatKHR* surfaceFormats = (VkSurfaceFormatKHR*)PushToStorage(memory, formatCount * sizeof(VkSurfaceFormatKHR));
    error = vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->Gpu, renderer->Surface, &formatCount, surfaceFormats);
    MP_ASSERT(!error);
    
    VkSurfaceFormatKHR surfaceFormat = PickSurfaceFormat(surfaceFormats, formatCount);
    renderer->Format = surfaceFormat.format;
    renderer->ColorSpace = surfaceFormat.colorSpace;
    renderer->CurrentFrame = 0;
    
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };
    
    for(uint32 i = 0; i < FRAME_LAG; i++)
    {
        error = vkCreateFence(renderer->Device, &fenceInfo, NULL, &renderer->Fences[i]);
        MP_ASSERT(!error);
        error = vkCreateSemaphore(renderer->Device, &semaphoreInfo, NULL, &renderer->ImageAcquiredSemaphores[i]);
        MP_ASSERT(!error);
        error = vkCreateSemaphore(renderer->Device, &semaphoreInfo, NULL, &renderer->DrawCompleteSemaphores[i]);
        MP_ASSERT(!error);
        
        if(renderer->SeparatePresentQueue)
        {
            error = vkCreateSemaphore(renderer->Device, &semaphoreInfo, NULL, &renderer->ImageOwnershipSemaphores[i]);
            MP_ASSERT(!error);
        }
    }
    renderer->FrameIndex = 0;
    
    vkGetPhysicalDeviceMemoryProperties(renderer->Gpu, &renderer->MemoryProps);
}

static void PrepareBuffers(Renderer* renderer, GameMemory* memory)
{
    VkResult error;
    VkSwapchainKHR oldSwapchain = renderer->Swapchain;
    
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    error = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->Gpu, renderer->Surface, &surfaceCapabilities);
    MP_ASSERT(!error);
    
    uint32 presentModeCount = 0;
    error = vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->Gpu, renderer->Surface, &presentModeCount, NULL);
    MP_ASSERT(!error);
    VkPresentModeKHR* presentModes = (VkPresentModeKHR*) PushToStorage(memory, presentModeCount * sizeof(VkPresentModeKHR));
    MP_ASSERT(presentModes);
    error = vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->Gpu, renderer->Surface, &presentModeCount, presentModes);
    MP_ASSERT(!error);
    
    VkExtent2D swapchainExtent;
    if(surfaceCapabilities.currentExtent.width == UINT32_MAX)
    {
        swapchainExtent.width = renderer->Width;
        swapchainExtent.height = renderer->Height;
        
        swapchainExtent.width = Uint32Clamp(swapchainExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapchainExtent.height = Uint32Clamp(swapchainExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);        
    }
    else
    {
        swapchainExtent = surfaceCapabilities.currentExtent;
        renderer->Width = surfaceCapabilities.currentExtent.width;
        renderer->Height = surfaceCapabilities.currentExtent.height;
    }
    
    if(renderer->Width == 0 || renderer->Height == 0)
    {
        renderer->IsMinimised = true;
        return;
    }
    else
    {
        renderer->IsMinimised = false;
    }
    
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    
    if(renderer->PresentMode != swapchainPresentMode)
    {
        for(uint32 i = 0; i < presentModeCount; i++)
        {
            if(presentModes[i] == renderer->PresentMode)
            {
                swapchainPresentMode = renderer->PresentMode;
                break;
            }
        }
    }
    if(swapchainPresentMode != renderer->PresentMode)
        OutputDebugStringA("Presentmode specified is not supported\n\n");
    
    // NOTE: Triple buffering is set by default
    uint32 desiredSwapchainImageCount = 3;
    if(desiredSwapchainImageCount < surfaceCapabilities.minImageCount)
        desiredSwapchainImageCount = surfaceCapabilities.minImageCount;
    
    if((surfaceCapabilities.maxImageCount > 0) && (desiredSwapchainImageCount > surfaceCapabilities.maxImageCount))
        desiredSwapchainImageCount = surfaceCapabilities.maxImageCount;
    
    VkSurfaceTransformFlagsKHR preTransform;
    if(surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        preTransform = surfaceCapabilities.currentTransform;
    
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };
    for(uint32 i = 0; i < ArrayCount(compositeAlphaFlags); i++)
    {
        if(surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
        {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }
    
    VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .surface = renderer->Surface,
        .minImageCount = desiredSwapchainImageCount,
        .imageFormat = renderer->Format,
        .imageColorSpace = renderer->ColorSpace,
        .imageExtent = {
                .width = swapchainExtent.width,
                .height = swapchainExtent.height,
            },
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = preTransform,
        .compositeAlpha = compositeAlpha,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = swapchainPresentMode,
        .oldSwapchain = oldSwapchain,
        .clipped = true,
    };
    error = vkCreateSwapchainKHR(renderer->Device, &swapchainInfo, NULL, &renderer->Swapchain);
    MP_ASSERT(!error);
    
    if(oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(renderer->Device, oldSwapchain, NULL);
    
    error = vkGetSwapchainImagesKHR(renderer->Device, renderer->Swapchain, &renderer->SwapChainImageCount, NULL);
    MP_ASSERT(!error);
    
    VkImage* swapchainImages = (VkImage*) PushToStorage(memory, renderer->SwapChainImageCount * sizeof(VkImage));
    MP_ASSERT(swapchainImages);
    error = vkGetSwapchainImagesKHR(renderer->Device, renderer->Swapchain, &renderer->SwapChainImageCount, swapchainImages);
    MP_ASSERT(!error);
    
    renderer->SwapchainImageResources = (SwapchainImageResources*) PushToStorage(memory, renderer->SwapChainImageCount * sizeof(SwapchainImageResources));
    MP_ASSERT(renderer->SwapchainImageResources);
    
    for(uint32 i = 0; i < renderer->SwapChainImageCount; i++)
    {
        VkImageViewCreateInfo imageView = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = renderer->Format,
            .components = {
                    .r = VK_COMPONENT_SWIZZLE_R,
                    .g = VK_COMPONENT_SWIZZLE_G,
                    .b = VK_COMPONENT_SWIZZLE_B,
                    .a = VK_COMPONENT_SWIZZLE_A,
                },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0,
                .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1
                },
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0
        };
        
        renderer->SwapchainImageResources[i].Image = swapchainImages[i];
        imageView.image = swapchainImages[i];
        
        error = vkCreateImageView(renderer->Device, &imageView, NULL, &renderer->SwapchainImageResources->View);
        MP_ASSERT(!error);
    }
}

static bool32 MemoryTypeFromProps(Renderer* renderer, uint32 typeBits, VkFlags reqMask, uint32* typeIndex)
{
    for(uint32 i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        if((typeBits & 1) && ((renderer->MemoryProps.memoryTypes[i].propertyFlags & reqMask) == reqMask))
        {
            *typeIndex = i;
            return true;
        }
        typeBits >>= 1;
    }
    return false;
}

static void PrepareDepth(Renderer* renderer)
{
    const VkFormat depthFormat = VK_FORMAT_D16_UNORM;
    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent = {renderer->Width, renderer->Height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0
    };
    
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = VK_NULL_HANDLE,
        .format = depthFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,.baseMipLevel = 0,
            .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1
            },
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D
    };
    
    VkMemoryRequirements memReqs;
    VkResult error;
    bool32 success;
    
    renderer->Depth.Format = depthFormat;
    
    error = vkCreateImage(renderer->Device, &imageInfo, NULL, &renderer->Depth.Image);
    MP_ASSERT(!error);
    
    vkGetImageMemoryRequirements(renderer->Device, renderer->Depth.Image, &memReqs);
    
    renderer->Depth.MemAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    renderer->Depth.MemAlloc.pNext = NULL;
    renderer->Depth.MemAlloc.allocationSize = memReqs.size;
    renderer->Depth.MemAlloc.memoryTypeIndex = 0;
    
    success = MemoryTypeFromProps(renderer, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->Depth.MemAlloc.memoryTypeIndex);
    MP_ASSERT(success);
    
    error = vkAllocateMemory(renderer->Device, &renderer->Depth.MemAlloc, NULL, &renderer->Depth.Memory);
    MP_ASSERT(!error);
    
    error = vkBindImageMemory(renderer->Device, renderer->Depth.Image, renderer->Depth.Memory, 0);
    MP_ASSERT(!error);
    
    viewInfo.image = renderer->Depth.Image;
    error = vkCreateImageView(renderer->Device, &viewInfo, NULL, &renderer->Depth.View);
    MP_ASSERT(!error);
}

static void PrepareTextureImage(Renderer* renderer, const char* filename, TextureObject* texObj,
                                VkImageTiling tiling, VkImageUsageFlags usage, VkFlags reqProps)
{
    const VkFormat texFormat = VK_FORMAT_R8G8B8A8_UNORM;
    int32 texWidth, texHeight;
    VkResult error;
    bool32 success;
    uint8* pixels = LoadBMP(filename, &texWidth, &texHeight);
    
    texObj->TexWidth = texWidth;
    texObj->TexHeight = texHeight;
    
    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = texFormat,
        .extent = {texWidth, texHeight, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .flags = 0,
        .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
    };
    
    error = vkCreateImage(renderer->Device, &imageInfo, NULL, &texObj->Image);
    MP_ASSERT(!error);
    
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(renderer->Device, texObj->Image, &memReqs);
    
    texObj->MemAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texObj->MemAlloc.pNext = NULL;
    texObj->MemAlloc.allocationSize = memReqs.size;
    texObj->MemAlloc.memoryTypeIndex = 0;
    
    success = MemoryTypeFromProps(renderer, memReqs.memoryTypeBits, reqProps, &texObj->MemAlloc.memoryTypeIndex);
    MP_ASSERT(success);
    
    error = vkAllocateMemory(renderer->Device, &texObj->MemAlloc, NULL, &texObj->Memory);
    MP_ASSERT(!error);
    
    error = vkBindImageMemory(renderer->Device, texObj->Image, texObj->Memory, 0);
    MP_ASSERT(!error);
    
    if(reqProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        const VkImageSubresource subRes = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0
        };
        VkSubresourceLayout layout;
        void* data;
        
        vkGetImageSubresourceLayout(renderer->Device, texObj->Image, &subRes, &layout);
        
        error = vkMapMemory(renderer->Device, texObj->Memory, 0, texObj->MemAlloc.allocationSize, 0, &data);
        MP_ASSERT(!error);
        memcpy(data, pixels, (uint64)texObj->MemAlloc.allocationSize);
        vkUnmapMemory(renderer->Device, texObj->Memory);
    }
    
    texObj->ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    FreeBMP(pixels);
}

static void SetImageLayout(Renderer* renderer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
                           VkImageLayout newImageLayout, VkAccessFlagBits srcAccessMask, VkPipelineStageFlags srcStages,
                           VkPipelineStageFlags dstStages)
{
    MP_ASSERT(renderer->Commandbuffer);
    
    VkImageMemoryBarrier imageMemoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = 0,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .oldLayout = oldImageLayout,
        .newLayout = newImageLayout,
        .image = image,
        .subresourceRange = {aspectMask, 0, 1, 0, 1}
    };
    
    switch (newImageLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;
        default:
            imageMemoryBarrier.dstAccessMask = 0;
            break;
    }
    
    vkCmdPipelineBarrier(renderer->Commandbuffer, srcStages, dstStages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
}

static void PrepareTextureBuffer(Renderer* renderer, const char* filename, TextureObject* texObj)
{
    int32 texWidth, texHeight;
    VkResult error;
    bool32 success;
    uint8* pixels = LoadBMP(filename, &texWidth, &texHeight);
    
    texObj->TexWidth = texWidth;
    texObj->TexHeight = texHeight;
    
    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = texWidth * texHeight * 4,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL
    };
    
    error = vkCreateBuffer(renderer->Device, &bufferInfo, NULL, &texObj->Buffer);
    MP_ASSERT(!error);
    
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(renderer->Device, texObj->Buffer, &memReqs);
    
    texObj->MemAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texObj->MemAlloc.pNext = NULL;
    texObj->MemAlloc.allocationSize = memReqs.size;
    texObj->MemAlloc.memoryTypeIndex = 0;
    
    VkFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    success = MemoryTypeFromProps(renderer, memReqs.memoryTypeBits, reqs, &texObj->MemAlloc.memoryTypeIndex);
    MP_ASSERT(success);
    
    error = vkAllocateMemory(renderer->Device, &texObj->MemAlloc, NULL, &texObj->Memory);
    MP_ASSERT(!error);
    
    error = vkBindBufferMemory(renderer->Device, texObj->Buffer, texObj->Memory, 0);
    MP_ASSERT(!error);
    
    VkSubresourceLayout layout = {0};
    layout.rowPitch = texWidth * 4;
    
    void* data;
    error = vkMapMemory(renderer->Device, texObj->Memory, 0, texObj->MemAlloc.allocationSize, 0, &data);
    MP_ASSERT(!error);
    
    memcpy(data, pixels, (uint64)texObj->MemAlloc.allocationSize);
    vkUnmapMemory(renderer->Device, texObj->Memory);
    
    FreeBMP(pixels);
}

static void DestroyTexture(Renderer* renderer, TextureObject* texObj)
{
    vkFreeMemory(renderer->Device, texObj->Memory, NULL);
    if(texObj->Image) vkDestroyImage(renderer->Device, texObj->Image, NULL);
    if(texObj->Buffer) vkDestroyBuffer(renderer->Device, texObj->Buffer, NULL);
}

static void PrepareTextures(Renderer* renderer)
{
    const VkFormat texFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties props;
    
    vkGetPhysicalDeviceFormatProperties(renderer->Gpu, texFormat, &props);
    // Loop through global textures
    for(uint32 i = 0; i < TEXTURE_COUNT; i++)
    {
        VkResult error;
        if((props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !renderer->UseStagingBuffer)
        {
            PrepareTextureImage(renderer, gTexFiles[i], &renderer->Textures[i], VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                                
            SetImageLayout(renderer, renderer->Textures[i].Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                           renderer->Textures[i].ImageLayout, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            renderer->StagingTexture.Image = 0;
        }
        else if(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
        {
            memset(&renderer->StagingTexture, 0, sizeof(renderer->StagingTexture));
            PrepareTextureBuffer(renderer, gTexFiles[i], &renderer->StagingTexture);
            PrepareTextureImage(renderer, gTexFiles[i], &renderer->Textures[i], VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                                
            SetImageLayout(renderer, renderer->Textures[i].Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT);
            
            VkBufferImageCopy copyRegion = {
                .bufferOffset = 0,
                .bufferRowLength = renderer->StagingTexture.TexWidth,
                .bufferImageHeight = renderer->StagingTexture.TexHeight,
                .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .imageOffset = {0, 0, 0},
                .imageExtent = {renderer->StagingTexture.TexWidth, renderer->StagingTexture.TexHeight, 1}
            };
            
            vkCmdCopyBufferToImage(renderer->Commandbuffer, renderer->StagingTexture.Buffer, renderer->Textures[i].Image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            
            SetImageLayout(renderer, renderer->Textures[i].Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           renderer->Textures[i].ImageLayout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
        else
        {
            MP_ASSERT(!"No support for R8G8B8A8_UNORM as texture image format");
        }
        
        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE
        };
        
        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = texFormat,
            .components = {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A
                },
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0 , 1},
            .flags = 0
        };
        
        error = vkCreateSampler(renderer->Device, &sampler, NULL, &renderer->Textures[i].Sampler);
        MP_ASSERT(!error);
        
        view.image = renderer->Textures[i].Image;
        error = vkCreateImageView(renderer->Device, &view, NULL, &renderer->Textures[i].View);
        MP_ASSERT(!error);
    }
}

static void PrepareforRendering(Renderer* renderer, GameMemory* memory)
{
    VkResult error;
    if(renderer->CommandPool == VK_NULL_HANDLE)
    {
        const VkCommandPoolCreateInfo cmdPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = renderer->GraphicsQueueFamilyIndex,
            .flags = 0
        };
        error = vkCreateCommandPool(renderer->Device, &cmdPoolInfo, NULL, &renderer->CommandPool);
        MP_ASSERT(!error);
    }
    
    const VkCommandBufferAllocateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = renderer->CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    error = vkAllocateCommandBuffers(renderer->Device, &cmd, &renderer->Commandbuffer);
    MP_ASSERT(!error);
    
    VkCommandBufferBeginInfo cmdBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .pInheritanceInfo = NULL,
        .flags = 0
    };
    error = vkBeginCommandBuffer(renderer->Commandbuffer, &cmdBeginInfo);
    MP_ASSERT(!error);
    
    PrepareBuffers(renderer, memory);
    
    if(renderer->IsMinimised)
    {
        renderer->Prepared = false;
        return;
    }
    
    PrepareDepth(renderer);
    PrepareTextures(renderer);
    
    
    // CONT
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

static void InitMemory(GameMemory *memory)
{
    memory->PermanentStorageSize = MegaBytes(64);
    memory->DynamicStorageSize = GigaBytes(1);
    uint64 totalSize = memory->PermanentStorageSize + memory->DynamicStorageSize;
    
    #if MP_INTERNAL
        LPVOID baseAddress = (LPVOID)TeraBytes(2);
    #else
        LPVOID baseAddress = 0;
    #endif
    
    memory->PermanentStorage = VirtualAlloc(baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    memory->DynamicStorage = ((uint8*)memory->PermanentStorage + memory->PermanentStorageSize);
    memory->DynamicStorageCurrent = memory->DynamicStorage;
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    
    switch (message)
    {
        case WM_SIZE:
        {
            GlobalWindowInfo.Width = (int32)(lParam & 0x0000FFFF);
            GlobalWindowInfo.Height = (int32)(lParam >> 16);
            GlobalWindowInfo.WindowResized = true;
        } break;

        case WM_CLOSE:
        {
            GlobalWindowInfo.Running = false;
        } break;

        case WM_DESTROY:
        {
            GlobalWindowInfo.Running = false;
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            MP_ASSERT(!"Keyboard input came in through a non-dispatch message!");
        } break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

static void Win32CreateWindow(Renderer* renderer)
{
    WNDCLASSA windowClass = {0};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = renderer->HInstance;
    //WindowClass.hIcon;
    windowClass.lpszClassName = "MariposaWindowClass";
    
    if(!RegisterClassA(&windowClass))
        OutputDebugString("Application window class register failed\n");
    
    renderer->Window = CreateWindowExA(0, windowClass.lpszClassName, "Mariposa",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, renderer->HInstance, NULL);
    
    if(renderer->Window)
        GlobalWindowInfo.Running = true;
    else
        OutputDebugString("Application window creation failed\n");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    GameMemory memory = {0};
    InitMemory(&memory);
    
    Renderer *renderer = (Renderer*)memory.PermanentStorage;
    
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    
    bool32 pause = false;
    
    if(memory.PermanentStorage)
    {
        LARGE_INTEGER lastCounter = Win32GetClockValue();
    
        RendererInit(renderer, &memory);
        renderer->HInstance = hInstance;
        Win32CreateWindow(renderer);
        InitVkSwapChain(renderer, &memory);
        ResetStorage(&memory);
        
        PrepareforRendering(renderer, &memory);
        
        while(GlobalWindowInfo.Running)
        {
            // Peekmessage
            
            // UpdateTick
            // RenderTick
            
            LARGE_INTEGER endCounter = Win32GetClockValue();
            renderer->Timestep = Win32GetSecondsElapsed(lastCounter, endCounter, perfCountFrequencyResult);
            lastCounter = endCounter;
        }
        
        // VkCleanup
    }
    
    return 0;
}