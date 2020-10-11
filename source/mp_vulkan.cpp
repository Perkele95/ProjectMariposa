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

static void CreateImageViews(VulkanData* vkData)
{
    for(uint32 i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++)
    {
        if(vkData->SwapChainImages[i] == 0)
            break;
        
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vkData->SwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vkData->SwapChainImageFormat;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        VkResult result = vkCreateImageView(vkData->Device, &createInfo, nullptr, &vkData->SwapChainImageViews[i]);
        if(result != VK_SUCCESS)
            OutputDebugStringA("Failed to create image views!");
    }
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

static VkShaderModule CreateShaderModule(VkDevice* device, debug_read_file_result* file)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = file->dataSize;
    createInfo.pCode = (const uint32*)file->data;
    
    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create shader module!");
    
    return shaderModule;
}

static void CreateRenderPass(VulkanData* vkData)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = vkData->SwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    VkResult result = vkCreateRenderPass(vkData->Device, &renderPassInfo, nullptr, &vkData->RenderPass);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create render pass!");
}

static void CreateGraphicsPipeline(VulkanData* vkData)
{
    VkShaderModule vertexShaderModule = CreateShaderModule(&vkData->Device, &vkData->VertexShader);
    VkShaderModule fragmentShaderModule = CreateShaderModule(&vkData->Device, &vkData->FragmentShader);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShaderModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShaderModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vkData->SwapChainExtent.width;
    viewport.height = (float)vkData->SwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = vkData->SwapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // NOTE: Depth Stencil
    //VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlend = {};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.logicOpEnable = VK_FALSE;
    colorBlend.logicOp = VK_LOGIC_OP_COPY;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorBlendAttachment;
    colorBlend.blendConstants[0] = 0.0f;
    colorBlend.blendConstants[1] = 0.0f;
    colorBlend.blendConstants[2] = 0.0f;
    colorBlend.blendConstants[3] = 0.0f;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    
    VkResult layoutResult = vkCreatePipelineLayout(vkData->Device, &pipelineLayoutInfo, nullptr, &vkData->PipelineLayout);
    if(layoutResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create pipeline layout!");
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = vkData->PipelineLayout;
    pipelineInfo.renderPass = vkData->RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    VkResult gPipelineResult = vkCreateGraphicsPipelines(vkData->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkData->GraphicsPipeline);
    if(gPipelineResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create graphics pipeline!");
    
    vkDestroyShaderModule(vkData->Device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(vkData->Device, fragmentShaderModule, nullptr);
}

static void CreateFramebuffers(VulkanData* vkData)
{
    for(int i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++)
    {
        if(vkData->SwapChainImageViews[i] == 0)
            break;
        
        VkImageView attachments[] = { vkData->SwapChainImageViews[i] };
        
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vkData->RenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vkData->SwapChainExtent.width;
        framebufferInfo.height = vkData->SwapChainExtent.height;
        framebufferInfo.layers = 1;
        
        VkResult result = vkCreateFramebuffer(vkData->Device, &framebufferInfo, nullptr, &vkData->Framebuffers[i]);
        if(result != VK_SUCCESS)
            OutputDebugStringA("Failed to create framebuffer!");
    }
}

VulkanData* VulkanInit(MP_MEMORY* gameMemory, HINSTANCE* hInstance, HWND* window, debug_read_file_result* vertShader, debug_read_file_result* fragShader)
{
    if(gameMemory->IsInitialised)
        gameMemory->IsInitialised = true;
        
    VulkanData* vkData = (VulkanData*)gameMemory->PermanentStorage;
    gameMemory->PermanentStorage = (VulkanData*)gameMemory->PermanentStorage + 1;
    OutputDebugStringA("Vulkan Data initialised\n");
    
    vkData->VertexShader = *vertShader;
    vkData->FragmentShader = *fragShader;
    
    if(enableValidationLayers && !CheckValidationLayerSupport(vkData))
        OutputDebugStringA("Validation layers requested, but not available!\n");
    
    // IMPORTANT: Order matters
    CreateInstance(vkData);
    CreateWindowSurface(vkData, hInstance, window);
    PickPhysicalDevice(vkData);
    CreateLogicalDevice(vkData);
    CreateSwapChain(vkData);
    CreateImageViews(vkData);
    CreateRenderPass(vkData);
    CreateGraphicsPipeline(vkData);
    CreateFramebuffers(vkData);
    
    return vkData;
}

void VulkanUpdate(void)
{
    
}

void VulkanCleanup(VulkanData* vkData)
{
    for(int i = 0; i < MP_VK_SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        // TODO: skip emtpy elements perhaps
        vkDestroyFramebuffer(vkData->Device, vkData->Framebuffers[i], nullptr);
    }
    
    vkDestroyPipeline(vkData->Device, vkData->GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkData->Device, vkData->PipelineLayout, nullptr);
    vkDestroyRenderPass(vkData->Device, vkData->RenderPass, nullptr);
    
    for(int i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++)
    {
        vkDestroyImageView(vkData->Device, vkData->SwapChainImageViews[i], nullptr);
    }
    
    vkDestroySwapchainKHR(vkData->Device, vkData->SwapChain, nullptr);
    vkDestroyDevice(vkData->Device, nullptr);
    vkDestroySurfaceKHR(vkData->Instance, vkData->Surface, nullptr);
    vkDestroyInstance(vkData->Instance, nullptr);
    OutputDebugStringA("Vulkan cleaned up\n");
}