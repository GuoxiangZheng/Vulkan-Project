#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>

const unsigned int WIDTH = 800;
const unsigned int  HEIGHT = 600;

// vector of validation layers you want to use to debug
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices
{
    std::optional<unsigned> graphicsFamily;
    bool isComplete() const { return graphicsFamily.has_value(); }
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
    // is not supported in this device (I think it's the GPU that hasn't in this case)
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

static void setupDebugMessenger(VkInstance* instance, VkDebugUtilsMessengerEXT* debug)
{
    if (!enableValidationLayers)
        return;

    // Information about the debug messenger
    VkDebugUtilsMessengerCreateInfoEXT createDInfo{};
    populateDebugMessengerCreateInfo(createDInfo);

    // Create the debug messenger
    if (CreateDebugUtilsMessengerEXT(*instance, &createDInfo, nullptr, debug) != VK_SUCCESS)
        throw std::runtime_error("failed to set up the debug messenger");
}

static QueueFamilyIndices findQueueFamily(VkPhysicalDevice device)
{
    QueueFamilyIndices idx;

    unsigned queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    unsigned i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            idx.graphicsFamily = i;

        if (idx.isComplete())
            break;

        i++;
    }

    return idx;
}

static bool isDeviceSuitable(VkPhysicalDevice device)
{
    // Checks for a dedicated graphics card that support geometry shaders
    /*VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;*/

    // For now, just use the first GPU that it encounters
    QueueFamilyIndices idx = findQueueFamily(device);

    return idx.isComplete();
}

static void pickPhysicalDevice(VkPhysicalDevice* phyDevice, VkInstance* instance)
{
    // Get the number of GPUs available
    unsigned deviceCount = 0;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    // Search for a specific GPU based on the isDeviceSuitable function
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            *phyDevice = device;
            break;
        }
    }

    if (*phyDevice == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU!");
}

static void createLogicalDevice(VkPhysicalDevice phyDevice, VkDevice* device, VkPhysicalDeviceFeatures* deviceFeatures, VkQueue* queue)
{
    QueueFamilyIndices idx = findQueueFamily(phyDevice);

    // Information about the queues
    VkDeviceQueueCreateInfo createQInfo{};
    createQInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    createQInfo.queueFamilyIndex = idx.graphicsFamily.value();
    createQInfo.queueCount = 1;
    float queuePriorirty = 1.f;
    createQInfo.pQueuePriorities = &queuePriorirty;

    // Information about the device/GPU
    VkDeviceCreateInfo createDevInfo{};
    createDevInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createDevInfo.pQueueCreateInfos = &createQInfo;
    createDevInfo.queueCreateInfoCount = 1;
    createDevInfo.pEnabledFeatures = deviceFeatures;
    createDevInfo.enabledExtensionCount = 0;

    // Create the logical device
    if (vkCreateDevice(phyDevice, &createDevInfo, nullptr, device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");

    // Get the handle for the graphics queue. The 0 is the queue index, if there is more than one, 
    // we need to pass the corresponding indices
    vkGetDeviceQueue(*device, idx.graphicsFamily.value(), 0, queue);
}

#pragma endregion

#pragma region HELLO TRIANGLE

class HelloTriangleApplication {
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
    setupDebugMessenger(&instance, &debugMessenger);
    pickPhysicalDevice(&physicalDevice, &instance);
    createLogicalDevice(physicalDevice, &device, &deviceFeatures, &graphicsQueue);
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

    vkDestroyDevice(device, nullptr);
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