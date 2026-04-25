#pragma once

// --- THE FIX: VULKAN DEPTH CLIPPING ---
// Forces GLM to use Vulkan's [0.0, 1.0] depth range instead of OpenGL's [-1.0, 1.0].
// Without this, objects near the camera plane can clip out of existence!
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine::Game {

    class Camera {
    public:
        Camera();
        ~Camera() = default;

        glm::mat4 getViewProjection(float aspectWidth, float aspectHeight) const;

        void update(float dt, const glm::vec3& moveInput, const glm::vec3& lookInput, float rollInput);
        
        float moveSpeed = 5.0f;
        float mouseSensitivity = 0.1f;
        float keyboardLookSensitivity = 90.0f;
        float rollSpeed = 60.0f;

        void resetPosition();

    private:
        glm::vec3 m_position;
        glm::quat m_orientation; 
    };

} // namespace Engine::Game