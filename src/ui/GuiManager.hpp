#pragma once

#include "../core/Window.hpp"
#include "../core/Config.hpp"
#include "../input/InputManager.hpp"
#include "../graphics/Renderer.hpp"
#include "../game/Camera.hpp"
#include <string>
#include <functional>

namespace Engine::UI {

    class GuiManager {
    public:
        GuiManager(Core::Window& window, Core::Config& config, Input::InputManager& inputManager, Graphics::Renderer& renderer);
        ~GuiManager() = default;

        void renderMainMenu(std::function<void()> onPlay, std::function<void()> onSettings, std::function<void()> onQuit);
        void renderWorldSelect(std::function<void(std::string)> onWorldLoad, std::function<void()> onBack);
        void renderLoadingScreen(float progress); // --- NEW: Loading Screen ---
        void renderSettings(Game::Camera& camera, std::function<void()> onBack);
        void renderGameplayOverlay(Game::Camera& camera, size_t activeEntitiesCount, bool isDevMode);

    private:
        Core::Window& m_window;
        Core::Config& m_config;
        Input::InputManager& m_inputManager;
        Graphics::Renderer& m_renderer;

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
        bool m_uiCullEnabled;
        int m_uiCullMode; 
        
        // --- NEW: Displacement Mapping ---
        bool m_uiDisplacement;
        float m_uiDisplacementScale;
        
        float m_uiCamStartX, m_uiCamStartY, m_uiCamStartZ;
        float m_uiCamStartPitch, m_uiCamStartYaw, m_uiCamStartRoll;

        void loadConfigToUI(Game::Camera& camera);
        void saveUIToConfig(Game::Camera& camera);
        void applyDisplayChanges();
    };

} // namespace Engine::UI