cmake_minimum_required(VERSION 3.8)
project(VulkanProject)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Compiler options (for all targets)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W3)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else ()
    add_compile_options(-Wall -Wextra -pedantic -Wshadow -Wconversion)

    if(CMAKE_COMPILE_WARNING_AS_ERROR)
        add_compile_options(-Werror)
    endif()
endif ()

# For Clion/VSCode
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)