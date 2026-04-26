#include "GameManager.hpp"
#include <iostream>
#include <chrono>

namespace Engine::Game {

    GameManager::GameManager(Core::Window& window, Core::Config& config, Graphics::Renderer& renderer, Graphics::ModelLoader& modelLoader)
        : m_window(window), m_config(config), m_renderer(renderer), m_modelLoader(modelLoader), 
          m_inputManager(window, config), m_guiManager(window, config, m_inputManager, renderer),
          m_state(EngineState::MainMenu) {
        
        m_camera.mouseSensitivity = m_config.getFloat("mouse_sensitivity", 0.1f);
        m_camera.keyboardLookSensitivity = m_config.getFloat("keyboard_look_sensitivity", 90.0f);
        m_camera.moveSpeed = m_config.getFloat("move_speed", 5.0f);
        m_camera.rollSpeed = m_config.getFloat("roll_speed", 60.0f);
        
        m_camera.fov = m_config.getFloat("cam_fov", 60.0f);
        m_camera.nearPlane = m_config.getFloat("cam_near", 0.1f);
        m_camera.farPlane = m_config.getFloat("cam_far", 1000.0f);
    }

    bool GameManager::shouldQuit() const { return m_state == EngineState::QuitRequested; }

    void GameManager::update(float dt) {
        switch (m_state) {
            case EngineState::MainMenu:
                m_guiManager.renderMainMenu(
                    [&]() { m_state = EngineState::WorldSelect; }, 
                    [&]() { m_state = EngineState::Settings; },    
                    [&]() { m_state = EngineState::QuitRequested; m_window.closeWindow(); } 
                );
                break;
            case EngineState::WorldSelect:
                m_guiManager.renderWorldSelect(
                    [&](std::string name) { loadWorld(name); },    
                    [&]() { m_state = EngineState::MainMenu; }     
                );
                break;
            case EngineState::LoadingWorld: {
                // Calculate progress to send to the UI
                float progress = m_loadTotal > 0 ? static_cast<float>(m_loadProgress) / static_cast<float>(m_loadTotal) : 0.0f;
                m_guiManager.renderLoadingScreen(progress);

                // Non-blocking check to see if the background thread finished
                if (m_loadingFuture.valid() && m_loadingFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    try {
                        m_activeEntities = m_loadingFuture.get(); // Catch the results
                        
                        // Main Thread: Push the pre-loaded RAM data into GPU VRAM
                        for (const auto& entity : m_activeEntities) {
                            auto model = m_modelLoader.loadModel(entity.modelPath); // Instant fetch from RAM cache
                            m_renderer.uploadModel(model); 
                        }
                        
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
                break;
            }
            case EngineState::Settings:
                m_guiManager.renderSettings(m_camera, [&]() { m_state = EngineState::MainMenu; });
                break;
            case EngineState::Playing:
                handlePlaying(dt);
                break;
            case EngineState::QuitRequested: 
                break;
        }

        glm::mat4 viewProj = m_camera.getViewProjection(static_cast<float>(m_window.getWidth()), static_cast<float>(m_window.getHeight()));
        m_renderer.drawFrame(viewProj, m_camera.getPosition(), m_activeEntities, m_modelLoader);
    }

    void GameManager::loadWorld(const std::string& worldName) {
        m_renderer.freeUploadedModels();
        m_activeEntities.clear();
        m_modelLoader.clearCache();
        
        std::string dir = m_config.getString("worlds_dir", "assets/worlds");
        std::string filepath = dir + "/" + worldName + "/level.scene";
        
        m_state = EngineState::LoadingWorld;
        m_loadProgress = 0;
        m_loadTotal = 1; // Prevent divide by zero

        // --- ASYNC DISK LOADING ---
        // Fire off a background thread so the UI keeps rendering at 60+ FPS
        m_loadingFuture = std::async(std::launch::async, [this, filepath]() {
            auto entities = m_sceneLoader.loadScene(filepath);
            m_loadTotal = static_cast<int>(entities.size());
            
            for (const auto& entity : entities) {
                // Extracts from Hard Drive to CPU RAM (Slowest part)
                m_modelLoader.loadModel(entity.modelPath);
                m_loadProgress++;
            }
            return entities;
        });
    }

    void GameManager::handlePlaying(float dt) {
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

        bool isDevMode = m_config.getBool("dev_mode", true);
        m_guiManager.renderGameplayOverlay(m_camera, m_activeEntities.size(), isDevMode);

        if (m_inputManager.wasEscapeJustPressed()) {
            m_state = EngineState::MainMenu;
            m_inputManager.setMouseCapture(false);
        }
    }

} // namespace Engine::Game