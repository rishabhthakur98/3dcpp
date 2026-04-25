#include "core/Window.hpp"
#include "graphics/Renderer.hpp"
#include <iostream>

int main() {
    try {
        // 1. Initialize the Window (800x600 resolution)
        Engine::Core::Window window(800, 600, "3D C++ Engine (Vulkan + HLSL)");

        // 2. Initialize the Graphics API (Vulkan)
        Engine::Graphics::Renderer renderer(window);

        // 3. Main Game Loop
        std::cout << "Entering main loop...\n";
        while (!window.shouldClose()) {
            
            // Process OS input and window events
            window.pollEvents();

            // Run Physics simulation step here (Jolt Physics / Fluid updates)
            // physicsSystem.step();

            // Render the visual frame
            renderer.drawFrame();
        }

    } catch (const std::exception& e) {
        // Catch any setup errors (e.g., Vulkan missing, window creation failing)
        std::cerr << "Engine Fatal Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}