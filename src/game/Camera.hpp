#pragma once

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
        
        // --- NEW: Camera Lens Parameters ---
        float fov = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;

        void setInitialState(const glm::vec3& pos, const glm::vec3& eulerRot);
        void resetOrientation();

        glm::vec3 getPosition() const { return m_position; }
        glm::vec3 getEulerAngles() const;

    private:
        glm::vec3 m_position;
        glm::quat m_orientation; 
        
        float m_pitch;
        float m_yaw;
        float m_roll;
    };

} // namespace Engine::Game