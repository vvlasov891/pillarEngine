#pragma once
#include <Scripting/ScriptComponent.h>
#include <Scene/Components.h>
#include <glm/glm.hpp>

namespace Pillar {

// ─────────────────────────────────────────────────────────────────────────────
// EnemyAI
//   Chases the player (entity named "Player") at speed 6.
//   On catch (distance < catchRadius) — does nothing (extend as needed).
// ─────────────────────────────────────────────────────────────────────────────
class EnemyAI : public ScriptComponent {
public:
    float MoveSpeed   = 6.0f;
    float CatchRadius = 1.2f;
    float RotateSpeed = 5.0f;   // degrees/s smoothing

    void OnStart()  override;
    void OnUpdate(float dt) override;

private:
    // Cached player transform pointer (refreshed each frame if null)
    TransformComponent* m_PlayerTransform = nullptr;
    float               m_CurrentYaw      = 0.0f;

    TransformComponent* FindPlayer();
};

} // namespace Pillar
