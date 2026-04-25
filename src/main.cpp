#include "core/Window.hpp"
#include "core/Config.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/ModelLoader.hpp"
#include "game/GameManager.hpp"

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <filesystem> 

int main() {
    try {
        if (std::filesystem::exists("../engine.cfg")) {
            std::filesystem::current_path("..");
        }

        Engine::Core::Config config("engine.cfg");
        
        bool isFullscreen = config.getBool("fullscreen", false);
        int resIndex = config.getInt("resolution_index", 3);
        
        int winWidth = 1920, winHeight = 1080;
        switch(resIndex) {
            case 0: winWidth = 7680; winHeight = 4320; break; 
            case 1: winWidth = 3840; winHeight = 2160; break; 
            case 2: winWidth = 2560; winHeight = 1440; break; 
            case 3: winWidth = 1920; winHeight = 1080; break; 
            case 4: winWidth = 1366; winHeight = 768;  break; 
            case 5: winWidth = 1280; winHeight = 720;  break; 
        }

        Engine::Core::Window window(winWidth, winHeight, isFullscreen, "3D C++ Engine (AAA Bindless)");
        
        // --- THE FIX: Pass config into the Renderer ---
        Engine::Graphics::Renderer renderer(window, config);
        Engine::Graphics::ModelLoader modelLoader;
        
        Engine::Game::GameManager gameManager(window, config, renderer, modelLoader);

        while (!window.shouldClose() && !gameManager.shouldQuit()) {
            
            auto frameStart = std::chrono::high_resolution_clock::now();
            window.pollEvents();

            renderer.beginUI();
            gameManager.update();
            renderer.drawFrame();

            if (config.getBool("limit_frames", true)) {
                int targetFps = config.getInt("target_fps", 60);
                std::chrono::duration<double, std::milli> targetFrameTime(1000.0 / targetFps);
                
                auto frameEnd = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> timeTaken = frameEnd - frameStart;
                
                if (timeTaken < targetFrameTime) {
                    std::this_thread::sleep_for(targetFrameTime - timeTaken);
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "\n[Engine Fatal Error]: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "\n[Engine Fatal Error]: An unknown error occurred.\n";
        return EXIT_FAILURE;
    }

    std::cout << "Engine shutting down safely.\n";
    return EXIT_SUCCESS;
}