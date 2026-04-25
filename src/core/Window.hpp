#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace Engine::Core {

    class Window {
    public:
        Window(int width, int height, bool fullscreen, const std::string& title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool shouldClose() const;
        void pollEvents();
        void closeWindow();

        // --- NEW: Live Window Management ---
        void applyDisplaySettings(bool fullscreen, int width, int height);
        
        bool wasResized() const { return m_framebufferResized; }
        void resetResizedFlag() { m_framebufferResized = false; }

        GLFWwindow* getNativeWindow() const { return m_window; }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }

    private:
        GLFWwindow* m_window;
        int m_width;
        int m_height;
        bool m_isFullscreen;
        bool m_framebufferResized = false;
        std::string m_title;

        // Static callback so GLFW can tell us when the user resizes the window
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    };

} // namespace Engine::Core