#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Engine::Scene {

    struct Transform {
        glm::vec3 position;
        glm::vec3 rotation; // Euler angles (Pitch, Yaw, Roll)
        glm::vec3 scale;
    };

    // --- NEW: Advanced Entity Properties ---
    struct EntityProperties {
        glm::vec3 tint = glm::vec3(1.0f);
        float mass = 0.0f; // 0.0 = static, >0.0 = dynamic physics
        bool visible = true;
        bool castShadows = true;
        std::string tag = "default";
    };

    struct SceneEntity {
        std::string modelPath;
        Transform transform;
        EntityProperties props;
    };

    class SceneLoader {
    public:
        SceneLoader() = default;
        ~SceneLoader() = default;

        std::vector<SceneEntity> loadScene(const std::string& filepath);

    private:
        std::string trim(const std::string& str) const;
        glm::vec3 parseVec3(const std::string& valueStr) const;
        bool parseBool(const std::string& valueStr) const;
        float parseFloat(const std::string& valueStr) const;
    };

} // namespace Engine::Scene