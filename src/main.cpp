#include "VulkanSetUp.h"

const unsigned int WIDTH = 800;
const unsigned int  HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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

    VKSetUp mSetUp;
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
    mSetUp.InitWindow(WIDTH, HEIGHT);
}

void HelloTriangleApplication::initVulkan()
{
    mSetUp.createInstance(enableValidationLayers);
    mSetUp.setupDebugMessenger(enableValidationLayers);
    mSetUp.createSurface();
    mSetUp.pickPhysicalDevice();
    mSetUp.createLogicalDevice();
    mSetUp.createSwapChain();
    mSetUp.createImageViews();
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(mSetUp.getWindow()))
    {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::cleanup()
{
    if (enableValidationLayers)
        mSetUp.destroyDebugMessenger();

    mSetUp.cleanup();
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