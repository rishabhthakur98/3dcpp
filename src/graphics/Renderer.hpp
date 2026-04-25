#pragma once

#include "../core/Window.hpp"

namespace Engine::Graphics {

    // A stub class for the Vulkan Renderer.
    // This will eventually hold your Vulkan Instance, Device, Swapchain, and Pipelines.
    class Renderer {
    public:
        // Pass a reference to our window so the renderer can create a Vulkan drawing surface.
        Renderer(Core::Window& window);
        ~Renderer();

        // Draw a single frame.
        void drawFrame();

    private:
        Core::Window& m_window;
        
        // Helper function to initialize the core Vulkan API.
        void initVulkan();
    };

} // namespace Engine::Graphics