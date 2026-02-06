#include "VulkanSetUp.h"
#include <fstream>

#pragma region VULKAN DEBUG HELPER FUNCTIONS
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
    VkDebugUtilsMessageTypeFlagsEXT msgType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debug, pAllocator);
}
#pragma endregion

void VKSetUp::InitWindow(unsigned width, unsigned height)
{
    // Initialize GLFW
    glfwInit();

    // Initialize the window without using OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create the window
    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
}

std::vector<const char*> VKSetUp::getRequiredExtensions(const bool& enableLayer)
{
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableLayer)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool VKSetUp::checkValidationLayerSupport() const
{
    // Get the validation layers
    unsigned int layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Search for those valid layers, if it isn't in the list, then the validation layer 
    // is not supported in this device (I think it's the GPU in this case)
    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

bool VKSetUp::checkDeviceExtensionSupport(const VkPhysicalDevice& device_) const
{
    // Get all available extensions
    unsigned extensionCount;
    vkEnumerateDeviceExtensionProperties(device_, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtension(extensionCount);
    vkEnumerateDeviceExtensionProperties(device_, nullptr, &extensionCount, availableExtension.data());

    // If the extension VK_KHR_swapchain is there, then it means that it's capable of creating a swap chain
    std::set<std::string> requiredExtension(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtension)
        requiredExtension.erase(extension.extensionName);

    return requiredExtension.empty();
}

bool VKSetUp::isDeviceSuitable(const VkPhysicalDevice& device_) const
{
    // Checks for a dedicated graphics card that support geometry shaders
    /*VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;*/

    // For now, just use the first GPU that it encounters
    QueueFamilyIndices idx = findQueueFamily(device_);
    bool extensionSupport = checkDeviceExtensionSupport(device_);
    bool swapchain = false;
    if (extensionSupport)
    {
        SwapChainSupportDetails swapChainDetails = querySwapChainSupport(device_);
        swapchain = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
    }

    return idx.isComplete() && extensionSupport && swapchain;
}

void VKSetUp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info)
{
    info                    = {};
    info.sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback    = debugCallback;
}

void VKSetUp::setupDebugMessenger(const bool& enableLayer)
{
    if (!enableLayer)
        return;

    // Information about the debug messenger
    VkDebugUtilsMessengerCreateInfoEXT createDInfo{};
    populateDebugMessengerCreateInfo(createDInfo);

    // Create the debug messenger
    if (CreateDebugUtilsMessengerEXT(instance, &createDInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("failed to set up the debug messenger");
}

void VKSetUp::pickPhysicalDevice()
{
    // Get the number of GPUs available
    unsigned deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    // Search for a specific GPU based on the isDeviceSuitable function
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU!");
}

VkSurfaceFormatKHR VKSetUp::chooseSwapChainSurfaceFormat(const SwapChainSupportDetails& details)
{
    for (const auto& formats : details.formats)
    {
        if (formats.format == VK_FORMAT_B8G8R8A8_SRGB && formats.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return formats;
    }

    return details.formats.front();
}

VkPresentModeKHR VKSetUp::chooseSwapPresentMode(const SwapChainSupportDetails& details)
{
    for (const auto& present : details.presentModes)
    {
        if (present == VK_PRESENT_MODE_MAILBOX_KHR)
            return present;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VKSetUp::chooseSwapExtent(const SwapChainSupportDetails& details)
{
    if (details.capabilities.currentExtent.width != std::numeric_limits<unsigned>::max())
        return details.capabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = { static_cast<unsigned>(width), static_cast<unsigned>(height) };

    actualExtent.width  = std::clamp(actualExtent.width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);

    return actualExtent;
}

VkShaderModule VKSetUp::createShaderModule(const std::vector<char>& code) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shadModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shadModule) != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module!");

    return shadModule;
}

void VKSetUp::createLogicalDevice()
{
    QueueFamilyIndices idx = findQueueFamily(physicalDevice);

    // Information about the queues
    std::vector<VkDeviceQueueCreateInfo> createQInfos;
    std::set<unsigned> uniqueQFamilies = { idx.graphicsFamily.value(), idx.presentFamily.value() };
    float queuePriorirty = 1.f;
    for (unsigned qFamily : uniqueQFamilies)
    {
        VkDeviceQueueCreateInfo createQInfo{};
        createQInfo.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createQInfo.queueFamilyIndex    = qFamily;
        createQInfo.queueCount          = 1;
        createQInfo.pQueuePriorities    = &queuePriorirty;
        createQInfos.push_back(createQInfo);
    }

    // Enable dynamic rendering
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeatures.dynamicRendering   = VK_TRUE;

    // Information about the device/GPU
    VkDeviceCreateInfo createDevInfo{};
    createDevInfo.sType                     = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createDevInfo.pNext                     = &dynamicRenderingFeatures;
    createDevInfo.queueCreateInfoCount      = static_cast<unsigned>(createQInfos.size());
    createDevInfo.pQueueCreateInfos         = createQInfos.data();
    createDevInfo.pEnabledFeatures          = &deviceFeatures;
    createDevInfo.ppEnabledExtensionNames   = deviceExtensions.data();
    createDevInfo.enabledExtensionCount     = static_cast<unsigned>(deviceExtensions.size());

    // Create the logical device
    if (vkCreateDevice(physicalDevice, &createDevInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");

    // Get the handles for the graphics and presentation queues. The 0 is the queue index, if there is more than one, 
    // we need to pass the corresponding indices
    vkGetDeviceQueue(device, idx.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, idx.presentFamily.value(), 0, &presentQueue);
}

void VKSetUp::createSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface");
}

void VKSetUp::createInstance(const bool& enableLayer)
{
    // To check the Vulkan API version
    /*uint32_t version = 0;
    vkEnumerateInstanceVersion(&version);
    printf("Vulkan version: %d.%d.%d\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));*/

    // Check for validation layers in Debug mode
    if (enableLayer && !checkValidationLayerSupport())
        throw std::runtime_error("validation layer requested, but no available");

    // Information about the program
    VkApplicationInfo appInfo{};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "Hello Triangle";
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName         = "No Engine";
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion          = VK_API_VERSION_1_3;

    // Information about the instance (It's the one that connects the program with the Vulkan library)
    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get all required extensions for the instance
    auto extensions                     = getRequiredExtensions(enableLayer);
    createInfo.enabledExtensionCount    = static_cast<unsigned int>(extensions.size());
    createInfo.ppEnabledExtensionNames  = extensions.data();

    // Pass a debug messenger to the instance if validation layers are enable
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableLayer)
    {
        createInfo.enabledLayerCount    = static_cast<unsigned int>(validationLayers.size());
        createInfo.ppEnabledLayerNames  = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext             = nullptr;
    }

    // Create the instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create the instance");
}

void VKSetUp::createSwapChain()
{
    // Get the surface details to render onto the window
    SwapChainSupportDetails details = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapChainSurfaceFormat(details);
    VkPresentModeKHR presentMode     = chooseSwapPresentMode(details);
    VkExtent2D extent                = chooseSwapExtent(details);

    // Get the amount of images to have in the swap chain
    unsigned imgCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imgCount > details.capabilities.maxImageCount)
        imgCount = details.capabilities.maxImageCount;

    // Information about swap chain
    VkSwapchainCreateInfoKHR createSCIfno{};
    createSCIfno.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createSCIfno.surface          = surface;
    createSCIfno.minImageCount    = imgCount;
    createSCIfno.imageFormat      = surfaceFormat.format;
    createSCIfno.imageColorSpace  = surfaceFormat.colorSpace;
    createSCIfno.imageExtent      = extent;
    createSCIfno.imageArrayLayers = 1;
    createSCIfno.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices idx = findQueueFamily(physicalDevice);
    unsigned indices[] = { idx.graphicsFamily.value(), idx.presentFamily.value() };
    if (idx.graphicsFamily != idx.presentFamily)
    {
        createSCIfno.imageSharingMode       = VK_SHARING_MODE_CONCURRENT;
        createSCIfno.queueFamilyIndexCount  = 2;
        createSCIfno.pQueueFamilyIndices    = indices;
    }
    else
    {
        createSCIfno.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        createSCIfno.queueFamilyIndexCount  = 0;        // Optional
        createSCIfno.pQueueFamilyIndices    = nullptr;  // Optional
    }

    createSCIfno.preTransform   = details.capabilities.currentTransform;
    createSCIfno.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createSCIfno.presentMode    = presentMode;
    createSCIfno.clipped        = VK_TRUE;
    createSCIfno.oldSwapchain   = VK_NULL_HANDLE;

    // Create the swap chain
    if (vkCreateSwapchainKHR(device, &createSCIfno, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain!");

    // Get the handles for the images of the swap chain
    vkGetSwapchainImagesKHR(device, swapChain, &imgCount, nullptr);
    swapChainImages.resize(imgCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imgCount, swapChainImages.data());

    // Get the extent and format
    mExtent = extent;
    mFormat = surfaceFormat.format;
}

SwapChainSupportDetails VKSetUp::querySwapChainSupport(const VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    // Get the surface capability
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Get the surface format
    unsigned formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Get the surface presentation modes
    unsigned presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

QueueFamilyIndices VKSetUp::findQueueFamily(const VkPhysicalDevice& device) const
{
    QueueFamilyIndices idx;

    // Get the queue families and assign the queue family index
    unsigned queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    unsigned i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
        {
            idx.graphicsFamily = i;
            idx.presentFamily  = i;
        }

        if (idx.isComplete())
            break;

        i++;
    }

    return idx;
}

void VKSetUp::recordCommandBuffer(uint32_t imgIdx)
{
    // Start recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Before rendering, swap the swapchain to COLOR_ATTACHMENT_OPTIMAL
    transitionImgLayout(imgIdx,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        {},
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    VkClearValue clear{};
    clear.color = { 0.f, 0.f, 0.f, 0.f };

    VkRenderingAttachmentInfo attInfo{};
    attInfo.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    attInfo.imageView   = SCImageView[imgIdx];
    attInfo.clearValue  = clear;

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea = { .offset = {0, 0}, .extent = mExtent };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &attInfo;

    // Start rendering
    vkCmdBeginRendering(commandBuffer, &renderInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport vp{};
    vp.x = 0.f;
    vp.y = 0.f;
    vp.minDepth = 0.f;
    vp.maxDepth = 1.f;
    vp.width = static_cast<float>(mExtent.width);
    vp.height = static_cast<float>(mExtent.height);
    vkCmdSetViewport(commandBuffer, 0, 1, &vp);

    VkRect2D rect{};
    rect.offset = VkOffset2D(0, 0);
    rect.extent = mExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &rect);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish rendering
    vkCmdEndRendering(commandBuffer);
    transitionImgLayout(imgIdx,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        {},
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
    
    // Finish recording
    vkEndCommandBuffer(commandBuffer);
}

void VKSetUp::transitionImgLayout(uint32_t imgIdx,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags2 srcAM,
    VkAccessFlags2 dstAM,
    VkPipelineStageFlags2 srcSM,
    VkPipelineStageFlags2 dstSM)
{
    VkImageSubresourceRange subRange{};
    subRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subRange.baseMipLevel   = 0;
    subRange.levelCount     = 1;
    subRange.baseArrayLayer = 0;
    subRange.layerCount     = 1;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcAccessMask       = srcAM;
    barrier.srcStageMask        = srcSM;
    barrier.dstAccessMask       = dstAM;
    barrier.dstStageMask        = dstSM;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = swapChainImages[imgIdx];
    barrier.subresourceRange    = subRange;

    VkDependencyInfo depenInfo{};
    depenInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depenInfo.dependencyFlags         = {};
    depenInfo.imageMemoryBarrierCount = 1;
    depenInfo.pImageMemoryBarriers    = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &depenInfo);
}

void VKSetUp::destroyDebugMessenger() const
{
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}

void VKSetUp::createImageViews()
{
    SCImageView.resize(swapChainImages.size());

    int size = static_cast<int>(swapChainImages.size());
    for (int i = 0; i < size; i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image    = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // The type of texture that it will be storing the data (1D, 2D or 3D textures)
        createInfo.format   = mFormat;

        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;

        // The type of texture that it will output to the window (all colors, black & white, ...)
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT; 
        createInfo.subresourceRange.baseMipLevel    = 0;
        createInfo.subresourceRange.levelCount      = 1;
        createInfo.subresourceRange.baseArrayLayer  = 0;
        createInfo.subresourceRange.layerCount      = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &SCImageView.at(i)) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image views! (a.k.a textures)");
    }
}

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("failed to open file!");

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void VKSetUp::createGraphicsPipeline()
{
#pragma region SHADER
    // read SPIR-V shader code. To generate .spv files, go to 
    // "data/shaders/compile.bat" and double-click it.
    auto vertShad = readFile("../data/shaders/vert.spv");
    auto fragShad = readFile("../data/shaders/frag.spv");

    // create the modules for the vertex and fragment shaders
    vShadMod = createShaderModule(vertShad);
    fShadMod = createShaderModule(fragShad);

    // create shader stages to actually use the shaders
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage   = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module  = vShadMod;
    vertShaderStageInfo.pName   = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage   = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module  = fShadMod;
    fragShaderStageInfo.pName   = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
#pragma endregion

    std::vector dynamicStates   = { VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT, VkDynamicState::VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount  = static_cast<uint32_t>(dynamicStates.size());
    dynState.pDynamicStates     = dynamicStates.data();

    // Describes the format of the vtx data to pass into the vtx shader (a.k.a. VAO and VBO).
    // For now, do nothing...
    VkPipelineVertexInputStateCreateInfo vtxInputInfo{};
    vtxInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vpState{};
    vpState.sType           = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpState.viewportCount   = 1;
    vpState.scissorCount    = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable         = VK_FALSE;
    rasterizer.rasterizerDiscardEnable  = VK_FALSE;
    rasterizer.polygonMode              = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode                 = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace                = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable          = VK_FALSE;
    rasterizer.depthBiasSlopeFactor     = 1.f;
    rasterizer.lineWidth                = 1.f;

    // Multisaplimg
    VkPipelineMultisampleStateCreateInfo multi{};
    multi.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multi.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multi.sampleShadingEnable   = VK_FALSE;

    // Depth and stencil (do nothing for now...)
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    // Color blending
    VkPipelineColorBlendAttachmentState colBlendAtt{};
    colBlendAtt.blendEnable     = VK_FALSE;
    colBlendAtt.colorWriteMask  = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // EXAMPLE FOR BLENDING OPERATIONS:
    /*if (blendEnable)
    {
        finalColor.rgb = (srcColorBlendFactor * newColor.rgb) < colorBlendOp > (dstColorBlendFactor * oldColor.rgb);
        finalColor.a = (srcAlphaBlendFactor * newColor.a) < alphaBlendOp > (dstAlphaBlendFactor * oldColor.a);
    }
    else
        finalColor = newColor;

    finalColor = finalColor & colorWriteMask;

    // for the above pseudo-code to work, colBlendAtt needs the following:
    colBlendAtt.blendEnable = VK_TRUE;
    colBlendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
    colBlendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    colBlendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    colBlendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colBlendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colBlendAtt.alphaBlendOp = VK_BLEND_OP_ADD;*/

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.logicOpEnable    = VK_FALSE;
    colorBlend.logicOp          = VK_LOGIC_OP_COPY;
    colorBlend.attachmentCount  = 1;
    colorBlend.pAttachments     = &colBlendAtt;

    // Pipeline layout (a.k.a. uniforms for shaders)
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
        throw std::runtime_error("failed to create the pipeline layout");

    // Final sewt up for the graphics set up
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount      = 1;
    renderingInfo.pColorAttachmentFormats   = &mFormat;

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.pNext                  = &renderingInfo;
    pipeInfo.stageCount             = 2;
    pipeInfo.pStages                = shaderStages;
    pipeInfo.pVertexInputState      = &vtxInputInfo;
    pipeInfo.pInputAssemblyState    = &inputAssembly;
    pipeInfo.pViewportState         = &vpState;
    pipeInfo.pRasterizationState    = &rasterizer;
    pipeInfo.pMultisampleState      = &multi;
    pipeInfo.pColorBlendState       = &colorBlend;
    pipeInfo.pDynamicState          = &dynState;
    pipeInfo.layout                 = layout;
    pipeInfo.renderPass             = nullptr;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("could not create the graphics pipeline");
}

void VKSetUp::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags              = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex   = findQueueFamily(physicalDevice).graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Could not create command pool");
}

void VKSetUp::createCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Could not allocate the command buffer");
}

void VKSetUp::createSyncObjs()
{
    VkSemaphoreCreateInfo sCreateInfo{};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fCreateInfo{};
    fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateSemaphore(device, &sCreateInfo, nullptr, &presentComplete);
    vkCreateSemaphore(device, &sCreateInfo, nullptr, &renderFinished);
    vkCreateFence(device, &fCreateInfo, nullptr, &drawFence);
}

void VKSetUp::drawFrame()
{
    // Wait for fences
    if (vkWaitForFences(device, 1, &drawFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
        throw std::runtime_error("Could not wait for the fence? (idk)");

    // Acquire the next image from the swap chain
    uint32_t idx = 0;
    if (vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentComplete, drawFence, &idx) != VK_SUCCESS)
        throw std::runtime_error("Could not aquire the next image idx");

    // Record and send the command buffer
    recordCommandBuffer(idx);
    vkResetFences(device, 1, &drawFence);

    // Submit the graphics queue
    VkPipelineStageFlags waitMask   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.commandBufferCount   = 1;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores      = &presentComplete;
    submitInfo.pWaitDstStageMask    = &waitMask;
    submitInfo.pCommandBuffers      = &commandBuffer;
    submitInfo.pSignalSemaphores    = &renderFinished;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFence);

    // Present
    VkPresentInfoKHR present{};
    present.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount  = 1;
    present.swapchainCount      = 1;
    present.pWaitSemaphores     = &renderFinished;
    present.pSwapchains         = &swapChain;
    present.pImageIndices       = &idx;

    if (vkQueuePresentKHR(graphicsQueue, &present) != VK_SUCCESS)
        throw std::runtime_error("Could not present the image");
}

void VKSetUp::cleanup()
{
    for (auto image : SCImageView)
        vkDestroyImageView(device, image, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyShaderModule(device, vShadMod, nullptr);
    vkDestroyShaderModule(device, fShadMod, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}