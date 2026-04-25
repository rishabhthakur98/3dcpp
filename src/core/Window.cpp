#include "Window.hpp"

namespace Engine::Core {

    Window::Window(int width, int height, const std::string& title)
        : m_width(width), m_height(height), m_title(title), m_window(nullptr) {
        
        // Initialize the GLFW library. If it fails, the engine cannot run.
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW library.");
        }

        // Tell GLFW we are using Vulkan, NOT OpenGL. 
        // This stops GLFW from creating an OpenGL context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        // Disable resizing for the initial setup to keep Vulkan swapchain logic simple.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Create the actual window.
        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!m_window) {
            // Clean up GLFW before throwing the error to prevent memory leaks.
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window.");
        }
    }

    Window::~Window() {
        // Destroy the window and terminate GLFW to free OS resources.
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void Window::pollEvents() {
        // This function processes all pending events (input, close requests, etc.)
        glfwPollEvents();
    }

} // namespace Engine::Core