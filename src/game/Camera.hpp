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

        void setInitialState(const glm::vec3& pos, const glm::vec3& eulerRot);
        void resetOrientation();

        glm::vec3 getPosition() const { return m_position; }
        
        // Safely extracts Euler angles from the internal Quaternion for the UI
        glm::vec3 getEulerAngles() const;

    private:
        glm::vec3 m_position;
        
        // AAA Standard: Rotation is strictly stored in memory as a Quaternion
        glm::quat m_orientation; 
    };

} // namespace Engine::Game