#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "../core/Window.hpp"
#include "../core/Config.hpp"

namespace Engine::Input {

    class InputManager {
    public:
        InputManager(Core::Window& window, Core::Config& config);
        ~InputManager() = default;

        // Call every frame
        void processKeybindCapture();
        
        // Polling states
        bool isActionPressed(const std::string& action) const;
        bool wasEscapeJustPressed();
        
        // Mouse handling
        glm::vec2 getMouseDelta();
        void setMouseCapture(bool captured);

        // UI Keybind mapping
        std::unordered_map<std::string, int> keybinds;
        std::string actionWaitingForKey = "";
        std::string getKeyName(int key) const;
        void saveBindsToConfig(Core::Config& config);

    private:
        Core::Window& m_window;
        
        double m_lastMouseX, m_lastMouseY;
        bool m_firstMouse = true;
        bool m_lastEscapeState = false;
    };

} // namespace Engine::Input