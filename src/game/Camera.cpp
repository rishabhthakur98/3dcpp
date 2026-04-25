#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine::Game {

    Camera::Camera() : m_position(0.0f, 0.0f, 0.0f), m_orientation(1.0f, 0.0f, 0.0f, 0.0f) {}

    void Camera::resetPosition() {
        m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    void Camera::update(float dt, const glm::vec3& moveInput, const glm::vec3& lookInput, float rollInput) {
        glm::vec3 right   = m_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 up      = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 forward = m_orientation * glm::vec3(0.0f, 0.0f, -1.0f);

        glm::vec3 velocity = (forward * moveInput.z) + (right * moveInput.x) + (up * moveInput.y);
        if (glm::length(velocity) > 0.0f) {
            velocity = glm::normalize(velocity) * moveSpeed * dt;
        }
        m_position += velocity;

        glm::quat yawQuat = glm::angleAxis(glm::radians(-lookInput.y), up);
        glm::quat pitchQuat = glm::angleAxis(glm::radians(-lookInput.x), right);
        glm::quat rollQuat = glm::angleAxis(glm::radians(rollInput * rollSpeed * dt), forward);

        m_orientation = yawQuat * pitchQuat * rollQuat * m_orientation;
        m_orientation = glm::normalize(m_orientation);
    }

    glm::mat4 Camera::getViewProjection(float aspectWidth, float aspectHeight) const {
        glm::vec3 forward = m_orientation * glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up      = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(m_position, m_position + forward, up);

        float aspect = aspectWidth / aspectHeight;
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 1000.0f);
        
        // Vulkan's Y-axis is inverted compared to OpenGL. We must flip the projection matrix.
        proj[1][1] *= -1;

        return proj * view; 
    }

} // namespace Engine::Game