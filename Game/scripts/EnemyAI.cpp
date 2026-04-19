#include "EnemyAI.h"
#include <Core/Application.h>
#include <Core/Log.h>
#include <Scene/Scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Pillar {

PILLAR_REGISTER_SCRIPT(EnemyAI)

void EnemyAI::OnStart() {
    PL_INFO("EnemyAI started on '{}'", GetComponent<TagComponent>().Name);
    m_PlayerTransform = FindPlayer();
    auto& tc = GetComponent<TransformComponent>();
    m_CurrentYaw = tc.Rotation.y;
}

TransformComponent* EnemyAI::FindPlayer() {
    // Дёргаем напрямую из Application
    Scene* scene = Application::Get().GetActiveScene();
    if (!scene) return nullptr;
    Entity player = scene->FindEntityByName("Player");
    if (!player.IsValid()) {
        PL_WARN("EnemyAI: no entity named 'Player' found.");
        return nullptr;
    }
    return &player.GetComponent<TransformComponent>();
}

void EnemyAI::OnUpdate(float dt) {
    // Refresh player pointer every frame (safe — player might respawn)
    if (!m_PlayerTransform) m_PlayerTransform = FindPlayer();
    if (!m_PlayerTransform) return;

    auto& tc = GetComponent<TransformComponent>();

    // Direction to player (XZ plane only — enemies don't fly)
    glm::vec3 toPlayer   = m_PlayerTransform->Position - tc.Position;
    toPlayer.y = 0.0f;
    float distance = glm::length(toPlayer);

    // ── Caught? ──────────────────────────────────────────────────────────────
    if (distance < CatchRadius) {
        // Caught the player — extend here (damage, animation, etc.)
        // Currently: stop moving
        return;
    }

    // ── Move toward player ────────────────────────────────────────────────────
    glm::vec3 dir = glm::normalize(toPlayer);
    tc.Position  += dir * MoveSpeed * dt;

    // ── Smooth rotation to face player ────────────────────────────────────────
    float targetYaw = glm::degrees(std::atan2f(dir.x, dir.z));
    // Shortest-angle lerp
    float diff = targetYaw - m_CurrentYaw;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    m_CurrentYaw    += diff * std::min(RotateSpeed * dt, 1.0f);
    tc.Rotation.y    = m_CurrentYaw;
}

} // namespace Pillar
