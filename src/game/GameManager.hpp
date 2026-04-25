#pragma once

#include "../core/Window.hpp"
#include "../core/Config.hpp"
#include "../graphics/Renderer.hpp"
#include "../graphics/ModelLoader.hpp"
#include "SceneLoader.hpp"
#include "Camera.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace Engine::Game {

    enum class EngineState { MainMenu, WorldSelect, Settings, Playing, QuitRequested };

    class GameManager {
    public:
        GameManager(Core::Window& window, Core::Config& config, Graphics::Renderer& renderer, Graphics::ModelLoader& modelLoader);
        ~GameManager() = default;

        GameManager(const GameManager&) = delete;
        GameManager& operator=(const GameManager&) = delete;

        void update(float dt);
        bool shouldQuit() const;

    private:
        Core::Window& m_window;
        Core::Config& m_config;
        Graphics::Renderer& m_renderer;
        Graphics::ModelLoader& m_modelLoader;
        SceneLoader m_sceneLoader; 
        Camera m_camera;
        
        std::vector<SceneEntity> m_activeEntities; 

        EngineState m_state;
        int m_menuIndex;
        bool m_escapePressed;
        
        double m_lastMouseX, m_lastMouseY;
        bool m_firstMouse;

        std::unordered_map<std::string, int> m_keybinds;
        std::string m_actionWaitingForKey; 
        std::string getKeyName(int key) const;
        void processKeybindCapture(); 

        bool m_uiFullscreen;
        int m_uiResolutionIndex;
        bool m_uiLimitFrames;
        int m_uiTargetFps;
        float m_uiContrast;
        
        bool m_uiHwRaytracing;
        bool m_uiMeshShaders; 
        bool m_uiSoftwareGI;  
        bool m_uiVRS;         

        bool m_uiDevMode;
        
        // --- NEW: Intuitive UI Culling Variables ---
        bool m_uiCullEnabled;
        int m_uiCullMode; 
        
        float m_uiCamStartX, m_uiCamStartY, m_uiCamStartZ;
        float m_uiCamStartPitch, m_uiCamStartYaw, m_uiCamStartRoll;

        void handleMainMenu();
        void handleWorldSelect();
        void handleSettings();
        void handlePlaying(float dt); 
        
        void loadWorld(const std::string& worldName);
        void pollInput();
    };

} // namespace Engine::Game