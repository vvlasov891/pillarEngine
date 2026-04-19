#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Pillar {

Camera::Camera(float fov, float nearPlane, float farPlane)
    : m_FOV(fov), m_Near(nearPlane), m_Far(farPlane) {
    UpdateVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    if (Mode == CameraMode::Perspective)
        return glm::perspective(glm::radians(m_FOV), aspect, m_Near, m_Far);
    float h = OrthoSize * 0.5f;
    return glm::ortho(-h * aspect, h * aspect, -h, h, m_Near, m_Far);
}

void Camera::SetRotation(float yaw, float pitch) {
    m_Yaw   = yaw;
    m_Pitch = std::clamp(pitch, -89.0f, 89.0f);
    UpdateVectors();
}

void Camera::ProcessKeyboard(float dt, bool forward, bool back, bool left,
                              bool right, bool up, bool down, float speed) {
    float vel = speed * dt;
    if (forward) m_Position += m_Front * vel;
    if (back)    m_Position -= m_Front * vel;
    if (left)    m_Position -= m_Right * vel;
    if (right)   m_Position += m_Right * vel;
    if (up)      m_Position += glm::vec3(0,1,0) * vel;
    if (down)    m_Position -= glm::vec3(0,1,0) * vel;
}

void Camera::ProcessMouseMovement(float dx, float dy, float sensitivity) {
    m_Yaw   += dx * sensitivity;
    m_Pitch -= dy * sensitivity;
    m_Pitch  = std::clamp(m_Pitch, -89.0f, 89.0f);
    UpdateVectors();
}

void Camera::ProcessMouseScroll(float offset) {
    m_FOV = std::clamp(m_FOV - offset * 2.0f, 10.0f, 120.0f);
}

void Camera::UpdateVectors() {
    glm::vec3 front;
    front.x = cosf(glm::radians(m_Yaw)) * cosf(glm::radians(m_Pitch));
    front.y = sinf(glm::radians(m_Pitch));
    front.z = sinf(glm::radians(m_Yaw)) * cosf(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, {0,1,0}));
    m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
}

} // namespace Pillar
