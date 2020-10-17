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
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
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
    QueueFamilyIndices indices = FindQueueFamilyIndices(vkData, checkedPhysDevice);
    
    bool extensionsSupported = CheckDeviceExtensionSupport(*checkedPhysDevice);
    bool swapChainAdequate = false;
    if(extensionsSupported)
    {
        SwapChainSupportDetails swapChainDetails = QuerySwapChainSupport(*checkedPhysDevice, vkData->Surface);
        swapChainAdequate = swapChainDetails.IsAdequate;
    }
    
    vkData->Indices = indices;
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
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
    uint32 uniqueQueueFamilies[] = { vkData->Indices.GraphicsFamily, vkData->Indices.PresentFamily };
    
    float queuePriority = 1.0f;
    for(uint32 i = 0; i < ArrayCount(uniqueQueueFamilies); i++)
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
    
    vkGetDeviceQueue(vkData->Device, vkData->Indices.GraphicsFamily, 0, &vkData->GraphicsQueue);
    vkGetDeviceQueue(vkData->Device, vkData->Indices.PresentFamily, 0, &vkData->PresentQueue);
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

VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, int windowWidth, int windowHeight)
{
    if(capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { (uint32)windowWidth, (uint32)windowHeight };
        
        actualExtent.width = Uint32Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = Uint32Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        return actualExtent;
    }
}

static void CreateSwapChain(VulkanData* vkData, int windowWidth, int windowHeight)
{
    SwapChainSupportDetails swapChainDetails = QuerySwapChainSupport(vkData->PhysicalDevice, vkData->Surface);
    
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainDetails.Formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainDetails.PresentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainDetails.Capabilities, windowWidth, windowHeight);
    
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
    
    uint32 queueFamilyIndices[] = { vkData->Indices.GraphicsFamily, vkData->Indices.PresentFamily };
    
    if(vkData->Indices.GraphicsFamily != vkData->Indices.PresentFamily)
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
    
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
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
    
    VkVertexInputBindingDescription bindingDesc = GetVertexBindingDescription();
    const uint32 attrDescsSize = 2;
    VkVertexInputAttributeDescription attributeDescs[attrDescsSize] = {};
    GetVertexAttributeDescriptions(attributeDescs);
    
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = attrDescsSize;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;
    
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
        
        vkData->FramebufferCount = i;
    }
}

static void CreateCommandPool(VulkanData* vkData)
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vkData->Indices.GraphicsFamily;
    
    VkResult result = vkCreateCommandPool(vkData->Device, &poolInfo, nullptr, &vkData->CommandPool);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create command pool!");
}

static uint32 FindMemoryType(const VkPhysicalDevice* physDevice, uint32 typeFilter, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(*physDevice, &memProperties);
    
    for(uint32 i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
            return i;
    }
    
    OutputDebugStringA("Failed to find suitable memory type!");
    return 0;
}

static void CopyBuffer(const VulkanData* vkData, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vkData->CommandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vkData->Device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(vkData->GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkData->GraphicsQueue);
    
    vkFreeCommandBuffers(vkData->Device, vkData->CommandPool, 1, &commandBuffer);
}

static void CreateBuffer(const VulkanData* vkData, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult bufferResult = vkCreateBuffer(vkData->Device, &bufferInfo, nullptr, buffer);
    if(bufferResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create vertex buffer!");
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkData->Device, *buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(&vkData->PhysicalDevice, memRequirements.memoryTypeBits, propertyFlags);
    
    VkResult allocResult = vkAllocateMemory(vkData->Device, &allocInfo, nullptr, bufferMemory);
    if(allocResult != VK_SUCCESS)
        OutputDebugStringA("Failed to allocate vertex buffer memory!");
    
    vkBindBufferMemory(vkData->Device, *buffer, *bufferMemory, 0);
}

static void CreateVertexbuffer(VulkanData* vkData)
{
    VkDeviceSize bufferSize = sizeof(gVertices);
    
    uint32 bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbufferMemory;
    CreateBuffer(vkData, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &stagingbuffer, &stagingbufferMemory);
    
    void* data;
    vkMapMemory(vkData->Device, stagingbufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, gVertices, (uint64)bufferSize);
    vkUnmapMemory(vkData->Device, stagingbufferMemory);
    
    uint32 usageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    CreateBuffer(vkData, bufferSize, usageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vkData->Vertexbuffer, &vkData->VertexbufferMemory);
    
    CopyBuffer(vkData, stagingbuffer, vkData->Vertexbuffer, bufferSize);
    
    vkDestroyBuffer(vkData->Device, stagingbuffer, nullptr);
    vkFreeMemory(vkData->Device, stagingbufferMemory, nullptr);
}

static void CreateIndexbuffer(VulkanData* vkData)
{
    VkDeviceSize bufferSize = sizeof(gIndices);
    
    uint32 bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbufferMemory;
    CreateBuffer(vkData, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &stagingbuffer, &stagingbufferMemory);
    
    void* data;
    vkMapMemory(vkData->Device, stagingbufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, gIndices, (uint64)bufferSize);
    vkUnmapMemory(vkData->Device, stagingbufferMemory);
    
    uint32 usageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    CreateBuffer(vkData, bufferSize, usageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vkData->Indexbuffer, &vkData->IndexbufferMemory);
    
    CopyBuffer(vkData, stagingbuffer, vkData->Indexbuffer, bufferSize);
    
    vkDestroyBuffer(vkData->Device, stagingbuffer, nullptr);
    vkFreeMemory(vkData->Device, stagingbufferMemory, nullptr);
}

static void CreateCommandBuffers(VulkanData* vkData)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkData->CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = vkData->FramebufferCount;
    
    VkResult allocateResult = vkAllocateCommandBuffers(vkData->Device, &allocInfo, vkData->Commandbuffers);
    if(allocateResult != VK_SUCCESS)
        OutputDebugStringA("Failed to allocate command buffers!");
    
    for(uint32 i = 0; i < vkData->FramebufferCount; i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        
        VkResult beginResult = vkBeginCommandBuffer(vkData->Commandbuffers[i], &beginInfo);
        if(beginResult != VK_SUCCESS)
            OutputDebugStringA("Failed to begin recording command buffer!");
        
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vkData->RenderPass;
        renderPassInfo.framebuffer = vkData->Framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = vkData->SwapChainExtent;
        
        VkClearValue clearColor = { 0.0f, 0.1f, 0.2f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(vkData->Commandbuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(vkData->Commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkData->GraphicsPipeline);
        
        VkBuffer vertexBuffers[] = { vkData->Vertexbuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vkData->Commandbuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vkData->Commandbuffers[i], vkData->Indexbuffer, 0, VK_INDEX_TYPE_UINT16);
        
        vkCmdDrawIndexed(vkData->Commandbuffers[i], ArrayCount(gIndices), 1, 0, 0, 0);
        vkCmdEndRenderPass(vkData->Commandbuffers[i]);
        
        VkResult endCmdResult = vkEndCommandBuffer(vkData->Commandbuffers[i]);
        if(endCmdResult != VK_SUCCESS)
            OutputDebugStringA("Failed to end command buffer recording!");
    }
}

static void CreateSyncObjects(VulkanData* vkData)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for(uint32 i = 0; i < MP_VK_FRAMES_IN_FLIGHT_MAX; i++)
    {
        VkResult semaphoreResult1 = vkCreateSemaphore(vkData->Device, &semaphoreInfo, nullptr, &vkData->ImageAvailableSemaphores[i]);
        VkResult semaphoreResult2 = vkCreateSemaphore(vkData->Device, &semaphoreInfo, nullptr, &vkData->RenderFinishedSemaphores[i]);
        VkResult frenceResult = vkCreateFence(vkData->Device, &fenceInfo, nullptr, &vkData->InFlightFences[i]);
        if(semaphoreResult1 != VK_SUCCESS || semaphoreResult2 != VK_SUCCESS)
                OutputDebugStringA("Failed to create synchronization objects for a frame!");
    }
}

static void CleanUpSwapChain(VulkanData* vkData)
{
    for(int i = 0; i < MP_VK_SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        // TODO: skip emtpy elements perhaps
        vkDestroyFramebuffer(vkData->Device, vkData->Framebuffers[i], nullptr);
    }
    
    vkFreeCommandBuffers(vkData->Device, vkData->CommandPool, vkData->FramebufferCount, vkData->Commandbuffers);
    vkDestroyPipeline(vkData->Device, vkData->GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkData->Device, vkData->PipelineLayout, nullptr);
    vkDestroyRenderPass(vkData->Device, vkData->RenderPass, nullptr);
    
    for(int i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++)
    {
        vkDestroyImageView(vkData->Device, vkData->SwapChainImageViews[i], nullptr);
    }
    
    vkDestroySwapchainKHR(vkData->Device, vkData->SwapChain, nullptr);
}

static void RecreateSwapChain(VulkanData* vkData, int windowWidth, int windowHeight)
{
    vkDeviceWaitIdle(vkData->Device);
    
    CleanUpSwapChain(vkData);
    
    CreateSwapChain(vkData, windowWidth, windowHeight);
    CreateImageViews(vkData);
    CreateRenderPass(vkData);
    CreateGraphicsPipeline(vkData);
    CreateFramebuffers(vkData);
    CreateCommandBuffers(vkData);
}

VulkanData* VulkanInit(MP_MEMORY* gameMemory, Win32WindowInfo* windowInfo, debug_read_file_result* vertShader, debug_read_file_result* fragShader)
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
    CreateWindowSurface(vkData, windowInfo->pHInstance, windowInfo->pWindow);
    PickPhysicalDevice(vkData);
    CreateLogicalDevice(vkData);
    CreateSwapChain(vkData, windowInfo->Width, windowInfo->Height);
    CreateImageViews(vkData);
    CreateRenderPass(vkData);
    CreateGraphicsPipeline(vkData);
    CreateFramebuffers(vkData);
    CreateCommandPool(vkData);
    CreateVertexbuffer(vkData);
    CreateIndexbuffer(vkData);
    CreateCommandBuffers(vkData);
    CreateSyncObjects(vkData);
    
    return vkData;
}

static void DrawFrame(VulkanData* vkData, int windowWidth, int windowHeight)
{
    vkWaitForFences(vkData->Device, 1, &vkData->InFlightFences[vkData->currentFrame], VK_TRUE, UINT64MAX);
    
    uint32 imageIndex = 0;
    VkResult imageResult = vkAcquireNextImageKHR(vkData->Device, vkData->SwapChain, UINT64MAX, vkData->ImageAvailableSemaphores[vkData->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(imageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain(vkData, windowWidth, windowHeight);
        return;
    }
    else if(imageResult != VK_SUCCESS && imageResult != VK_SUBOPTIMAL_KHR)
    {
        OutputDebugStringA("Failed to acquire swap chain image!");
    }
    
    if(vkData->InFlightImages[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(vkData->Device, 1, &vkData->InFlightImages[imageIndex], VK_TRUE, UINT64MAX);
    }
    vkData->InFlightImages[imageIndex] = vkData->InFlightFences[vkData->currentFrame];
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = { vkData->ImageAvailableSemaphores[vkData->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkData->Commandbuffers[imageIndex];
    
    VkSemaphore signalSemaphores[] = { vkData->RenderFinishedSemaphores[vkData->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkResetFences(vkData->Device, 1, &vkData->InFlightFences[vkData->currentFrame]);
    
    VkResult submitResult = vkQueueSubmit(vkData->GraphicsQueue, 1, &submitInfo, vkData->InFlightFences[vkData->currentFrame]);
    if(submitResult != VK_SUCCESS)
        OutputDebugStringA("Failed to submit draw command buffer!");
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = { vkData->SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    VkResult presentResult = vkQueuePresentKHR(vkData->PresentQueue, &presentInfo);
    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || vkData->FramebufferResized)
    {
        *(vkData->FramebufferResized) = false;
        RecreateSwapChain(vkData, windowWidth, windowHeight);
    }
    else if(presentResult != VK_SUCCESS)
    {
        OutputDebugStringA("Failed to present swap chain image!");
    }
    
    vkData->currentFrame = (vkData->currentFrame + 1) % MP_VK_FRAMES_IN_FLIGHT_MAX;
}

void VulkanUpdate(VulkanData* vkData, int windowWidth, int windowHeight)
{
    if(windowWidth == 0 || windowHeight == 0)
    {
        return;
    }
    
    DrawFrame(vkData, windowWidth, windowHeight);
}

void VulkanCleanup(VulkanData* vkData)
{
    CleanUpSwapChain(vkData);
    
    vkDestroyBuffer(vkData->Device, vkData->Indexbuffer, nullptr);
    vkFreeMemory(vkData->Device, vkData->IndexbufferMemory, nullptr);
    vkDestroyBuffer(vkData->Device, vkData->Vertexbuffer, nullptr);
    vkFreeMemory(vkData->Device, vkData->VertexbufferMemory, nullptr);
    
    for(uint32 i = 0; i < MP_VK_FRAMES_IN_FLIGHT_MAX; i++)
    {
        vkDestroySemaphore(vkData->Device, vkData->ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(vkData->Device, vkData->RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(vkData->Device, vkData->InFlightFences[i], nullptr);
    }
    
    vkDestroyCommandPool(vkData->Device, vkData->CommandPool, nullptr);
    
    vkDestroyDevice(vkData->Device, nullptr);
    vkDestroySurfaceKHR(vkData->Instance, vkData->Surface, nullptr);
    vkDestroyInstance(vkData->Instance, nullptr);
    OutputDebugStringA("Vulkan cleaned up\n");
}