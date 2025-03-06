cmake_minimum_required(VERSION 3.8)
project(VkProj)

# Engine library
add_executable(${PROJECT_NAME} main.cpp "VulkanSetUp.h" "VulkanSetUp.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC .)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)

# GLFW3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

# VULKAN HEADERS
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)