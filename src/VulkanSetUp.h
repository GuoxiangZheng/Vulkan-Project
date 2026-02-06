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

// vector of validation layers you want to use to debug
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

// vector of extensions you want to use
const std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };

class VKSetUp
{
public:

    VKSetUp(){}

    void InitWindow(unsigned width, unsigned height);

    std::vector<const char*>    getRequiredExtensions(const bool& enableLayer);
    GLFWwindow*                 getWindow() const { return window; }
    VkDevice                    getDevice() const { return device; }
    
    void setupDebugMessenger(const bool& enableLayer);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface();
    void createInstance(const bool& enableLayer);
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();
    void createCommandPool();
    void createCommandBuffer();
    void createSyncObjs();

    void drawFrame();

    void destroyDebugMessenger() const;
    void cleanup();

private:

    bool checkValidationLayerSupport() const;
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& device_) const;
    bool isDeviceSuitable(const VkPhysicalDevice& device) const;

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info);

    VkSurfaceFormatKHR  chooseSwapChainSurfaceFormat(const SwapChainSupportDetails& details);
    VkPresentModeKHR    chooseSwapPresentMode(const SwapChainSupportDetails& details);
    VkExtent2D          chooseSwapExtent(const SwapChainSupportDetails& details);
    VkShaderModule      createShaderModule(const std::vector<char>& code) const;

    SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice device) const;
    QueueFamilyIndices      findQueueFamily(const VkPhysicalDevice& device) const;

    void recordCommandBuffer(uint32_t imgIdx);
    void transitionImgLayout(uint32_t imgIdx,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkAccessFlags2 srcAM,
        VkAccessFlags2 dstAM,
        VkPipelineStageFlags2 srcSM,
        VkPipelineStageFlags2 dstSM);

    GLFWwindow* window = nullptr;
    
    VkInstance instance = nullptr;
    
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
    
    VkPhysicalDevice            physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures    deviceFeatures{};
    
    VkDevice device{};
    
    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    
    VkSurfaceKHR surface = nullptr;
    
    VkSwapchainKHR swapChain = nullptr;
    
    VkExtent2D mExtent{};
    VkFormat   mFormat{};
    
    VkShaderModule vShadMod = nullptr;
    VkShaderModule fShadMod = nullptr;
    
    VkPipelineLayout    layout           = nullptr;
    VkPipeline          graphicsPipeline = nullptr;

    VkCommandPool   commandPool     = nullptr;
    VkCommandBuffer commandBuffer   = nullptr;

    VkSemaphore presentComplete = nullptr;
    VkSemaphore renderFinished  = nullptr;
    VkFence     drawFence       = nullptr;

    std::vector<VkImage>        swapChainImages;
    std::vector<VkImageView>    SCImageView;
};