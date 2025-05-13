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
    info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugCallback;
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

    actualExtent.width = std::clamp(actualExtent.width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);

    return actualExtent;
}

VkShaderModule VKSetUp::createShaderModule(const std::vector<char>& code)
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
        createQInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createQInfo.queueFamilyIndex = qFamily;
        createQInfo.queueCount = 1;
        createQInfo.pQueuePriorities = &queuePriorirty;
        createQInfos.push_back(createQInfo);
    }

    // Information about the device/GPU
    VkDeviceCreateInfo createDevInfo{};
    createDevInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createDevInfo.queueCreateInfoCount = static_cast<unsigned>(createQInfos.size());
    createDevInfo.pQueueCreateInfos = createQInfos.data();
    createDevInfo.pEnabledFeatures = &deviceFeatures;
    createDevInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createDevInfo.enabledExtensionCount = static_cast<unsigned>(deviceExtensions.size());

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
    // Check for validation layers in Debug mode
    if (enableLayer && !checkValidationLayerSupport())
        throw std::runtime_error("validation layer requested, but no available");

    // Information about the program
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Information about the instance (It's the one that connects the program with the Vulkan library)
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get all required extensions for the instance
    auto extensions = getRequiredExtensions(enableLayer);
    createInfo.enabledExtensionCount = static_cast<unsigned int>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Pass a debug messenger to the instance if validation layers are enable
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableLayer)
    {
        createInfo.enabledLayerCount = static_cast<unsigned int>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
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
    VkPresentModeKHR presentMode = chooseSwapPresentMode(details);
    VkExtent2D extent = chooseSwapExtent(details);

    // Get the amount of images to have in the swap chain
    unsigned imgCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imgCount > details.capabilities.maxImageCount)
        imgCount = details.capabilities.maxImageCount;

    // Information about swap chain
    VkSwapchainCreateInfoKHR createSCIfno{};
    createSCIfno.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createSCIfno.surface = surface;
    createSCIfno.minImageCount = imgCount;
    createSCIfno.imageFormat = surfaceFormat.format;
    createSCIfno.imageColorSpace = surfaceFormat.colorSpace;
    createSCIfno.imageExtent = extent;
    createSCIfno.imageArrayLayers = 1;
    createSCIfno.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices idx = findQueueFamily(physicalDevice);
    unsigned indices[] = { idx.graphicsFamily.value(), idx.presentFamily.value() };
    if (idx.graphicsFamily != idx.presentFamily)
    {
        createSCIfno.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createSCIfno.queueFamilyIndexCount = 2;
        createSCIfno.pQueueFamilyIndices = indices;
    }
    else
    {
        createSCIfno.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createSCIfno.queueFamilyIndexCount = 0;     // Optional
        createSCIfno.pQueueFamilyIndices = nullptr; // Optional
    }

    createSCIfno.preTransform = details.capabilities.currentTransform;
    createSCIfno.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createSCIfno.presentMode = presentMode;
    createSCIfno.clipped = VK_TRUE;
    createSCIfno.oldSwapchain = VK_NULL_HANDLE;

    // Create the swap chain
    if (vkCreateSwapchainKHR(device, &createSCIfno, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain!");

    // Get the handles for the images of the swap chain
    vkGetSwapchainImagesKHR(device, swapChain, &imgCount, nullptr);
    swapChainImages.resize(imgCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imgCount, swapChainImages.data());

    // Get the extent and format
    mExtent = extent;
    format = surfaceFormat.format;
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
            idx.presentFamily = i;
        }

        if (idx.isComplete())
            break;

        i++;
    }

    return idx;
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
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // The type of texture that it will be storing the data (1D, 2D or 3D textures)
        createInfo.format = format;

        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;

        // The type of texture that it will output to the window (all colors, black & white, ...)
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

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
    // read SPIR-V shader code. To generate .spv files, go to 
    // "data/shaders/compile.bat" and double-click it.
    auto vertShad = readFile("data/shaders/vert.spv");
    auto fragShad = readFile("data/shaders/frag.spv");

    // create the modules for the vertex and fragment shaders
    vShadMod = createShaderModule(vertShad);
    fShadMod = createShaderModule(fragShad);

    // create shader stages to actually use the shaders
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vShadMod;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fShadMod;
    fragShaderStageInfo.pName = "main";

    // destroy the modules
    vkDestroyShaderModule(device, vShadMod, nullptr);
    vkDestroyShaderModule(device, fShadMod, nullptr);
}

void VKSetUp::cleanup()
{
    for (auto image : SCImageView)
        vkDestroyImageView(device, image, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}