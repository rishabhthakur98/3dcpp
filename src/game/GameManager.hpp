#pragma once

#include "../core/Window.hpp"
#include "../core/Config.hpp"
#include "../graphics/Renderer.hpp"
#include "../graphics/ModelLoader.hpp"
#include "../scene/SceneLoader.hpp"
#include "../input/InputManager.hpp"
#include "../ui/GuiManager.hpp"
#include "Camera.hpp"
#include <string>
#include <vector>

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
        
        Scene::SceneLoader m_sceneLoader; 
        Input::InputManager m_inputManager;
        UI::GuiManager m_guiManager; // UI entirely delegated
        
        Camera m_camera;
        std::vector<Scene::SceneEntity> m_activeEntities; 

        EngineState m_state;

        void handlePlaying(float dt); 
        void loadWorld(const std::string& worldName);
    };

} // namespace Engine::Game