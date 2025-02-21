#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <string>
#include <limits>
#include <algorithm>

const unsigned int WIDTH = 800;
const unsigned int  HEIGHT = 600;

// vector of validation layers you want to use to debug
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices
{
    std::optional<unsigned> graphicsFamily;
    std::optional<unsigned> presentFamily;
    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

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

#pragma region HELPER FUNCTIONS

static std::vector<const char*> getRequiredExtensions()
{
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

static bool checkValidationLayerSupport()
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

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info)
{
    info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugCallback;
}

static void createInstance(VkInstance* instance)
{
    // Check for validation layers in Debug mode
    if (enableValidationLayers && !checkValidationLayerSupport())
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
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<unsigned int>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Pass a debug messenger to the instance if validation layers are enable
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<unsigned int>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Create the instance
    if (vkCreateInstance(&createInfo, nullptr, instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create the instance");
}

static void setupDebugMessenger(const VkInstance& instance, VkDebugUtilsMessengerEXT* debug)
{
    if (!enableValidationLayers)
        return;

    // Information about the debug messenger
    VkDebugUtilsMessengerCreateInfoEXT createDInfo{};
    populateDebugMessengerCreateInfo(createDInfo);

    // Create the debug messenger
    if (CreateDebugUtilsMessengerEXT(instance, &createDInfo, nullptr, debug) != VK_SUCCESS)
        throw std::runtime_error("failed to set up the debug messenger");
}

static SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR& surface)
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

static QueueFamilyIndices findQueueFamily(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
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

static bool checkDeviceExtensionSupport(const VkPhysicalDevice& device)
{
    // Get all available extensions
    unsigned extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtension(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtension.data());

    // If the extension VK_KHR_swapchain is there, then it means that it's capable of creating a swap chain
    std::set<std::string> requiredExtension(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtension)
        requiredExtension.erase(extension.extensionName);

    return requiredExtension.empty();
}

static bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    // Checks for a dedicated graphics card that support geometry shaders
    /*VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;*/

    // For now, just use the first GPU that it encounters
    QueueFamilyIndices idx = findQueueFamily(device, surface);
    bool extensionSupport = checkDeviceExtensionSupport(device);
    bool swapchain = false;
    if (extensionSupport)
    {
        SwapChainSupportDetails swapChainDetails = querySwapChainSupport(device, surface);
        swapchain = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
    }

    return idx.isComplete() && extensionSupport && swapchain;
}

static void pickPhysicalDevice(VkPhysicalDevice* phyDevice, const VkInstance& instance, const VkSurfaceKHR& surface)
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
        if (isDeviceSuitable(device, surface))
        {
            *phyDevice = device;
            break;
        }
    }

    if (*phyDevice == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU!");
}

static void createLogicalDevice(const VkPhysicalDevice& phyDevice, const VkSurfaceKHR& surface, VkDevice* device, VkPhysicalDeviceFeatures* deviceFeatures, VkQueue* gQueue, VkQueue* pQueue)
{
    QueueFamilyIndices idx = findQueueFamily(phyDevice, surface);

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
    createDevInfo.pEnabledFeatures = deviceFeatures;
    createDevInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createDevInfo.enabledExtensionCount = static_cast<unsigned>(deviceExtensions.size());

    // Create the logical device
    if (vkCreateDevice(phyDevice, &createDevInfo, nullptr, device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");

    // Get the handles for the graphics and presentation queues. The 0 is the queue index, if there is more than one, 
    // we need to pass the corresponding indices
    vkGetDeviceQueue(*device, idx.graphicsFamily.value(), 0, gQueue);
    vkGetDeviceQueue(*device, idx.presentFamily.value(), 0, pQueue);
}

static void createSurface(const VkInstance& instance, GLFWwindow* window, VkSurfaceKHR* surface)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface");
}

static VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const SwapChainSupportDetails& details)
{
    for (const auto& formats : details.formats)
    {
        if (formats.format == VK_FORMAT_B8G8R8A8_SRGB && formats.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return formats;
    }

    return details.formats.front();
}

static VkPresentModeKHR chooseSwapPresentMode(const SwapChainSupportDetails& details)
{
    for (const auto& present : details.presentModes)
    {
        if (present == VK_PRESENT_MODE_MAILBOX_KHR)
            return present;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const SwapChainSupportDetails& details, GLFWwindow* window)
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

static void createSwapChain(const VkPhysicalDevice& phyDevice, const VkDevice& device, const VkSurfaceKHR& surface, GLFWwindow* window, VkSwapchainKHR* swapChain, VkExtent2D* extent_, VkFormat* format_, std::vector<VkImage>& images)
{
    // Get the surface details to render onto the window
    SwapChainSupportDetails details = querySwapChainSupport(phyDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapChainSurfaceFormat(details);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(details);
    VkExtent2D extent = chooseSwapExtent(details, window);

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

    QueueFamilyIndices idx = findQueueFamily(phyDevice, surface);
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
    if (vkCreateSwapchainKHR(device, &createSCIfno, nullptr, swapChain) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain!");

    // Get the handles for the images of the swap chain
    vkGetSwapchainImagesKHR(device, *swapChain, &imgCount, nullptr);
    images.resize(imgCount);
    vkGetSwapchainImagesKHR(device, *swapChain, &imgCount, images.data());

    // Get the extent and format
    *extent_ = extent;
    *format_ = surfaceFormat.format;
}

#pragma endregion

#pragma region HELLO TRIANGLE

class HelloTriangleApplication
{
public:
    void run();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    GLFWwindow* window = nullptr;
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debugMessenger{};
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDevice device{};
    VkQueue graphicsQueue{};
    VkQueue presentQueue{};
    VkSurfaceKHR surface{};
    VkSwapchainKHR swapChain{};
    VkExtent2D extent{};
    VkFormat format{};
    std::vector<VkImage> swapChainImages;
};

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow()
{
    // Initialize GLFW
    glfwInit();

    // Initialize the window without using OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create the window
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::initVulkan()
{
    createInstance(&instance);
    setupDebugMessenger(instance, &debugMessenger);
    createSurface(instance, window, &surface);
    pickPhysicalDevice(&physicalDevice, instance, surface);
    createLogicalDevice(physicalDevice, surface, &device, &deviceFeatures, &graphicsQueue, &presentQueue);
    createSwapChain(physicalDevice, device, surface, window, &swapChain, &extent, &format, swapChainImages);
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::cleanup()
{
    if (enableValidationLayers)
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

#pragma endregion

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}