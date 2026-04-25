#include "Window.hpp"
#include <stdexcept>
#include <iostream>

namespace Engine::Core {

    Window::Window(int width, int height, bool fullscreen, const std::string& title)
        : m_width(width), m_height(height), m_isFullscreen(fullscreen), m_title(title), m_window(nullptr) {
        
        if (!glfwInit()) throw std::runtime_error("Critical Error: Failed to initialize GLFW.");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // We now ALLOW resizing so the user can drag the window edges!
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); 

        GLFWmonitor* monitor = nullptr;
        if (m_isFullscreen) {
            monitor = glfwGetPrimaryMonitor();
            if (monitor) {
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                m_width = mode->width;
                m_height = mode->height;
            }
        } 

        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), monitor, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Critical Error: Failed to create GLFW window.");
        }

        // Link our C++ class to the C-style GLFW callback
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    }

    Window::~Window() {
        if (m_window) glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void Window::applyDisplaySettings(bool fullscreen, int width, int height) {
        m_isFullscreen = fullscreen;
        m_width = width;
        m_height = height;

        if (fullscreen) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            // Revert to a centered windowed mode
            glfwSetWindowMonitor(m_window, nullptr, 100, 100, width, height, GLFW_DONT_CARE);
        }
        m_framebufferResized = true; // Tell Vulkan it needs to rebuild the swapchain!
    }

    void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
        app->m_width = width;
        app->m_height = height;
    }

    bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }
    void Window::closeWindow() { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }
    void Window::pollEvents() { glfwPollEvents(); }

} // namespace Engine::Core