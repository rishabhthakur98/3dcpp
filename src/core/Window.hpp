#pragma once

// GLFW handles window creation and OS-level input events.
// We define GLFW_INCLUDE_VULKAN before including glfw3.h so it automatically 
// includes the Vulkan headers we need.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <stdexcept>

namespace Engine::Core {

    // A class to encapsulate the OS window and input handling.
    class Window {
    public:
        // Initialize the window with a specific width, height, and title.
        Window(int width, int height, const std::string& title);
        
        // Destructor ensures resources are cleaned up properly when the window closes.
        ~Window();

        // Prevent copying of the window object to avoid double-freeing GLFW resources.
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        // Check if the user has requested the window to close (e.g., clicking the 'X').
        bool shouldClose() const;

        // Process OS events (keyboard, mouse, window resizing).
        void pollEvents();

        // Get the raw GLFW window pointer for Vulkan surface creation later.
        GLFWwindow* getNativeWindow() const { return m_window; }

    private:
        GLFWwindow* m_window;
        int m_width;
        int m_height;
        std::string m_title;
    };

} // namespace Engine::Core