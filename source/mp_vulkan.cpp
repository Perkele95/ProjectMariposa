#include "mp_vulkan.h"
#include "..\Vulkan\Include\vulkan\vulkan_win32.h"

#define STB_IMAGE_IMPLEMENTATION
#include "..\stb_image\stb_image.h"

static void CreateInstance(VulkanData* renderer, MP_MEMORY* memory)
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
    
    VkExtensionProperties* extensionProperties = (VkExtensionProperties*) PushBackTransientStorage(memory, sizeof(VkExtensionProperties) * extensionsCount);
    
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionProperties);

    char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
    
    createInfo.enabledExtensionCount = ArrayCount(extensions);
    createInfo.ppEnabledExtensionNames = extensions;

    OutputDebugStringA("Vulkan extensions:\n");
    for(uint32 i = 0; i < extensionsCount; i++)
    {
        OutputDebugStringA("    ");
        OutputDebugStringA(extensionProperties[i].extensionName);
        OutputDebugStringA("\n");
    }

    VkResult result = vkCreateInstance(&createInfo, 0, &renderer->Instance);
    MP_ASSERT(result == VK_SUCCESS);
    OutputDebugStringA("Vulkan instance created\n");
}

static bool CheckValidationLayerSupport(VulkanData* renderer, MP_MEMORY* memory)
{
    uint32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    VkLayerProperties* availableLayers = (VkLayerProperties*) PushBackTransientStorage(memory, sizeof(VkLayerProperties) * layerCount);
    
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

static QueueFamilyIndices FindQueueFamilyIndices(VulkanData* renderer, VkPhysicalDevice* checkedPhysDevice, MP_MEMORY* memory) {
    QueueFamilyIndices indices = {};
    uint32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties* queueFamilyProperties = (VkQueueFamilyProperties*) PushBackTransientStorage(memory, sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamilyCount, queueFamilyProperties);

    for(uint32 i = 0; i < queueFamilyCount; i++)
    {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.GraphicsFamily = i;
            if(!indices.HasGraphicsFamily)
                indices.HasGraphicsFamily = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(*checkedPhysDevice, i, renderer->Surface, &presentSupport);
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

static bool CheckDeviceExtensionSupport(VkPhysicalDevice physDevice, MP_MEMORY* memory)
{
    uint32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);
    VkExtensionProperties* availableExtensions = (VkExtensionProperties*) PushBackTransientStorage(memory, sizeof(VkExtensionProperties) * extensionCount);
    
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

static bool IsDeviceSuitable(VulkanData* renderer, VkPhysicalDevice* checkedPhysDevice, MP_MEMORY* memory)
{
    QueueFamilyIndices indices = FindQueueFamilyIndices(renderer, checkedPhysDevice, memory);

    bool extensionsSupported = CheckDeviceExtensionSupport(*checkedPhysDevice, memory);
    bool swapChainAdequate = false;
    if(extensionsSupported)
    {
        SwapChainSupportDetails swapChainDetails = QuerySwapChainSupport(*checkedPhysDevice, renderer->Surface);
        swapChainAdequate = swapChainDetails.IsAdequate;
    }

    renderer->Indices = indices;

    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(*checkedPhysDevice, &supportedFeatures);

    return indices.IsComplete && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

static void PickPhysicalDevice(VulkanData* renderer, MP_MEMORY* memory)
{
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(renderer->Instance, &deviceCount, nullptr);
    if(deviceCount == 0)    // TODO: replace with runtime error
        OutputDebugStringA("Failed to find GPUs with Vulkan Support");
    
    VkPhysicalDevice* devices = (VkPhysicalDevice*) PushBackTransientStorage(memory, sizeof(VkPhysicalDevice) * deviceCount);
    
    vkEnumeratePhysicalDevices(renderer->Instance, &deviceCount, devices);

    for(uint32 i = 0; i < deviceCount; i++)
    {
        if(IsDeviceSuitable(renderer, &devices[i], memory))
        {
            renderer->Gpu = devices[i];
            break;
        }
    }

    if(renderer->Gpu == VK_NULL_HANDLE)
        OutputDebugStringA("Failed to find a suitable GPU!");
}

static void CreateLogicalDevice(VulkanData* renderer)
{
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
    
    uint32 uniqueQueueFamilies[2] = {};
    uint32 queueCount = 0;
    if(renderer->Indices.GraphicsFamily == renderer->Indices.PresentFamily)
    {
        uniqueQueueFamilies[0] = renderer->Indices.GraphicsFamily;
        queueCount = 1;
    }
    else
    {
        uniqueQueueFamilies[0] = renderer->Indices.GraphicsFamily;
        uniqueQueueFamilies[1] = renderer->Indices.PresentFamily;
        queueCount = 2;
    }
    
    float queuePriority = 1.0f;
    for(uint32 i = 0; i < queueCount; i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCount;
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

    VkResult result = vkCreateDevice(renderer->Gpu, &createInfo, nullptr, &renderer->Device);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create logical device!");

    vkGetDeviceQueue(renderer->Device, renderer->Indices.GraphicsFamily, 0, &renderer->GraphicsQueue);
    vkGetDeviceQueue(renderer->Device, renderer->Indices.PresentFamily, 0, &renderer->PresentQueue);
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

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(VkSurfaceFormatKHR availableFormats[])
{
    for(int i = 0; i < ArrayCount(availableFormats); i++)
    {
        if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormats[i];
    }

    return availableFormats[0];
}

static void CreateSwapChain(VulkanData* renderer, int windowWidth, int windowHeight)
{
    SwapChainSupportDetails swapChainDetails = QuerySwapChainSupport(renderer->Gpu, renderer->Surface);

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
    createInfo.surface = renderer->Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32 queueFamilyIndices[] = { renderer->Indices.GraphicsFamily, renderer->Indices.PresentFamily };

    if(renderer->Indices.GraphicsFamily != renderer->Indices.PresentFamily)
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

    VkResult result = vkCreateSwapchainKHR(renderer->Device, &createInfo, nullptr, &renderer->SwapChain);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create swap chain!");

    MP_ASSERT(imageCount <= ArrayCount(renderer->SwapChainImages));
    vkGetSwapchainImagesKHR(renderer->Device, renderer->SwapChain, &imageCount, nullptr);
    vkGetSwapchainImagesKHR(renderer->Device, renderer->SwapChain, &imageCount, renderer->SwapChainImages);

    renderer->SwapChainImageFormat = surfaceFormat.format;
    renderer->SwapChainExtent = extent;
}

static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if(vkCreateImageView(device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
        OutputDebugStringA("Failed to create image view!");

    return imageView;
}

static void CreateImageViews(VulkanData* renderer)
{
    for(uint32 i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++) // TODO: count
    {
        if(renderer->SwapChainImages[i] == 0)
            break;

        renderer->SwapChainImageViews[i] = CreateImageView(renderer->Device, renderer->SwapChainImages[i], renderer->SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        
        renderer->SwapChainImageCount = i;
    }
}

static void CreateWindowSurface(VulkanData* renderer, HINSTANCE* hInstance, HWND* window)
{
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = *window;
    createInfo.hinstance = *hInstance;

    VkResult result = vkCreateWin32SurfaceKHR(renderer->Instance, &createInfo, nullptr, &renderer->Surface);
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

static VkFormat FindSupportedFormat(VkPhysicalDevice physDevice, const VkFormat formats[], VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for(int i = 0; i < ArrayCount(formats); i++)
    {
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(physDevice, formats[i], &props);
        
        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return formats[i];
        else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return formats[i];
    }
    
    MP_ASSERT(false);
    OutputDebugStringA("Failed to find supported format!\n");
    return VK_FORMAT_UNDEFINED;
}

static VkFormat FindDepthFormat(VkPhysicalDevice physDevice)
{
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return FindSupportedFormat(physDevice, formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

static void CreateRenderPass(VulkanData* renderer)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = renderer->SwapChainImageFormat;
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
    
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = FindDepthFormat(renderer->Gpu);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = ArrayCount(attachments);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(renderer->Device, &renderPassInfo, nullptr, &renderer->RenderPass);
    if(result != VK_SUCCESS)
        OutputDebugStringA("Failed to create render pass!");
}

static void CreateDescriptorSetLayout(VulkanData* renderer)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = ArrayCount(bindings);
    layoutInfo.pBindings = bindings;

    VkResult layoutResult = vkCreateDescriptorSetLayout(renderer->Device, &layoutInfo, nullptr, &renderer->DescriptorSetLayout);
    if(layoutResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create descriptor set layout!");
}

static void CreateGraphicsPipeline(VulkanData* renderer)
{
    VkShaderModule vertexShaderModule = CreateShaderModule(&renderer->Device, &renderer->VertexShader);
    VkShaderModule fragmentShaderModule = CreateShaderModule(&renderer->Device, &renderer->FragmentShader);

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

    const uint32 attrDescsSize = 3;
    VkVertexInputBindingDescription bindingDesc = GetVertexBindingDescription();
    VkVertexInputAttributeDescription attributeDescs[attrDescsSize] = {};
    GetVertexAttributeDescriptions(attributeDescs, attrDescsSize);

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
    viewport.width = (float)renderer->SwapChainExtent.width;
    viewport.height = (float)renderer->SwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = renderer->SwapChainExtent;

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
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

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
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &renderer->DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VkResult layoutResult = vkCreatePipelineLayout(renderer->Device, &pipelineLayoutInfo, nullptr, &renderer->PipelineLayout);
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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = renderer->PipelineLayout;
    pipelineInfo.renderPass = renderer->RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult gPipelineResult = vkCreateGraphicsPipelines(renderer->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &renderer->GraphicsPipeline);
    if(gPipelineResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create graphics pipeline!");

    vkDestroyShaderModule(renderer->Device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(renderer->Device, fragmentShaderModule, nullptr);
}

static void CreateFramebuffers(VulkanData* renderer)
{
    for(uint32 i = 0; i < renderer->SwapChainImageCount; i++)
    {
        VkImageView attachments[] = { renderer->SwapChainImageViews[i] , renderer->DepthImageView };
        
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderer->RenderPass;
        framebufferInfo.attachmentCount = ArrayCount(attachments);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = renderer->SwapChainExtent.width;
        framebufferInfo.height = renderer->SwapChainExtent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(renderer->Device, &framebufferInfo, nullptr, &renderer->Framebuffers[i]);
        if(result != VK_SUCCESS)
            OutputDebugStringA("Failed to create framebuffer!");
    }
}

static void CreateCommandPool(VulkanData* renderer)
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = renderer->Indices.GraphicsFamily;

    VkResult result = vkCreateCommandPool(renderer->Device, &poolInfo, nullptr, &renderer->CommandPool);
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

static void CreateImage(VulkanData* renderer, uint32 width, uint32 height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, VkMemoryPropertyFlags propFlags, VkImage* image, VkDeviceMemory* imageMemory)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult imageResult = vkCreateImage(renderer->Device, &imageInfo, nullptr, image);
    if(imageResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(renderer->Device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(&renderer->Gpu, memRequirements.memoryTypeBits, propFlags);

    VkResult allocResult = vkAllocateMemory(renderer->Device, &allocInfo, nullptr, imageMemory);
    if(allocResult != VK_SUCCESS)
        OutputDebugStringA("Failed to allocate image memory!");

    vkBindImageMemory(renderer->Device, *image, *imageMemory, 0);
}

static VkCommandBuffer BeginSingleTimeCommands(const VulkanData* renderer)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = renderer->CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(renderer->Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

static void EndSingleTimeCommands(const VulkanData* renderer, VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(renderer->GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(renderer->GraphicsQueue);

    vkFreeCommandBuffers(renderer->Device, renderer->CommandPool, 1, &commandBuffer);
}

static void TransitionImageLayout(VulkanData* renderer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(renderer);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = renderer->TextureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = {};
    VkPipelineStageFlags destStage = {};

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        OutputDebugStringA("Unsupported layout transition!\n");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommands(renderer, commandBuffer);
}

static void CreateDepthResources(VulkanData* renderer)
{
    VkFormat depthFormat = FindDepthFormat(renderer->Gpu);
    
    CreateImage(renderer, renderer->SwapChainExtent.width, renderer->SwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->DepthImage, &renderer->DepthImageMemory);
    renderer->DepthImageView = CreateImageView(renderer->Device, renderer->DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

static void CreateBuffer(const VulkanData* renderer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult bufferResult = vkCreateBuffer(renderer->Device, &bufferInfo, nullptr, buffer);
    if(bufferResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create vertex buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(renderer->Device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(&renderer->Gpu, memRequirements.memoryTypeBits, propertyFlags);

    VkResult allocResult = vkAllocateMemory(renderer->Device, &allocInfo, nullptr, bufferMemory);
    if(allocResult != VK_SUCCESS)
        OutputDebugStringA("Failed to allocate vertex buffer memory!");

    vkBindBufferMemory(renderer->Device, *buffer, *bufferMemory, 0);
}

static void CopyBufferToImage(const VulkanData* renderer, VkBuffer buffer, uint32 width, uint32 height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(renderer);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, renderer->TextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(renderer, commandBuffer);
}

static void CreateTextureImage(VulkanData* renderer)
{
    int texWidth, texHeight;
    uint8* pixels = stbi_load("../assets/dirt.png", &texWidth, &texHeight, nullptr, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;  // 4 = bytes per pixel

    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbufferMemory;

    VkMemoryPropertyFlags propFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    CreateBuffer(renderer, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propFlags, &stagingbuffer, &stagingbufferMemory);

    void* data;
    vkMapMemory(renderer->Device, stagingbufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, (uint64)imageSize);
    vkUnmapMemory(renderer->Device, stagingbufferMemory);

    stbi_image_free(pixels);

    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    CreateImage(renderer, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->TextureImage, &renderer->TextureImageMemory);

    TransitionImageLayout(renderer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(renderer, stagingbuffer, (uint32)texWidth, (uint32)texHeight);
    TransitionImageLayout(renderer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(renderer->Device, stagingbuffer, nullptr);
    vkFreeMemory(renderer->Device, stagingbufferMemory, nullptr);
}

static void CreateTextureImageView(VulkanData* renderer)
{
    renderer->TextureImageView = CreateImageView(renderer->Device, renderer->TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

static void CreateTextureSampler(VulkanData* renderer)
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkResult samplerResult = vkCreateSampler(renderer->Device, &samplerInfo, nullptr, &renderer->TextureSampler);
    if(samplerResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create texture sampler!");
}

static void CopyBuffer(const VulkanData* renderer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(renderer);

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommands(renderer, commandBuffer);
}

static void CreateGeometryBuffers(VulkanData* renderer, MP_RENDERDATA* renderData)
{    
    VkDeviceSize vertbufferSize = sizeof(renderData->Cubes[0].Vertices);
    VkDeviceSize indexbufferSize = sizeof(renderData->Cubes[0].Indices);
    uint32 bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer vertStagingbuffer, indexStagingbuffer;
    VkDeviceMemory vertStagingbufferMemory, indexStagingbufferMemory;
    void* vertData;
    void* indexData;
    VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    
    CreateBuffer(renderer, vertbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &vertStagingbuffer, &vertStagingbufferMemory);
    CreateBuffer(renderer, indexbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &indexStagingbuffer, &indexStagingbufferMemory);
    vkMapMemory(renderer->Device, vertStagingbufferMemory, 0, vertbufferSize, 0, &vertData);
    vkMapMemory(renderer->Device, indexStagingbufferMemory, 0, indexbufferSize, 0, &indexData);
    memcpy(vertData, renderData->Cubes[0].Vertices, (uint64)vertbufferSize);
    memcpy(indexData, renderData->Cubes[0].Indices, (uint64)indexbufferSize);
    vkUnmapMemory(renderer->Device, vertStagingbufferMemory);
    vkUnmapMemory(renderer->Device, indexStagingbufferMemory);
    
    CreateBuffer(renderer, vertbufferSize, vertUsageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->Vertexbuffer, &renderer->VertexbufferMemory);
    CreateBuffer(renderer, indexbufferSize, indexUsageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->Indexbuffer, &renderer->IndexbufferMemory);
    CopyBuffer(renderer, vertStagingbuffer, renderer->Vertexbuffer, vertbufferSize);
    CopyBuffer(renderer, indexStagingbuffer, renderer->Indexbuffer, indexbufferSize);
    
    vkDestroyBuffer(renderer->Device, vertStagingbuffer, nullptr);
    vkDestroyBuffer(renderer->Device, indexStagingbuffer, nullptr);
    vkFreeMemory(renderer->Device, vertStagingbufferMemory, nullptr);
    vkFreeMemory(renderer->Device, indexStagingbufferMemory, nullptr);
}

static void CreateUniformbuffers(VulkanData* renderer)
{
    VkDeviceSize bufferSize = sizeof(UniformbufferObject);
    uint32 bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for(uint32 i = 0; i < renderer->SwapChainImageCount; i++)
    {
        CreateBuffer(renderer, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSrcFlags, &renderer->Uniformbuffers[i], &renderer->UniformbuffersMemory[i]);
    }
}

static void CreateDescriptorPool(VulkanData* renderer)
{
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = renderer->SwapChainImageCount;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = renderer->SwapChainImageCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = ArrayCount(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = renderer->SwapChainImageCount;

    VkResult poolResult = vkCreateDescriptorPool(renderer->Device, &poolInfo, nullptr, &renderer->DescriptorPool);
    if(poolResult != VK_SUCCESS)
        OutputDebugStringA("Failed to create descriptor pool!");
}

static void CreateDescriptorSets(VulkanData* renderer)
{
    VkDescriptorSetLayout layouts[MP_VK_SWAP_IMAGE_MAX];
    for(uint32 i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++)
        layouts[i] = renderer->DescriptorSetLayout;

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = renderer->DescriptorPool;
    allocInfo.descriptorSetCount = renderer->SwapChainImageCount;
    allocInfo.pSetLayouts = layouts;

    VkResult allocResult = vkAllocateDescriptorSets(renderer->Device, &allocInfo, renderer->DescriptorSets);
    if(allocResult != VK_SUCCESS)
        OutputDebugStringA("Failed to allocate descriptor sets!");

    for(uint32 i = 0; i < renderer->SwapChainImageCount; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = renderer->Uniformbuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformbufferObject);
        
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = renderer->TextureImageView;
        imageInfo.sampler = renderer->TextureSampler;

        VkWriteDescriptorSet descriptorWrites[2] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = renderer->DescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = renderer->DescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(renderer->Device, ArrayCount(descriptorWrites), descriptorWrites, 0, nullptr);
    }
}

static void CreateCommandBuffers(VulkanData* renderer, MP_RENDERDATA* renderData)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = renderer->CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = renderer->SwapChainImageCount;

    VkResult allocateResult = vkAllocateCommandBuffers(renderer->Device, &allocInfo, renderer->Commandbuffers);
    if(allocateResult != VK_SUCCESS)
        OutputDebugStringA("Failed to allocate command buffers!");

    for(uint32 i = 0; i < renderer->SwapChainImageCount; i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkResult beginResult = vkBeginCommandBuffer(renderer->Commandbuffers[i], &beginInfo);
        if(beginResult != VK_SUCCESS)
            OutputDebugStringA("Failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderer->RenderPass;
        renderPassInfo.framebuffer = renderer->Framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = renderer->SwapChainExtent;

        VkClearValue clearValues[2];
        clearValues[0].color  = { 0.6f, 0.9f, 1.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = ArrayCount(clearValues);
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(renderer->Commandbuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(renderer->Commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->GraphicsPipeline);

        VkBuffer vertexBuffers[] = { renderer->Vertexbuffer };
        VkDeviceSize offsets[] = { 0 };
        
        // TODO: Do this per unique 3D object!
        vkCmdBindVertexBuffers(renderer->Commandbuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(renderer->Commandbuffers[i], renderer->Indexbuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(renderer->Commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->PipelineLayout, 0, 1, &renderer->DescriptorSets[i], 0, nullptr);
        vkCmdDrawIndexed(renderer->Commandbuffers[i], ArrayCount(renderData->Cubes[0].Indices), 1, 0, 0, 0);
        // ----------------------------------
        
        vkCmdEndRenderPass(renderer->Commandbuffers[i]);

        VkResult endCmdResult = vkEndCommandBuffer(renderer->Commandbuffers[i]);
        if(endCmdResult != VK_SUCCESS)
            OutputDebugStringA("Failed to end command buffer recording!");
    }
}

static void CreateSyncObjects(VulkanData* renderer)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32 i = 0; i < MP_VK_FRAMES_IN_FLIGHT_MAX; i++)
    {
        VkResult semaphoreResult1 = vkCreateSemaphore(renderer->Device, &semaphoreInfo, nullptr, &renderer->ImageAvailableSemaphores[i]);
        VkResult semaphoreResult2 = vkCreateSemaphore(renderer->Device, &semaphoreInfo, nullptr, &renderer->RenderFinishedSemaphores[i]);
        VkResult frenceResult = vkCreateFence(renderer->Device, &fenceInfo, nullptr, &renderer->InFlightFences[i]);
        if(semaphoreResult1 != VK_SUCCESS || semaphoreResult2 != VK_SUCCESS)
                OutputDebugStringA("Failed to create synchronization objects for a frame!");
    }
}

static void CleanUpSwapChain(VulkanData* renderer)
{
    vkDestroyImageView(renderer->Device, renderer->DepthImageView, nullptr);
    vkDestroyImage(renderer->Device, renderer->DepthImage, nullptr);
    vkFreeMemory(renderer->Device, renderer->DepthImageMemory, nullptr);
    
    for(int i = 0; i < MP_VK_SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        // TODO: skip emtpy elements perhaps
        vkDestroyFramebuffer(renderer->Device, renderer->Framebuffers[i], nullptr);
    }

    vkFreeCommandBuffers(renderer->Device, renderer->CommandPool, renderer->SwapChainImageCount, renderer->Commandbuffers);
    vkDestroyPipeline(renderer->Device, renderer->GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(renderer->Device, renderer->PipelineLayout, nullptr);
    vkDestroyRenderPass(renderer->Device, renderer->RenderPass, nullptr);

    for(int i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++)
    {
        vkDestroyImageView(renderer->Device, renderer->SwapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(renderer->Device, renderer->SwapChain, nullptr);

    for(uint32 i = 0; i < MP_VK_SWAP_IMAGE_MAX; i++)
    {
        vkDestroyBuffer(renderer->Device, renderer->Uniformbuffers[i], nullptr);
        vkFreeMemory(renderer->Device, renderer->UniformbuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(renderer->Device, renderer->DescriptorPool, nullptr);
}

static void RecreateSwapChain(VulkanData* renderer, int windowWidth, int windowHeight, MP_RENDERDATA* renderData)
{    
    vkDeviceWaitIdle(renderer->Device);

    CleanUpSwapChain(renderer);

    CreateSwapChain(renderer, windowWidth, windowHeight);
    CreateImageViews(renderer);
    CreateRenderPass(renderer);
    CreateGraphicsPipeline(renderer);
    CreateDepthResources(renderer);
    CreateFramebuffers(renderer);
    CreateUniformbuffers(renderer);
    CreateDescriptorPool(renderer);
    CreateDescriptorSets(renderer);
    CreateCommandBuffers(renderer, renderData);
}

VulkanData* VulkanInit(MP_MEMORY* memory, Win32WindowInfo* windowInfo, debug_read_file_result* vertShader, debug_read_file_result* fragShader, MP_RENDERDATA* renderData)
{
    VulkanData* renderer = (VulkanData*) PushBackPermanentStorage(memory, sizeof(VulkanData));
    
    OutputDebugStringA("Vulkan Data initialised\n");

    renderer->VertexShader = *vertShader;
    renderer->FragmentShader = *fragShader;

    if(enableValidationLayers && !CheckValidationLayerSupport(renderer, memory))
        OutputDebugStringA("Validation layers requested, but not available!\n");

    // IMPORTANT: Order matters
    CreateInstance(renderer, memory);
    CreateWindowSurface(renderer, windowInfo->pHInstance, windowInfo->pWindow);
    PickPhysicalDevice(renderer, memory);
    PurgeTransientStorage(memory);
    CreateLogicalDevice(renderer);
    CreateSwapChain(renderer, windowInfo->Width, windowInfo->Height);
    CreateImageViews(renderer);
    CreateRenderPass(renderer);
    CreateDescriptorSetLayout(renderer);
    CreateGraphicsPipeline(renderer);
    CreateCommandPool(renderer);
    CreateDepthResources(renderer);
    CreateFramebuffers(renderer);
    CreateTextureImage(renderer);
    CreateTextureImageView(renderer);
    CreateTextureSampler(renderer);
    CreateGeometryBuffers(renderer, renderData);
    CreateUniformbuffers(renderer);
    CreateDescriptorPool(renderer);
    CreateDescriptorSets(renderer);
    CreateCommandBuffers(renderer, renderData);
    CreateSyncObjects(renderer);

    return renderer;
}

static void UpdateUniformbuffer(uint32 currentImage, VulkanData* renderer, MP_RENDERDATA* renderData)
{
    UniformbufferObject ubo = {};

    ubo.Model = Mat4x4Identity();
    ubo.View = LookAt(renderData->CameraPosition, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}) * Mat4RotateX(renderData->CameraRotation.X) * Mat4RotateY(renderData->CameraRotation.Y);
    ubo.Proj = Perspective(PI32 / 4.0f, (float)renderer->SwapChainExtent.width / (float)renderer->SwapChainExtent.height, 0.1f, 10.0f);

    void* data;
    VkDeviceSize dataSize = sizeof(ubo);
    vkMapMemory(renderer->Device, renderer->UniformbuffersMemory[currentImage], 0, dataSize, 0, &data);
    memcpy(data, &ubo, dataSize);
    vkUnmapMemory(renderer->Device, renderer->UniformbuffersMemory[currentImage]);
}

void VulkanUpdate(VulkanData* renderer, int windowWidth, int windowHeight, MP_RENDERDATA* renderData)
{
    if(windowWidth == 0 || windowHeight == 0)
    {
        return;
    }

    vkWaitForFences(renderer->Device, 1, &renderer->InFlightFences[renderer->currentFrame], VK_TRUE, UINT64MAX);

    uint32 imageIndex = 0;
    VkResult imageResult = vkAcquireNextImageKHR(renderer->Device, renderer->SwapChain, UINT64MAX, renderer->ImageAvailableSemaphores[renderer->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(imageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain(renderer, windowWidth, windowHeight, renderData);
        return;
    }
    else if(imageResult != VK_SUCCESS && imageResult != VK_SUBOPTIMAL_KHR)
    {
        OutputDebugStringA("Failed to acquire swap chain image!");
    }

    if(renderer->InFlightImages[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(renderer->Device, 1, &renderer->InFlightImages[imageIndex], VK_TRUE, UINT64MAX);
    }
    renderer->InFlightImages[imageIndex] = renderer->InFlightFences[renderer->currentFrame];

    UpdateUniformbuffer(imageIndex, renderer, renderData);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { renderer->ImageAvailableSemaphores[renderer->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->Commandbuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { renderer->RenderFinishedSemaphores[renderer->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(renderer->Device, 1, &renderer->InFlightFences[renderer->currentFrame]);

    VkResult submitResult = vkQueueSubmit(renderer->GraphicsQueue, 1, &submitInfo, renderer->InFlightFences[renderer->currentFrame]);
    if(submitResult != VK_SUCCESS)
        OutputDebugStringA("Failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { renderer->SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(renderer->PresentQueue, &presentInfo);
    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || renderer->FramebufferResized)
    {
        *(renderer->FramebufferResized) = false;
        RecreateSwapChain(renderer, windowWidth, windowHeight, renderData);
    }
    else if(presentResult != VK_SUCCESS)
    {
        OutputDebugStringA("Failed to present swap chain image!");
    }

    renderer->currentFrame = (renderer->currentFrame + 1) % MP_VK_FRAMES_IN_FLIGHT_MAX;
}

void VulkanCleanup(VulkanData* renderer)
{
    CleanUpSwapChain(renderer);

    vkDestroySampler(renderer->Device, renderer->TextureSampler, nullptr);
    vkDestroyImageView(renderer->Device, renderer->TextureImageView, nullptr);
    vkDestroyImage(renderer->Device, renderer->TextureImage, nullptr);
    vkFreeMemory(renderer->Device, renderer->TextureImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(renderer->Device, renderer->DescriptorSetLayout, nullptr);

    vkDestroyBuffer(renderer->Device, renderer->Indexbuffer, nullptr);
    vkFreeMemory(renderer->Device, renderer->IndexbufferMemory, nullptr);
    vkDestroyBuffer(renderer->Device, renderer->Vertexbuffer, nullptr);
    vkFreeMemory(renderer->Device, renderer->VertexbufferMemory, nullptr);

    for(uint32 i = 0; i < MP_VK_FRAMES_IN_FLIGHT_MAX; i++)
    {
        vkDestroySemaphore(renderer->Device, renderer->ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(renderer->Device, renderer->RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(renderer->Device, renderer->InFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(renderer->Device, renderer->CommandPool, nullptr);

    vkDestroyDevice(renderer->Device, nullptr);
    vkDestroySurfaceKHR(renderer->Instance, renderer->Surface, nullptr);
    vkDestroyInstance(renderer->Instance, nullptr);
    OutputDebugStringA("Vulkan cleaned up\n");
}
