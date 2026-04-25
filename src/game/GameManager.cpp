#include "GameManager.hpp"
#include <iostream>
#include <filesystem>
#include <imgui.h> 

namespace Engine::Game {

    GameManager::GameManager(Core::Window& window, Core::Config& config, Graphics::Renderer& renderer, Graphics::ModelLoader& modelLoader)
        : m_window(window), m_config(config), m_renderer(renderer), m_modelLoader(modelLoader), 
          m_inputManager(window, config), m_state(EngineState::MainMenu), m_menuIndex(0) {
        
        m_uiFullscreen = m_config.getBool("fullscreen", false);
        m_uiResolutionIndex = m_config.getInt("resolution_index", 3); 
        m_uiLimitFrames = m_config.getBool("limit_frames", true);
        m_uiTargetFps = m_config.getInt("target_fps", 60);
        m_uiContrast = m_config.getFloat("contrast", 1.0f);
        
        m_uiHwRaytracing = m_config.getBool("hw_raytracing", false);
        m_uiMeshShaders = m_config.getBool("mesh_shaders", false);
        m_uiSoftwareGI = m_config.getBool("software_gi", true);
        m_uiVRS = m_config.getBool("vrs", false);
        
        m_uiCullEnabled = m_config.getBool("cull_enabled", false);
        m_uiCullMode = m_config.getInt("cull_mode", 0);
        m_uiDevMode = m_config.getBool("dev_mode", true);

        m_uiCamStartX = m_config.getFloat("cam_start_x", 0.0f);
        m_uiCamStartY = m_config.getFloat("cam_start_y", 0.0f);
        m_uiCamStartZ = m_config.getFloat("cam_start_z", 0.0f);
        m_uiCamStartPitch = m_config.getFloat("cam_start_pitch", 0.0f);
        m_uiCamStartYaw = m_config.getFloat("cam_start_yaw", 0.0f);
        m_uiCamStartRoll = m_config.getFloat("cam_start_roll", 0.0f);

        m_camera.mouseSensitivity = m_config.getFloat("mouse_sensitivity", 0.1f);
        m_camera.keyboardLookSensitivity = m_config.getFloat("keyboard_look_sensitivity", 90.0f);
        m_camera.moveSpeed = m_config.getFloat("move_speed", 5.0f);
        m_camera.rollSpeed = m_config.getFloat("roll_speed", 60.0f);
    }

    bool GameManager::shouldQuit() const { return m_state == EngineState::QuitRequested; }

    void GameManager::update(float dt) {
        // Delegate key rebinding interception to the InputManager
        if (!m_inputManager.actionWaitingForKey.empty()) {
            m_inputManager.processKeybindCapture();
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

        if (ImGui::Button("Back", ImVec2(150, 40)) || m_inputManager.wasEscapeJustPressed()) m_state = EngineState::MainMenu; 

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
                
                const char* resolutions[] = { 
                    "7680x4320 (8K)", "3840x2160 (4K)", "2560x1440 (1440p)", 
                    "1920x1080 (1080p)", "1600x900 (900p)", "1366x768 (768p)", 
                    "1280x720 (720p)", "854x480 (480p)", "640x360 (360p)", 
                    "426x240 (240p)", "256x144 (144p)" 
                };
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
                    "Look Up", "Look Down", "Look Left", "Look Right", "Reset Orientation"
                };

                for (const auto& action : bindOrder) {
                    ImGui::Text("%s:", action.c_str());
                    ImGui::SameLine(180.0f);
                    
                    std::string btnLabel = (m_inputManager.actionWaitingForKey == action) ? "[ PRESS ANY KEY ]" : "[ " + m_inputManager.getKeyName(m_inputManager.keybinds[action]) + " ]";
                    bool isCurrentlyWaiting = (m_inputManager.actionWaitingForKey == action);
                    
                    if (isCurrentlyWaiting) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button((btnLabel + "##" + action).c_str(), ImVec2(150, 0))) m_inputManager.actionWaitingForKey = action;
                    if (isCurrentlyWaiting) ImGui::PopStyleColor();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("AAA Pipeline")) {
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

            if (ImGui::BeginTabItem("Developer")) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Checkbox("Enable Developer Mode (Overlay)", &m_uiDevMode);
                
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));

                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Rasterizer Settings (Requires Rebuild):");
                ImGui::Checkbox("Enable Culling", &m_uiCullEnabled);
                
                if (!m_uiCullEnabled) ImGui::BeginDisabled();
                const char* cullModes[] = { "Back Face", "Front Face" };
                ImGui::Combo("Cull Mode", &m_uiCullMode, cullModes, IM_ARRAYSIZE(cullModes));
                if (!m_uiCullEnabled) ImGui::EndDisabled();

                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));

                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Initial Level Camera State:");
                float startPos[3] = { m_uiCamStartX, m_uiCamStartY, m_uiCamStartZ };
                if (ImGui::InputFloat3("Start Pos (X,Y,Z)", startPos)) {
                    m_uiCamStartX = startPos[0]; m_uiCamStartY = startPos[1]; m_uiCamStartZ = startPos[2];
                }
                
                float startRot[3] = { m_uiCamStartPitch, m_uiCamStartYaw, m_uiCamStartRoll };
                if (ImGui::InputFloat3("Start Rot (P,Y,R)", startRot)) {
                    m_uiCamStartPitch = startRot[0]; m_uiCamStartYaw = startRot[1]; m_uiCamStartRoll = startRot[2];
                }

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

            // Delegate to InputManager
            m_inputManager.saveBindsToConfig(m_config);

            m_config.setBool("hw_raytracing", m_uiHwRaytracing);
            m_config.setBool("mesh_shaders", m_uiMeshShaders);
            m_config.setBool("software_gi", m_uiSoftwareGI);
            m_config.setBool("vrs", m_uiVRS);
            
            m_config.setBool("dev_mode", m_uiDevMode);
            m_config.setBool("cull_enabled", m_uiCullEnabled);
            m_config.setInt("cull_mode", m_uiCullMode);
            
            m_config.setFloat("cam_start_x", m_uiCamStartX);
            m_config.setFloat("cam_start_y", m_uiCamStartY);
            m_config.setFloat("cam_start_z", m_uiCamStartZ);
            m_config.setFloat("cam_start_pitch", m_uiCamStartPitch);
            m_config.setFloat("cam_start_yaw", m_uiCamStartYaw);
            m_config.setFloat("cam_start_roll", m_uiCamStartRoll);

            m_config.save();
            m_renderer.rebuildGraphicsPipeline();
            
            int winWidth = 1920, winHeight = 1080;
            switch(m_uiResolutionIndex) {
                case 0: winWidth = 7680; winHeight = 4320; break; 
                case 1: winWidth = 3840; winHeight = 2160; break; 
                case 2: winWidth = 2560; winHeight = 1440; break; 
                case 3: winWidth = 1920; winHeight = 1080; break; 
                case 4: winWidth = 1600; winHeight = 900;  break; 
                case 5: winWidth = 1366; winHeight = 768;  break; 
                case 6: winWidth = 1280; winHeight = 720;  break; 
                case 7: winWidth = 854;  winHeight = 480;  break; 
                case 8: winWidth = 640;  winHeight = 360;  break; 
                case 9: winWidth = 426;  winHeight = 240;  break; 
                case 10: winWidth = 256; winHeight = 144;  break; 
            }
            m_window.applyDisplaySettings(m_uiFullscreen, winWidth, winHeight);
            m_state = EngineState::MainMenu;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(150, 40)) || m_inputManager.wasEscapeJustPressed()) {
            m_state = EngineState::MainMenu;
            m_inputManager.actionWaitingForKey = ""; 
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
            for (const auto& entity : m_activeEntities) m_modelLoader.loadModel(entity.modelPath);
            
            glm::vec3 startPos(m_uiCamStartX, m_uiCamStartY, m_uiCamStartZ);
            glm::vec3 startRot(m_uiCamStartPitch, m_uiCamStartYaw, m_uiCamStartRoll);
            m_camera.setInitialState(startPos, startRot);
            
            m_state = EngineState::Playing;
            m_inputManager.setMouseCapture(true);

        } catch (const std::exception& e) {
            std::cerr << "[Warning]: " << e.what() << "\n";
            m_state = EngineState::WorldSelect;
        }
    }

    void GameManager::handlePlaying(float dt) {
        // --- NEW: CLEAN DELEGATED INPUT MAPPING ---
        glm::vec2 mouseDelta = m_inputManager.getMouseDelta();
        float deltaX = mouseDelta.x * m_camera.mouseSensitivity;
        float deltaY = mouseDelta.y * m_camera.mouseSensitivity;
        
        if (m_inputManager.isActionPressed("Look Left"))  deltaX -= m_camera.keyboardLookSensitivity * dt;
        if (m_inputManager.isActionPressed("Look Right")) deltaX += m_camera.keyboardLookSensitivity * dt;
        if (m_inputManager.isActionPressed("Look Up"))    deltaY -= m_camera.keyboardLookSensitivity * dt;
        if (m_inputManager.isActionPressed("Look Down"))  deltaY += m_camera.keyboardLookSensitivity * dt;

        float roll = 0.0f;
        if (m_inputManager.isActionPressed("Roll CW")) roll += 1.0f; 
        if (m_inputManager.isActionPressed("Roll CCW")) roll -= 1.0f; 

        glm::vec3 move(0.0f);
        if (m_inputManager.isActionPressed("Forward")) move.z += 1.0f; 
        if (m_inputManager.isActionPressed("Backward")) move.z -= 1.0f; 
        if (m_inputManager.isActionPressed("Left")) move.x -= 1.0f; 
        if (m_inputManager.isActionPressed("Right")) move.x += 1.0f; 
        if (m_inputManager.isActionPressed("Up (Ascend)")) move.y += 1.0f; 
        if (m_inputManager.isActionPressed("Down (Descend)")) move.y -= 1.0f; 

        if (m_inputManager.isActionPressed("Reset Orientation")) m_camera.resetOrientation();

        glm::vec3 look(deltaY, deltaX, 0.0f);
        m_camera.update(dt, move, look, roll);

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(0,1,0,1), "GAMEPLAY ACTIVE (6DOF Camera)");
        ImGui::Text("Entities in RAM: %zu", m_activeEntities.size());
        
        if (m_uiDevMode) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::TextColored(ImVec4(1,1,0,1), "--- DEVELOPER MODE ---");
            ImGui::Text("FPS: %.1f (Expected Target: %d)", ImGui::GetIO().Framerate, m_uiTargetFps);
            
            glm::vec3 pos = m_camera.getPosition();
            glm::vec3 rot = m_camera.getEulerAngles();
            ImGui::Text("Camera Pos: [ X: %.2f,  Y: %.2f,  Z: %.2f ]", pos.x, pos.y, pos.z);
            ImGui::Text("Camera Rot: [ P: %.2f,  Y: %.2f,  R: %.2f ]", rot.x, rot.y, rot.z);
        }

        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::Text("Press ESCAPE to pause");
        ImGui::End();

        if (m_inputManager.wasEscapeJustPressed()) {
            m_state = EngineState::MainMenu;
            m_inputManager.setMouseCapture(false);
        }
    }

} // namespace Engine::Game