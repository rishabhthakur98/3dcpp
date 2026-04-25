#pragma once

#include "../core/Window.hpp"
#include "../core/Config.hpp"
#include "../graphics/Renderer.hpp"
#include "../graphics/ModelLoader.hpp"
#include "SceneLoader.hpp"
#include <string>
#include <vector>

namespace Engine::Game {

    // Defines the strict, mutually exclusive states the engine can be in
    enum class EngineState { MainMenu, WorldSelect, Settings, Playing, QuitRequested };

    class GameManager {
    public:
        // GameManager needs the config to save settings, and the renderer to check GPU specs
        GameManager(Core::Window& window, Core::Config& config, Graphics::Renderer& renderer, Graphics::ModelLoader& modelLoader);
        ~GameManager() = default;

        // Delete copy operations to maintain strict ownership of the state
        GameManager(const GameManager&) = delete;
        GameManager& operator=(const GameManager&) = delete;

        // Called once per frame to handle logic specific to the current state
        void update();

        // Allows the main loop to know if the game should shut down
        bool shouldQuit() const;

    private:
        Core::Window& m_window;
        Core::Config& m_config;
        Graphics::Renderer& m_renderer;
        Graphics::ModelLoader& m_modelLoader;
        SceneLoader m_sceneLoader; // Handles reading the blueprint files
        
        // Holds the live objects currently active in our world
        std::vector<SceneEntity> m_activeEntities; 

        EngineState m_state;
        int m_menuIndex;
        
        // Input debouncing flags
        bool m_upPressed, m_downPressed, m_enterPressed, m_escapePressed;

        // --- UI State Variables for Settings ---
        bool m_uiFullscreen;
        int m_uiResolutionIndex;
        bool m_uiLimitFrames;
        int m_uiTargetFps;
        float m_uiContrast;
        
        // AAA Hardware Feature Toggles
        bool m_uiHwRaytracing;
        bool m_uiMeshShaders; // <-- THE FIX: Added missing declaration
        bool m_uiSoftwareGI;  // <-- THE FIX: Added missing declaration
        bool m_uiVRS;         // <-- THE FIX: Added missing declaration

        // State Handlers
        void handleMainMenu();
        void handleWorldSelect();
        void handleSettings();
        void handlePlaying();
        
        // Dynamic file loading and UI drawing
        void loadWorld(const std::string& worldName);
        void pollInput();
        void drawGuiMenu(const std::string& title, const std::vector<std::string>& options);
    };

} // namespace Engine::Game