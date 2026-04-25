#include "InputManager.hpp"
#include <cctype>

namespace Engine::Input {

    InputManager::InputManager(Core::Window& window, Core::Config& config) : m_window(window) {
        keybinds["Forward"] = config.getInt("key_forward", GLFW_KEY_W);
        keybinds["Backward"] = config.getInt("key_backward", GLFW_KEY_S);
        keybinds["Left"] = config.getInt("key_left", GLFW_KEY_A);
        keybinds["Right"] = config.getInt("key_right", GLFW_KEY_D);
        keybinds["Up (Ascend)"] = config.getInt("key_up", GLFW_KEY_LEFT_SHIFT);
        keybinds["Down (Descend)"] = config.getInt("key_down", GLFW_KEY_SPACE);
        keybinds["Roll CW"] = config.getInt("key_roll_cw", GLFW_KEY_E);
        keybinds["Roll CCW"] = config.getInt("key_roll_ccw", GLFW_KEY_Q);
        keybinds["Look Up"] = config.getInt("key_pitch_up", GLFW_KEY_UP);
        keybinds["Look Down"] = config.getInt("key_pitch_down", GLFW_KEY_DOWN);
        keybinds["Look Left"] = config.getInt("key_yaw_left", GLFW_KEY_LEFT);
        keybinds["Look Right"] = config.getInt("key_yaw_right", GLFW_KEY_RIGHT);
        keybinds["Reset Orientation"] = config.getInt("key_reset_orientation", GLFW_KEY_R);
    }

    bool InputManager::isActionPressed(const std::string& action) const {
        auto it = keybinds.find(action);
        if (it != keybinds.end()) {
            return glfwGetKey(m_window.getNativeWindow(), it->second) == GLFW_PRESS;
        }
        return false;
    }

    bool InputManager::wasEscapeJustPressed() {
        bool current = glfwGetKey(m_window.getNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS;
        bool result = current && !m_lastEscapeState;
        m_lastEscapeState = current;
        return result;
    }

    glm::vec2 InputManager::getMouseDelta() {
        double mouseX, mouseY;
        glfwGetCursorPos(m_window.getNativeWindow(), &mouseX, &mouseY);
        
        if (m_firstMouse) {
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
            m_firstMouse = false;
        }

        float deltaX = static_cast<float>(mouseX - m_lastMouseX);
        float deltaY = static_cast<float>(mouseY - m_lastMouseY);
        
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        return glm::vec2(deltaX, deltaY);
    }

    void InputManager::setMouseCapture(bool captured) {
        glfwSetInputMode(m_window.getNativeWindow(), GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (captured) m_firstMouse = true; 
    }

    void InputManager::processKeybindCapture() {
        if (actionWaitingForKey.empty()) return;
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
            if (glfwGetKey(m_window.getNativeWindow(), key) == GLFW_PRESS) {
                if (key != GLFW_KEY_ESCAPE) keybinds[actionWaitingForKey] = key;
                actionWaitingForKey = ""; 
                break;
            }
        }
    }

    std::string InputManager::getKeyName(int key) const {
        const char* name = glfwGetKeyName(key, 0);
        if (name) {
            std::string str(name);
            for (auto& c : str) c = static_cast<char>(toupper(c));
            return str;
        }
        switch(key) {
            case GLFW_KEY_SPACE: return "SPACE";
            case GLFW_KEY_LEFT_SHIFT: return "L_SHIFT";
            case GLFW_KEY_RIGHT_SHIFT: return "R_SHIFT";
            case GLFW_KEY_UP: return "UP_ARROW";
            case GLFW_KEY_DOWN: return "DOWN_ARROW";
            case GLFW_KEY_LEFT: return "LEFT_ARROW";
            case GLFW_KEY_RIGHT: return "RIGHT_ARROW";
            default: return "KEY_" + std::to_string(key);
        }
    }

    void InputManager::saveBindsToConfig(Core::Config& config) {
        config.setInt("key_forward", keybinds["Forward"]);
        config.setInt("key_backward", keybinds["Backward"]);
        config.setInt("key_left", keybinds["Left"]);
        config.setInt("key_right", keybinds["Right"]);
        config.setInt("key_up", keybinds["Up (Ascend)"]);
        config.setInt("key_down", keybinds["Down (Descend)"]);
        config.setInt("key_roll_cw", keybinds["Roll CW"]);
        config.setInt("key_roll_ccw", keybinds["Roll CCW"]);
        config.setInt("key_pitch_up", keybinds["Look Up"]);
        config.setInt("key_pitch_down", keybinds["Look Down"]);
        config.setInt("key_yaw_left", keybinds["Look Left"]);
        config.setInt("key_yaw_right", keybinds["Look Right"]);
        config.setInt("key_reset_orientation", keybinds["Reset Orientation"]);
    }

} // namespace Engine::Input