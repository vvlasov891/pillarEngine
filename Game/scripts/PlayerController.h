#pragma once
#include <Scripting/ScriptComponent.h>
#include <Scene/Components.h>
#include <Core/Input.h>
#include <Renderer/Camera.h>

namespace Pillar {

// ─────────────────────────────────────────────────────────────────────────────
// PlayerController
//   WASD movement, mouse-look camera, speed = 8.
//   Attach to any entity that has a TransformComponent + CameraComponent.
// ─────────────────────────────────────────────────────────────────────────────
class PlayerController : public ScriptComponent {
public:
    float MoveSpeed    = 8.0f;
    float MouseSensitivity = 0.12f;
    float JumpForce    = 5.0f;

    void OnStart()  override;
    void OnUpdate(float dt) override;

private:
    float  m_Yaw      = -90.0f;
    float  m_Pitch    =   0.0f;
    bool   m_Grounded = true;
    Camera m_Cam;
};

} // namespace Pillar
