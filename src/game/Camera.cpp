#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Engine::Game {

    Camera::Camera() : m_position(0.0f, 0.0f, 0.0f), m_orientation(1.0f, 0.0f, 0.0f, 0.0f) {}

    void Camera::setInitialState(const glm::vec3& pos, const glm::vec3& eulerRot) {
        m_position = pos;
        // Convert intuitive config Euler angles into the internal Quaternion
        glm::vec3 rads(glm::radians(eulerRot.x), glm::radians(eulerRot.y), glm::radians(eulerRot.z));
        m_orientation = glm::quat(rads);
    }

    void Camera::resetOrientation() {
        m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    glm::vec3 Camera::getEulerAngles() const {
        return glm::vec3(
            glm::degrees(glm::pitch(m_orientation)),
            glm::degrees(glm::yaw(m_orientation)),
            glm::degrees(glm::roll(m_orientation))
        );
    }

    void Camera::update(float dt, const glm::vec3& moveInput, const glm::vec3& lookInput, float rollInput) {
        // 1. Convert internal Quaternion TO Euler for human-intuitive math
        float pitch = glm::degrees(glm::pitch(m_orientation));
        float yaw   = glm::degrees(glm::yaw(m_orientation));
        float roll  = glm::degrees(glm::roll(m_orientation));

        // Apply delta inputs
        pitch -= lookInput.x;
        yaw   -= lookInput.y;
        roll  += rollInput * rollSpeed * dt;

        // 2. Clamp pitch to prevent the camera from flipping upside down
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        // Keep yaw and roll within 360 bounds
        if (yaw > 360.0f) yaw -= 360.0f;
        if (yaw < -360.0f) yaw += 360.0f;
        if (roll > 360.0f) roll -= 360.0f;
        if (roll < -360.0f) roll += 360.0f;

        // 3. Convert FROM Euler back to Quaternion for internal storage
        glm::vec3 newEulerRads(glm::radians(pitch), glm::radians(yaw), glm::radians(roll));
        m_orientation = glm::quat(newEulerRads);

        // --- Apply Movement ---
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
        proj[1][1] *= -1; // Vulkan Y-flip

        return proj * view; 
    }

} // namespace Engine::Game