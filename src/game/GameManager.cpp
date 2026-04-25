#include "GameManager.hpp"
#include <iostream>
#include <filesystem>
#include <cctype>
#include <imgui.h> 

namespace Engine::Game {

    GameManager::GameManager(Core::Window& window, Core::Config& config, Graphics::Renderer& renderer, Graphics::ModelLoader& modelLoader)
        : m_window(window), m_config(config), m_renderer(renderer), m_modelLoader(modelLoader), 
          m_state(EngineState::MainMenu), m_menuIndex(0), m_escapePressed(false),
          m_firstMouse(true), m_actionWaitingForKey("") {
        
        m_uiFullscreen = m_config.getBool("fullscreen", false);
        m_uiResolutionIndex = m_config.getInt("resolution_index", 3); 
        m_uiLimitFrames = m_config.getBool("limit_frames", true);
        m_uiTargetFps = m_config.getInt("target_fps", 60);
        m_uiContrast = m_config.getFloat("contrast", 1.0f);
        
        m_uiHwRaytracing = m_config.getBool("hw_raytracing", false);
        m_uiMeshShaders = m_config.getBool("mesh_shaders", false);
        m_uiSoftwareGI = m_config.getBool("software_gi", true);
        m_uiVRS = m_config.getBool("vrs", false);

        m_camera.mouseSensitivity = m_config.getFloat("mouse_sensitivity", 0.1f);
        m_camera.keyboardLookSensitivity = m_config.getFloat("keyboard_look_sensitivity", 90.0f);
        m_camera.moveSpeed = m_config.getFloat("move_speed", 5.0f);
        m_camera.rollSpeed = m_config.getFloat("roll_speed", 60.0f);

        m_keybinds["Forward"] = m_config.getInt("key_forward", GLFW_KEY_W);
        m_keybinds["Backward"] = m_config.getInt("key_backward", GLFW_KEY_S);
        m_keybinds["Left"] = m_config.getInt("key_left", GLFW_KEY_A);
        m_keybinds["Right"] = m_config.getInt("key_right", GLFW_KEY_D);
        m_keybinds["Up (Ascend)"] = m_config.getInt("key_up", GLFW_KEY_LEFT_SHIFT);
        m_keybinds["Down (Descend)"] = m_config.getInt("key_down", GLFW_KEY_SPACE);
        m_keybinds["Roll CW"] = m_config.getInt("key_roll_cw", GLFW_KEY_E);
        m_keybinds["Roll CCW"] = m_config.getInt("key_roll_ccw", GLFW_KEY_Q);
        
        m_keybinds["Look Up"] = m_config.getInt("key_pitch_up", GLFW_KEY_UP);
        m_keybinds["Look Down"] = m_config.getInt("key_pitch_down", GLFW_KEY_DOWN);
        m_keybinds["Look Left"] = m_config.getInt("key_yaw_left", GLFW_KEY_LEFT);
        m_keybinds["Look Right"] = m_config.getInt("key_yaw_right", GLFW_KEY_RIGHT);
    }

    bool GameManager::shouldQuit() const { return m_state == EngineState::QuitRequested; }

    std::string GameManager::getKeyName(int key) const {
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
            case GLFW_KEY_LEFT_CONTROL: return "L_CTRL";
            case GLFW_KEY_UP: return "UP_ARROW";
            case GLFW_KEY_DOWN: return "DOWN_ARROW";
            case GLFW_KEY_LEFT: return "LEFT_ARROW";
            case GLFW_KEY_RIGHT: return "RIGHT_ARROW";
            default: return "KEY_" + std::to_string(key);
        }
    }

    void GameManager::processKeybindCapture() {
        if (m_actionWaitingForKey.empty()) return;

        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
            if (glfwGetKey(m_window.getNativeWindow(), key) == GLFW_PRESS) {
                if (key != GLFW_KEY_ESCAPE) { 
                    m_keybinds[m_actionWaitingForKey] = key;
                }
                m_actionWaitingForKey = ""; 
                break;
            }
        }
    }

    void GameManager::pollInput() {
        GLFWwindow* rawWindow = m_window.getNativeWindow();
        bool escape = (glfwGetKey(rawWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        static bool lastEscape = false;
        m_escapePressed = escape && !lastEscape;
        lastEscape = escape;
    }

    void GameManager::update(float dt) {
        pollInput();

        if (!m_actionWaitingForKey.empty()) {
            processKeybindCapture();
        }

        bool stylesPushed = false;
        if (m_state != EngineState::Playing) {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            stylesPushed = true;
        }

        switch (m_state) {
            case EngineState::MainMenu: handleMainMenu(); break;
            case EngineState::WorldSelect: handleWorldSelect(); break;
            case EngineState::Settings: handleSettings(); break;
            case EngineState::Playing: handlePlaying(dt); break;
            case EngineState::QuitRequested: break;
        }

        if (stylesPushed) ImGui::PopStyleColor(2);

        glm::mat4 viewProj = m_camera.getViewProjection(static_cast<float>(m_window.getWidth()), static_cast<float>(m_window.getHeight()));
        m_renderer.drawFrame(viewProj);
    }

    void GameManager::handleMainMenu() {
        ImGui::Begin("MainMenu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Dummy(ImVec2(0.0f, 100.0f));
        ImGui::Indent(100.0f); 
        
        ImGui::Text("=== 3D C++ ENGINE ===");
        ImGui::Dummy(ImVec2(0.0f, 30.0f));

        if (ImGui::Button("Load Game", ImVec2(250, 50))) m_state = EngineState::WorldSelect;
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("Settings", ImVec2(250, 50))) m_state = EngineState::Settings;
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("Quit Game", ImVec2(250, 50))) { 
            m_state = EngineState::QuitRequested; 
            m_window.closeWindow(); 
        }
        ImGui::End();
    }

    void GameManager::handleWorldSelect() {
        ImGui::Begin("WorldSelect", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Dummy(ImVec2(0.0f, 100.0f));
        ImGui::Indent(100.0f); 
        ImGui::Text("=== SELECT WORLD ===");
        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        std::string worldsDir = m_config.getString("worlds_dir", "assets/worlds");
        bool foundWorlds = false;

        if (std::filesystem::exists(worldsDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(worldsDir)) {
                if (entry.is_directory() && std::filesystem::exists(entry.path() / "level.scene")) {
                    foundWorlds = true;
                    std::string worldName = entry.path().filename().string();
                    if (ImGui::Button(worldName.c_str(), ImVec2(300, 50))) loadWorld(worldName);
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                }
            }
        }

        if (!foundWorlds) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No valid worlds found in: %s", worldsDir.c_str());
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
        }

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (ImGui::Button("Back", ImVec2(150, 40)) || m_escapePressed) m_state = EngineState::MainMenu; 

        ImGui::End();
    }

    void GameManager::handleSettings() {
        ImGui::Begin("SettingsMenu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Dummy(ImVec2(0.0f, 50.0f));
        ImGui::Indent(50.0f);
        ImGui::Text("=== SETTINGS ===");
        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        if (ImGui::BeginTabBar("SettingsTabs")) {
            
            if (ImGui::BeginTabItem("Display")) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Checkbox("Fullscreen Mode", &m_uiFullscreen);
                const char* resolutions[] = { "7680x4320 (8K)", "3840x2160 (4K)", "2560x1440 (2K)", "1920x1080 (FHD)", "1366x768 (HD)", "1280x720 (720p)" };
                ImGui::Combo("Resolution", &m_uiResolutionIndex, resolutions, IM_ARRAYSIZE(resolutions));
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Checkbox("Limit Framerate", &m_uiLimitFrames);
                if (m_uiLimitFrames) ImGui::SliderInt("Target FPS", &m_uiTargetFps, 30, 240);
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::SliderFloat("Contrast", &m_uiContrast, 0.5f, 2.0f);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Controls")) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Sensitivities:");
                ImGui::SliderFloat("Mouse Sensitivity", &m_camera.mouseSensitivity, 0.01f, 1.0f);
                ImGui::SliderFloat("Keyboard Look", &m_camera.keyboardLookSensitivity, 10.0f, 200.0f);
                ImGui::SliderFloat("Movement Speed", &m_camera.moveSpeed, 1.0f, 50.0f);
                ImGui::SliderFloat("Roll Speed", &m_camera.rollSpeed, 10.0f, 180.0f);
                
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Key Bindings (Click to rebind):");
                ImGui::Dummy(ImVec2(0.0f, 5.0f));

                std::vector<std::string> bindOrder = {
                    "Forward", "Backward", "Left", "Right", 
                    "Up (Ascend)", "Down (Descend)", "Roll CW", "Roll CCW",
                    "Look Up", "Look Down", "Look Left", "Look Right"
                };

                for (const auto& action : bindOrder) {
                    ImGui::Text("%s:", action.c_str());
                    ImGui::SameLine(180.0f);
                    
                    std::string btnLabel = (m_actionWaitingForKey == action) ? "[ PRESS ANY KEY ]" : "[ " + getKeyName(m_keybinds[action]) + " ]";
                    
                    // --- THE FIX: State Snapshot ---
                    // By saving the state to a local boolean before the button is drawn,
                    // we guarantee the push and pop match perfectly even if the button changes the state!
                    bool isCurrentlyWaiting = (m_actionWaitingForKey == action);
                    
                    if (isCurrentlyWaiting) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    }
                    
                    if (ImGui::Button((btnLabel + "##" + action).c_str(), ImVec2(150, 0))) {
                        m_actionWaitingForKey = action;
                    }
                    
                    if (isCurrentlyWaiting) {
                        ImGui::PopStyleColor();
                    }
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("AAA Pipeline")) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::TextWrapped("These cutting-edge features require specific GPU architecture support.");
                ImGui::Dummy(ImVec2(0.0f, 10.0f));

                const auto& specs = m_renderer.getGpuSpecs();
                
                if (!specs.supportsHwRayTracing) { ImGui::BeginDisabled(); m_uiHwRaytracing = false; }
                ImGui::Checkbox("Hardware Ray Tracing", &m_uiHwRaytracing);
                if (!specs.supportsHwRayTracing) { ImGui::EndDisabled(); ImGui::SameLine(); ImGui::TextColored(ImVec4(1,0.4,0.4,1), "(Unsupported)"); }

                if (!specs.supportsMeshShaders) { ImGui::BeginDisabled(); m_uiMeshShaders = false; }
                ImGui::Checkbox("Nanite (Mesh Shaders)", &m_uiMeshShaders);
                if (!specs.supportsMeshShaders) { ImGui::EndDisabled(); ImGui::SameLine(); ImGui::TextColored(ImVec4(1,0.4,0.4,1), "(Unsupported)"); }

                ImGui::Checkbox("Lumen (Software GI)", &m_uiSoftwareGI);
                ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4,1,0.4,1), "(Supported via Compute)");

                if (!specs.supportsVRS) { ImGui::BeginDisabled(); m_uiVRS = false; }
                ImGui::Checkbox("Variable Rate Shading", &m_uiVRS);
                if (!specs.supportsVRS) { ImGui::EndDisabled(); ImGui::SameLine(); ImGui::TextColored(ImVec4(1,0.4,0.4,1), "(Unsupported)"); }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Dummy(ImVec2(0.0f, 50.0f));
        
        if (ImGui::Button("Apply and Save", ImVec2(200, 40))) {
            m_config.setBool("fullscreen", m_uiFullscreen);
            m_config.setInt("resolution_index", m_uiResolutionIndex);
            m_config.setBool("limit_frames", m_uiLimitFrames);
            m_config.setInt("target_fps", m_uiTargetFps);
            m_config.setFloat("contrast", m_uiContrast);
            
            m_config.setFloat("mouse_sensitivity", m_camera.mouseSensitivity);
            m_config.setFloat("keyboard_look_sensitivity", m_camera.keyboardLookSensitivity);
            m_config.setFloat("move_speed", m_camera.moveSpeed);
            m_config.setFloat("roll_speed", m_camera.rollSpeed);

            m_config.setInt("key_forward", m_keybinds["Forward"]);
            m_config.setInt("key_backward", m_keybinds["Backward"]);
            m_config.setInt("key_left", m_keybinds["Left"]);
            m_config.setInt("key_right", m_keybinds["Right"]);
            m_config.setInt("key_up", m_keybinds["Up (Ascend)"]);
            m_config.setInt("key_down", m_keybinds["Down (Descend)"]);
            m_config.setInt("key_roll_cw", m_keybinds["Roll CW"]);
            m_config.setInt("key_roll_ccw", m_keybinds["Roll CCW"]);
            m_config.setInt("key_pitch_up", m_keybinds["Look Up"]);
            m_config.setInt("key_pitch_down", m_keybinds["Look Down"]);
            m_config.setInt("key_yaw_left", m_keybinds["Look Left"]);
            m_config.setInt("key_yaw_right", m_keybinds["Look Right"]);

            m_config.setBool("hw_raytracing", m_uiHwRaytracing);
            m_config.setBool("mesh_shaders", m_uiMeshShaders);
            m_config.setBool("software_gi", m_uiSoftwareGI);
            m_config.setBool("vrs", m_uiVRS);
            m_config.save();
            
            int winWidth = 1920, winHeight = 1080;
            switch(m_uiResolutionIndex) {
                case 0: winWidth = 7680; winHeight = 4320; break; 
                case 1: winWidth = 3840; winHeight = 2160; break; 
                case 2: winWidth = 2560; winHeight = 1440; break; 
                case 3: winWidth = 1920; winHeight = 1080; break; 
                case 4: winWidth = 1366; winHeight = 768;  break; 
                case 5: winWidth = 1280; winHeight = 720;  break; 
            }
            m_window.applyDisplaySettings(m_uiFullscreen, winWidth, winHeight);
            m_state = EngineState::MainMenu;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(150, 40)) || m_escapePressed) {
            m_state = EngineState::MainMenu;
            m_actionWaitingForKey = ""; 
        }

        ImGui::End();
    }

    void GameManager::loadWorld(const std::string& worldName) {
        m_activeEntities.clear();
        m_modelLoader.clearCache();
        
        std::string dir = m_config.getString("worlds_dir", "assets/worlds");
        std::string filepath = dir + "/" + worldName + "/level.scene";
        
        try {
            m_activeEntities = m_sceneLoader.loadScene(filepath);
            for (const auto& entity : m_activeEntities) {
                m_modelLoader.loadModel(entity.modelPath);
            }
            
            m_camera.resetPosition();
            m_state = EngineState::Playing;

            glfwSetInputMode(m_window.getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_firstMouse = true;

        } catch (const std::exception& e) {
            std::cerr << "[Warning]: " << e.what() << "\n";
            m_state = EngineState::WorldSelect;
        }
    }

    void GameManager::handlePlaying(float dt) {
        GLFWwindow* win = m_window.getNativeWindow();

        double mouseX, mouseY;
        glfwGetCursorPos(win, &mouseX, &mouseY);
        
        if (m_firstMouse) {
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
            m_firstMouse = false;
        }

        float deltaX = static_cast<float>(mouseX - m_lastMouseX) * m_camera.mouseSensitivity;
        float deltaY = static_cast<float>(mouseY - m_lastMouseY) * m_camera.mouseSensitivity;
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        
        if (glfwGetKey(win, m_keybinds["Look Left"]) == GLFW_PRESS)  deltaX -= m_camera.keyboardLookSensitivity * dt;
        if (glfwGetKey(win, m_keybinds["Look Right"]) == GLFW_PRESS) deltaX += m_camera.keyboardLookSensitivity * dt;
        if (glfwGetKey(win, m_keybinds["Look Up"]) == GLFW_PRESS)    deltaY -= m_camera.keyboardLookSensitivity * dt;
        if (glfwGetKey(win, m_keybinds["Look Down"]) == GLFW_PRESS)  deltaY += m_camera.keyboardLookSensitivity * dt;

        float roll = 0.0f;
        if (glfwGetKey(win, m_keybinds["Roll CW"]) == GLFW_PRESS) roll += 1.0f; 
        if (glfwGetKey(win, m_keybinds["Roll CCW"]) == GLFW_PRESS) roll -= 1.0f; 

        glm::vec3 move(0.0f);
        if (glfwGetKey(win, m_keybinds["Forward"]) == GLFW_PRESS) move.z += 1.0f; 
        if (glfwGetKey(win, m_keybinds["Backward"]) == GLFW_PRESS) move.z -= 1.0f; 
        if (glfwGetKey(win, m_keybinds["Left"]) == GLFW_PRESS) move.x -= 1.0f; 
        if (glfwGetKey(win, m_keybinds["Right"]) == GLFW_PRESS) move.x += 1.0f; 
        if (glfwGetKey(win, m_keybinds["Up (Ascend)"]) == GLFW_PRESS) move.y += 1.0f; 
        if (glfwGetKey(win, m_keybinds["Down (Descend)"]) == GLFW_PRESS) move.y -= 1.0f; 

        glm::vec3 look(deltaY, deltaX, 0.0f);
        m_camera.update(dt, move, look, roll);

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(0,1,0,1), "GAMEPLAY ACTIVE (6DOF Camera)");
        ImGui::Text("Entities in RAM: %zu", m_activeEntities.size());
        ImGui::Text("Press ESCAPE to pause");
        ImGui::End();

        if (m_escapePressed) {
            m_state = EngineState::MainMenu;
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

} // namespace Engine::Game