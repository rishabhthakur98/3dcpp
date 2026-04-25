#include "GameManager.hpp"
#include <iostream>
#include <filesystem>
#include <imgui.h> 

namespace Engine::Game {

    GameManager::GameManager(Core::Window& window, Core::Config& config, Graphics::Renderer& renderer, Graphics::ModelLoader& modelLoader)
        : m_window(window), m_config(config), m_renderer(renderer), m_modelLoader(modelLoader), 
          m_state(EngineState::MainMenu), m_menuIndex(0), 
          m_upPressed(false), m_downPressed(false), m_enterPressed(false), m_escapePressed(false) {
        
        m_uiFullscreen = m_config.getBool("fullscreen", false);
        m_uiResolutionIndex = m_config.getInt("resolution_index", 3); 
        m_uiLimitFrames = m_config.getBool("limit_frames", true);
        m_uiTargetFps = m_config.getInt("target_fps", 60);
        m_uiContrast = m_config.getFloat("contrast", 1.0f);
        
        m_uiHwRaytracing = m_config.getBool("hw_raytracing", false);
        m_uiMeshShaders = m_config.getBool("mesh_shaders", false);
        m_uiSoftwareGI = m_config.getBool("software_gi", true);
        m_uiVRS = m_config.getBool("vrs", false);
    }

    bool GameManager::shouldQuit() const { return m_state == EngineState::QuitRequested; }

    void GameManager::pollInput() {
        GLFWwindow* rawWindow = m_window.getNativeWindow();
        bool escape = (glfwGetKey(rawWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS);

        static bool lastEscape = false;
        m_escapePressed = escape && !lastEscape;
        lastEscape = escape;
    }

    void GameManager::update() {
        pollInput();

        // --- THE FIX: ImGui Stack Integrity ---
        // We track whether we pushed styles THIS frame using a boolean.
        // This guarantees we always pop them, even if the state changes halfway through the logic.
        bool stylesPushed = false;

        if (m_state != EngineState::Playing) {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            stylesPushed = true;
        }

        // Route the logic
        switch (m_state) {
            case EngineState::MainMenu: handleMainMenu(); break;
            case EngineState::WorldSelect: handleWorldSelect(); break;
            case EngineState::Settings: handleSettings(); break;
            case EngineState::Playing: handlePlaying(); break;
            case EngineState::QuitRequested: break;
        }

        // Safely pop styles if they were applied at the start of this frame
        if (stylesPushed) {
            ImGui::PopStyleColor(2);
        }
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
                if (entry.is_directory()) {
                    if (std::filesystem::exists(entry.path() / "level.scene")) {
                        foundWorlds = true;
                        std::string worldName = entry.path().filename().string();
                        
                        if (ImGui::Button(worldName.c_str(), ImVec2(300, 50))) {
                            // --- THE FIX: State Override Bug ---
                            // We now ONLY call loadWorld here. We let loadWorld() decide
                            // if it successfully changes the state to EngineState::Playing.
                            loadWorld(worldName);
                        }
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    }
                }
            }
        }

        if (!foundWorlds) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No valid worlds found in: %s", worldsDir.c_str());
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
        }

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (ImGui::Button("Back", ImVec2(150, 40)) || m_escapePressed) { 
            m_state = EngineState::MainMenu; 
        }

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
        }

        ImGui::End();
    }

    void GameManager::loadWorld(const std::string& worldName) {
        m_activeEntities.clear();
        m_modelLoader.clearCache();
        
        std::string dir = m_config.getString("worlds_dir", "assets/worlds");
        std::string filepath = dir + "/" + worldName + "/level.scene";
        
        std::cout << "\nLoading world blueprint: " << filepath << "...\n";
        
        try {
            m_activeEntities = m_sceneLoader.loadScene(filepath);
            for (const auto& entity : m_activeEntities) {
                m_modelLoader.loadModel(entity.modelPath);
            }
            std::cout << "World successfully loaded!\n";
            
            // --- THE FIX: State Change on Success ---
            // We ONLY transition to the gameplay state if the entire parsing
            // and model loading process finishes without throwing an error!
            m_state = EngineState::Playing;

        } catch (const std::exception& e) {
            std::cerr << "[Warning]: " << e.what() << "\n";
            m_state = EngineState::WorldSelect;
        }
    }

    void GameManager::handlePlaying() {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(0,1,0,1), "GAMEPLAY ACTIVE");
        ImGui::Text("Entities Loaded: %zu", m_activeEntities.size());
        ImGui::Text("Press ESCAPE to pause");
        ImGui::End();

        if (m_escapePressed) {
            m_state = EngineState::MainMenu;
        }
    }

} // namespace Engine::Game