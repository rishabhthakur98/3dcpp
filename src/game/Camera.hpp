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
        glm::vec3 getEulerAngles() const;

    private:
        glm::vec3 m_position;
        glm::quat m_orientation; 
        
        // --- THE FIX: EXPLICIT EULER STORAGE ---
        // By storing these as explicit numbers, Roll can NEVER change 
        // unless you specifically trigger the rollInput variable.
        float m_pitch;
        float m_yaw;
        float m_roll;
    };

} // namespace Engine::Game