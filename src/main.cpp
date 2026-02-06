#include "VulkanSetUp.h"

int WIDTH  = 800;
int HEIGHT = 600;

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
    mSetUp.createGraphicsPipeline();
    mSetUp.createCommandPool();
    mSetUp.createCommandBuffer();
    mSetUp.createSyncObjs();
}

void HelloTriangleApplication::mainLoop()
{
    auto window = mSetUp.getWindow();
    while (!glfwWindowShouldClose(mSetUp.getWindow()))
    {
        glfwPollEvents();

        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &WIDTH, &HEIGHT);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE))
            break;

        mSetUp.drawFrame();
    }

    vkDeviceWaitIdle(mSetUp.getDevice());
}

void HelloTriangleApplication::cleanup()
{
    if (enableValidationLayers)
        mSetUp.destroyDebugMessenger();

    mSetUp.cleanup();
}

#pragma endregion

int main()
{
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