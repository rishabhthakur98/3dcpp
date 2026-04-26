#include "GameManager.hpp"
#include <iostream>

namespace Engine::Game {

    GameManager::GameManager(Core::Window& window, Core::Config& config, Graphics::Renderer& renderer, Graphics::ModelLoader& modelLoader)
        : m_window(window), m_config(config), m_renderer(renderer), m_modelLoader(modelLoader), 
          m_inputManager(window, config), m_guiManager(window, config, m_inputManager, renderer),
          m_state(EngineState::MainMenu) {
        
        m_camera.mouseSensitivity = m_config.getFloat("mouse_sensitivity", 0.1f);
        m_camera.keyboardLookSensitivity = m_config.getFloat("keyboard_look_sensitivity", 90.0f);
        m_camera.moveSpeed = m_config.getFloat("move_speed", 5.0f);
        m_camera.rollSpeed = m_config.getFloat("roll_speed", 60.0f);
        
        // Load FOV parameters directly into the camera on boot!
        m_camera.fov = m_config.getFloat("cam_fov", 60.0f);
        m_camera.nearPlane = m_config.getFloat("cam_near", 0.1f);
        m_camera.farPlane = m_config.getFloat("cam_far", 1000.0f);
    }

    bool GameManager::shouldQuit() const { return m_state == EngineState::QuitRequested; }

    void GameManager::update(float dt) {
        // --- DELEGATED UI ROUTING ---
        // We use lambda functions to handle state changes triggered from inside the UI module
        switch (m_state) {
            case EngineState::MainMenu:
                m_guiManager.renderMainMenu(
                    [&]() { m_state = EngineState::WorldSelect; }, // onPlay
                    [&]() { m_state = EngineState::Settings; },    // onSettings
                    [&]() { m_state = EngineState::QuitRequested; m_window.closeWindow(); } // onQuit
                );
                break;
            case EngineState::WorldSelect:
                m_guiManager.renderWorldSelect(
                    [&](std::string name) { loadWorld(name); },    // onWorldLoad
                    [&]() { m_state = EngineState::MainMenu; }     // onBack
                );
                break;
            case EngineState::Settings:
                m_guiManager.renderSettings(m_camera, [&]() { m_state = EngineState::MainMenu; });
                break;
            case EngineState::Playing:
                handlePlaying(dt);
                break;
            case EngineState::QuitRequested: 
                break;
        }

        // Draw the Vulkan Frame and pass all our dynamically loaded entities over to the GPU
        glm::mat4 viewProj = m_camera.getViewProjection(static_cast<float>(m_window.getWidth()), static_cast<float>(m_window.getHeight()));
        
        // Pass the explicit camera position so the shader has perfect Vector math for PBR and reflections!
        m_renderer.drawFrame(viewProj, m_camera.getPosition(), m_activeEntities, m_modelLoader);
    }

    void GameManager::loadWorld(const std::string& worldName) {
        // Safely free all existing GPU memory before loading new assets into RAM
        m_renderer.freeUploadedModels();
        m_activeEntities.clear();
        m_modelLoader.clearCache();
        
        std::string dir = m_config.getString("worlds_dir", "assets/worlds");
        std::string filepath = dir + "/" + worldName + "/level.scene";
        
        try {
            // SceneLoader parses the raw file into structs
            m_activeEntities = m_sceneLoader.loadScene(filepath);
            
            // Loop through the parsed entities and instruct the loaders to unpack and upload them!
            for (const auto& entity : m_activeEntities) {
                // Loads from Hard Drive into CPU RAM
                auto model = m_modelLoader.loadModel(entity.modelPath);
                
                // Copies from CPU RAM across the PCIe bus into GPU VRAM
                m_renderer.uploadModel(model); 
            }
            
            // Set the camera based on configurations saved in engine.cfg
            glm::vec3 startPos(m_config.getFloat("cam_start_x", 0.0f), m_config.getFloat("cam_start_y", 0.0f), m_config.getFloat("cam_start_z", 0.0f));
            glm::vec3 startRot(m_config.getFloat("cam_start_pitch", 0.0f), m_config.getFloat("cam_start_yaw", 0.0f), m_config.getFloat("cam_start_roll", 0.0f));
            m_camera.setInitialState(startPos, startRot);
            
            m_state = EngineState::Playing;
            m_inputManager.setMouseCapture(true);

        } catch (const std::exception& e) {
            std::cerr << "[Warning]: " << e.what() << "\n";
            m_state = EngineState::WorldSelect;
        }
    }

    void GameManager::handlePlaying(float dt) {
        // Fetch raw hardware inputs cleanly from the InputManager
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

        // Tell the isolated UI Manager to draw the developer overlay
        bool isDevMode = m_config.getBool("dev_mode", true);
        m_guiManager.renderGameplayOverlay(m_camera, m_activeEntities.size(), isDevMode);

        if (m_inputManager.wasEscapeJustPressed()) {
            m_state = EngineState::MainMenu;
            m_inputManager.setMouseCapture(false);
        }
    }

} // namespace Engine::Game