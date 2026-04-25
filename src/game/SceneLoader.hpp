#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Engine::Game {

    // Defines a 3D transformation in space
    struct Transform {
        glm::vec3 position;
        glm::vec3 rotation; // Pitch, Yaw, Roll in degrees
        glm::vec3 scale;
    };

    // Defines a single object in the game world
    struct SceneEntity {
        std::string modelPath;
        Transform transform;
    };

    // A zero-dependency parser for our custom .scene blueprint files
    class SceneLoader {
    public:
        SceneLoader() = default;
        ~SceneLoader() = default;

        // Parses the file and returns a list of all entities that need to be created
        std::vector<SceneEntity> loadScene(const std::string& filepath);

    private:
        // Helper functions for safe string parsing
        std::string trim(const std::string& str) const;
        glm::vec3 parseVec3(const std::string& valueStr) const;
    };

} // namespace Engine::Game