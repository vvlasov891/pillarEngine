#include "PlayerController.h"
#include <Core/Application.h>
#include <Core/Log.h>
#include <Scene/Scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Pillar {

PILLAR_REGISTER_SCRIPT(PlayerController)

void PlayerController::OnStart() {
    PL_INFO("PlayerController started on '{}'",
            GetComponent<TagComponent>().Name);

    // Lock cursor for FPS look
    Input::SetCursorLocked(true);

    // Sync yaw/pitch from entity rotation if set
    auto& tc = GetComponent<TransformComponent>();
    m_Yaw   = tc.Rotation.y;
    m_Pitch = tc.Rotation.x;
}

void PlayerController::OnUpdate(float dt) {
    auto& tc = GetComponent<TransformComponent>();

    // ── Mouse look ────────────────────────────────────────────────────────────
    glm::vec2 delta = Input::GetMouseDelta();
    m_Yaw   += delta.x * MouseSensitivity;
    m_Pitch -= delta.y * MouseSensitivity;
    m_Pitch  = std::clamp(m_Pitch, -89.0f, 89.0f);

    // Store in transform so other systems / editor can read it
    tc.Rotation.x = m_Pitch;
    tc.Rotation.y = m_Yaw;

    // Compute forward / right from yaw only (no pitch for movement — FPS standard)
    float yawRad = glm::radians(m_Yaw);
    glm::vec3 forward = { cosf(yawRad), 0.0f, sinf(yawRad) };
    glm::vec3 right   = glm::normalize(glm::cross(forward, {0,1,0}));

    // ── WASD movement ─────────────────────────────────────────────────────────
    glm::vec3 move = {0,0,0};
    if (Input::IsKeyDown(PL_KEY_W)) move += forward;
    if (Input::IsKeyDown(PL_KEY_S)) move -= forward;
    if (Input::IsKeyDown(PL_KEY_D)) move += right;
    if (Input::IsKeyDown(PL_KEY_A)) move -= right;

    if (glm::length(move) > 0.001f)
        move = glm::normalize(move);

    tc.Position += move * MoveSpeed * dt;

    // ── Jump (Space) ──────────────────────────────────────────────────────────
    if (HasComponent<RigidbodyComponent>()) {
        auto& rb = GetComponent<RigidbodyComponent>();
        if (Input::IsKeyPressed(PL_KEY_SPACE) && tc.Position.y <= 0.05f) {
            rb.Velocity.y = JumpForce;
        }
    }

    // ── Push camera entity to follow player ───────────────────────────────────
    // We look for an entity named "MainCamera" and position it at player eye-height
    // (The Camera entity should have a CameraComponent with IsMain = true)
    Scene* scene = &Application::Get().GetActiveScene()[0];
    // Iterate safely: find camera
    auto& reg = scene->GetRegistry();
    auto camView = reg.view<CameraComponent, TransformComponent>();
    for (auto camEnt : camView) {
        auto& cc = reg.get<CameraComponent>(camEnt);
        if (!cc.IsMain) continue;
        auto& camTc = reg.get<TransformComponent>(camEnt);
        // Eye position = player pos + eye height
        camTc.Position = tc.Position + glm::vec3(0, 1.7f, 0);
        camTc.Rotation.x = m_Pitch;
        camTc.Rotation.y = m_Yaw;
        break;
    }

    // ── ESC to unlock cursor ──────────────────────────────────────────────────
    if (Input::IsKeyPressed(PL_KEY_ESCAPE))
        Input::SetCursorLocked(!Input::IsCursorLocked());
}

} // namespace Pillar
