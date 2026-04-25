#include "GuiManager.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>

namespace Engine::UI {

    GuiManager::GuiManager(Core::Window& window, Core::Config& config, Input::InputManager& inputManager, Graphics::Renderer& renderer)
        : m_window(window), m_config(config), m_inputManager(inputManager), m_renderer(renderer) {}

    void GuiManager::loadConfigToUI(Game::Camera& camera) {
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
        
        // Load FOV parameters
        camera.fov = m_config.getFloat("cam_fov", 60.0f);
        camera.nearPlane = m_config.getFloat("cam_near", 0.1f);
        camera.farPlane = m_config.getFloat("cam_far", 1000.0f);
    }

    void GuiManager::saveUIToConfig(Game::Camera& camera) {
        m_config.setBool("fullscreen", m_uiFullscreen);
        m_config.setInt("resolution_index", m_uiResolutionIndex);
        m_config.setBool("limit_frames", m_uiLimitFrames);
        m_config.setInt("target_fps", m_uiTargetFps);
        m_config.setFloat("contrast", m_uiContrast);
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
        
        // Save FOV parameters
        m_config.setFloat("cam_fov", camera.fov);
        m_config.setFloat("cam_near", camera.nearPlane);
        m_config.setFloat("cam_far", camera.farPlane);
        
        m_config.save();
    }

    void GuiManager::applyDisplayChanges() {
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
        m_renderer.rebuildGraphicsPipeline();
    }

    void GuiManager::renderMainMenu(std::function<void()> onPlay, std::function<void()> onSettings, std::function<void()> onQuit) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        
        ImGui::Begin("MainMenu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Dummy(ImVec2(0.0f, 100.0f)); ImGui::Indent(100.0f); 
        ImGui::Text("=== 3D C++ ENGINE ==="); ImGui::Dummy(ImVec2(0.0f, 30.0f));

        if (ImGui::Button("Load Game", ImVec2(250, 50))) onPlay();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("Settings", ImVec2(250, 50))) onSettings();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("Quit Game", ImVec2(250, 50))) onQuit();
        
        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    void GuiManager::renderWorldSelect(std::function<void(std::string)> onWorldLoad, std::function<void()> onBack) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        ImGui::Begin("WorldSelect", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Dummy(ImVec2(0.0f, 100.0f)); ImGui::Indent(100.0f); 
        ImGui::Text("=== SELECT WORLD ==="); ImGui::Dummy(ImVec2(0.0f, 20.0f));

        std::string worldsDir = m_config.getString("worlds_dir", "assets/worlds");
        bool foundWorlds = false;

        if (std::filesystem::exists(worldsDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(worldsDir)) {
                if (entry.is_directory() && std::filesystem::exists(entry.path() / "level.scene")) {
                    foundWorlds = true;
                    std::string worldName = entry.path().filename().string();
                    if (ImGui::Button(worldName.c_str(), ImVec2(300, 50))) onWorldLoad(worldName);
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                }
            }
        }
        if (!foundWorlds) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No valid worlds found in: %s", worldsDir.c_str());
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
        }

        ImGui::Separator(); ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("Back", ImVec2(150, 40)) || m_inputManager.wasEscapeJustPressed()) onBack(); 

        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    void GuiManager::renderSettings(Game::Camera& camera, std::function<void()> onBack) {
        static bool loaded = false;
        if (!loaded) { loadConfigToUI(camera); loaded = true; }

        if (!m_inputManager.actionWaitingForKey.empty()) m_inputManager.processKeybindCapture();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        ImGui::Begin("SettingsMenu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::Dummy(ImVec2(0.0f, 50.0f)); ImGui::Indent(50.0f);
        ImGui::Text("=== SETTINGS ==="); ImGui::Dummy(ImVec2(0.0f, 20.0f));

        if (ImGui::BeginTabBar("SettingsTabs")) {
            
            if (ImGui::BeginTabItem("Display")) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Checkbox("Fullscreen Mode", &m_uiFullscreen);
                const char* resolutions[] = { 
                    "7680x4320 (8K)", "3840x2160 (4K)", "2560x1440 (1440p)", "1920x1080 (1080p)", 
                    "1600x900 (900p)", "1366x768 (768p)", "1280x720 (720p)", "854x480 (480p)", 
                    "640x360 (360p)", "426x240 (240p)", "256x144 (144p)" 
                };
                ImGui::Combo("Resolution", &m_uiResolutionIndex, resolutions, IM_ARRAYSIZE(resolutions));
                ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::Separator(); ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Checkbox("Limit Framerate", &m_uiLimitFrames);
                if (m_uiLimitFrames) ImGui::SliderInt("Target FPS", &m_uiTargetFps, 30, 240);
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::SliderFloat("Contrast", &m_uiContrast, 0.5f, 2.0f);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Controls")) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Sensitivities:");
                ImGui::SliderFloat("Mouse Sensitivity", &camera.mouseSensitivity, 0.01f, 1.0f);
                ImGui::SliderFloat("Keyboard Look", &camera.keyboardLookSensitivity, 10.0f, 200.0f);
                ImGui::SliderFloat("Movement Speed", &camera.moveSpeed, 1.0f, 50.0f);
                ImGui::SliderFloat("Roll Speed", &camera.rollSpeed, 10.0f, 180.0f);
                
                ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::Separator(); ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Key Bindings (Click to rebind):");
                ImGui::Dummy(ImVec2(0.0f, 5.0f));

                std::vector<std::string> bindOrder = {
                    "Forward", "Backward", "Left", "Right", "Up (Ascend)", "Down (Descend)", 
                    "Roll CW", "Roll CCW", "Look Up", "Look Down", "Look Left", "Look Right", "Reset Orientation"
                };

                for (const auto& action : bindOrder) {
                    ImGui::Text("%s:", action.c_str()); ImGui::SameLine(180.0f);
                    std::string btnLabel = (m_inputManager.actionWaitingForKey == action) ? "[ PRESS ANY KEY ]" : "[ " + m_inputManager.getKeyName(m_inputManager.keybinds[action]) + " ]";
                    bool isCurrentlyWaiting = (m_inputManager.actionWaitingForKey == action);
                    
                    if (isCurrentlyWaiting) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button((btnLabel + "##" + action).c_str(), ImVec2(150, 0))) m_inputManager.actionWaitingForKey = action;
                    if (isCurrentlyWaiting) ImGui::PopStyleColor();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Developer")) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::Checkbox("Enable Developer Mode (Overlay)", &m_uiDevMode);
                ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::Separator(); ImGui::Dummy(ImVec2(0.0f, 10.0f));

                // --- THE NEW CAMERA LENS SLIDERS ---
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Camera Lens Settings:");
                ImGui::SliderFloat("Field of View (FOV)", &camera.fov, 30.0f, 120.0f);
                ImGui::SliderFloat("Near Clip Plane", &camera.nearPlane, 0.01f, 10.0f);
                ImGui::SliderFloat("Far Clip Plane", &camera.farPlane, 100.0f, 10000.0f);

                ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::Separator(); ImGui::Dummy(ImVec2(0.0f, 10.0f));

                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Rasterizer Settings (Requires Rebuild):");
                ImGui::Checkbox("Enable Culling", &m_uiCullEnabled);
                if (!m_uiCullEnabled) ImGui::BeginDisabled();
                const char* cullModes[] = { "Back Face", "Front Face" };
                ImGui::Combo("Cull Mode", &m_uiCullMode, cullModes, IM_ARRAYSIZE(cullModes));
                if (!m_uiCullEnabled) ImGui::EndDisabled();

                ImGui::Dummy(ImVec2(0.0f, 10.0f)); ImGui::Separator(); ImGui::Dummy(ImVec2(0.0f, 10.0f));

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
            saveUIToConfig(camera);
            applyDisplayChanges();
            loaded = false; 
            onBack();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(150, 40)) || m_inputManager.wasEscapeJustPressed()) {
            loaded = false;
            m_inputManager.actionWaitingForKey = ""; 
            onBack();
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    void GuiManager::renderGameplayOverlay(Game::Camera& camera, size_t activeEntitiesCount, bool isDevMode) {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(0,1,0,1), "GAMEPLAY ACTIVE (6DOF Camera)");
        ImGui::Text("Entities Extracted to RAM: %zu", activeEntitiesCount);
        
        if (isDevMode) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::TextColored(ImVec4(1,1,0,1), "--- DEVELOPER MODE ---");
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            
            glm::vec3 pos = camera.getPosition();
            glm::vec3 rot = camera.getEulerAngles();
            ImGui::Text("Camera Pos: [ X: %.2f,  Y: %.2f,  Z: %.2f ]", pos.x, pos.y, pos.z);
            ImGui::Text("Camera Rot: [ P: %.2f,  Y: %.2f,  R: %.2f ]", rot.x, rot.y, rot.z);
            
            // --- NEW: FOV Tracking in Overlay ---
            ImGui::Text("Lens FOV: %.1f", camera.fov);
        }

        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::Text("Press ESCAPE to pause");
        ImGui::End();
    }

} // namespace Engine::UI