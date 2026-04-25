#include "Renderer.hpp"
#include <iostream>

namespace Engine::Graphics {

    Renderer::Renderer(Core::Window& window) : m_window(window) {
        initVulkan();
    }

    Renderer::~Renderer() {
        // Vulkan cleanup will go here (destroying devices, instances, VMA allocations).
        std::cout << "Cleaning up Vulkan resources...\n";
    }

    void Renderer::initVulkan() {
        // In the future, this is where you will call vkCreateInstance.
        // For now, we verify that GLFW can find the Vulkan loader.
        if (!glfwVulkanSupported()) {
            throw std::runtime_error("Vulkan is not supported on this system or drivers are missing.");
        }
        
        std::cout << "Vulkan support verified. Ready for instance creation.\n";
    }

    void Renderer::drawFrame() {
        // The core rendering loop. 
        // 1. Acquire an image from the swapchain.
        // 2. Execute command buffers (HLSL shaders process geometry/fluids here).
        // 3. Present the image to the screen.
    }

} // namespace Engine::Graphics