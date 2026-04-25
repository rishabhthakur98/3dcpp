#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Engine::Game {

    Camera::Camera() : m_position(0.0f), m_orientation(1.0f, 0.0f, 0.0f, 0.0f), m_pitch(0.0f), m_yaw(0.0f), m_roll(0.0f) {}

    void Camera::setInitialState(const glm::vec3& pos, const glm::vec3& eulerRot) {
        m_position = pos;
        m_pitch = eulerRot.x;
        m_yaw = eulerRot.y;
        m_roll = eulerRot.z;
    }

    void Camera::resetOrientation() {
        m_pitch = 0.0f;
        m_yaw = 0.0f;
        m_roll = 0.0f;
    }

    glm::vec3 Camera::getEulerAngles() const {
        return glm::vec3(m_pitch, m_yaw, m_roll);
    }

    void Camera::update(float dt, const glm::vec3& moveInput, const glm::vec3& lookInput, float rollInput) {
        // 1. Update explicit Euler angles directly from user input
        m_pitch -= lookInput.x;
        m_yaw   -= lookInput.y;
        m_roll  += rollInput * rollSpeed * dt;

        // 2. Clamp pitch to prevent the camera from doing backflips
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        // Keep yaw and roll within clean 360-degree bounds
        if (m_yaw > 180.0f) m_yaw -= 360.0f;
        if (m_yaw < -180.0f) m_yaw += 360.0f;
        if (m_roll > 180.0f) m_roll -= 360.0f;
        if (m_roll < -180.0f) m_roll += 360.0f;

        // 3. Build a brand new, clean Quaternion from scratch every frame
        // This guarantees NO gimbal lock and NO accidental roll induction
        glm::quat qPitch = glm::angleAxis(glm::radians(m_pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat qYaw   = glm::angleAxis(glm::radians(m_yaw),   glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat qRoll  = glm::angleAxis(glm::radians(m_roll),  glm::vec3(0.0f, 0.0f, 1.0f));
        
        m_orientation = qYaw * qPitch * qRoll;

        // 4. Apply Movement
        glm::vec3 right   = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 up      = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 forward = m_orientation * glm::vec3(0.0f, 0.0f, -1.0f);

        glm::vec3 velocity = (forward * moveInput.z) + (right * moveInput.x) + (up * moveInput.y);
        if (glm::length(velocity) > 0.0f) {
            velocity = glm::normalize(velocity) * moveSpeed * dt;
        }
        m_position += velocity;
    }

    glm::mat4 Camera::getViewProjection(float aspectWidth, float aspectHeight) const {
        glm::vec3 forward = m_orientation * glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up      = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(m_position, m_position + forward, up);

        float aspect = aspectWidth / aspectHeight;
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 1000.0f);
        
        // Vulkan's Y-axis is inverted compared to OpenGL
        proj[1][1] *= -1;

        return proj * view; 
    }

} // namespace Engine::Game