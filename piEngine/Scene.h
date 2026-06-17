#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Pillar {

enum class CameraMode { Perspective, Orthographic };

class Camera {
public:
    Camera() = default;
    Camera(float fov, float nearPlane, float farPlane);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    // Position / orientation
    glm::vec3 GetPosition()  const { return m_Position; }
    glm::vec3 GetFront()     const { return m_Front; }
    glm::vec3 GetRight()     const { return m_Right; }
    glm::vec3 GetUp()        const { return m_Up; }
    float     GetYaw()       const { return m_Yaw; }
    float     GetPitch()     const { return m_Pitch; }

    void SetPosition(const glm::vec3& pos) { m_Position = pos; }
    void SetRotation(float yaw, float pitch);
    void SetFOV(float fov) { m_FOV = fov; }
    void SetNearFar(float n, float f) { m_Near = n; m_Far = f; }

    // Editor fly-cam
    void ProcessKeyboard(float dt, bool forward, bool back, bool left, bool right,
                         bool up, bool down, float speed = 5.0f);
    void ProcessMouseMovement(float dx, float dy, float sensitivity = 0.1f);
    void ProcessMouseScroll(float offset);

    CameraMode Mode = CameraMode::Perspective;
    float OrthoSize = 10.0f;

private:
    void UpdateVectors();

    glm::vec3 m_Position = {0, 2, 5};
    glm::vec3 m_Front    = {0, 0,-1};
    glm::vec3 m_Right    = {1, 0, 0};
    glm::vec3 m_Up       = {0, 1, 0};

    float m_Yaw   = -90.0f;
    float m_Pitch =   0.0f;
    float m_FOV   =  60.0f;
    float m_Near  =   0.1f;
    float m_Far   = 1000.0f;
};

} // namespace Pillar
