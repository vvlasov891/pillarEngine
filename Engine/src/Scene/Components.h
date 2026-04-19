#pragma once
#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Core/UUID.h"

namespace Pillar {

struct GPUMesh;
class ScriptComponent;

// ── Tag ───────────────────────────────────────────────────────────────────────
struct TagComponent {
    std::string Name = "Entity";
    UUID        ID;
    bool        Active = true;

    TagComponent() : ID() {}
    TagComponent(const std::string& name) : Name(name), ID() {}
};

// ── Transform ─────────────────────────────────────────────────────────────────
struct TransformComponent {
    glm::vec3 Position = {0, 0, 0};
    glm::vec3 Rotation = {0, 0, 0};  // Euler degrees
    glm::vec3 Scale    = {1, 1, 1};

    glm::mat4 GetMatrix() const;
};

// ── MeshRenderer ──────────────────────────────────────────────────────────────
struct MeshRendererComponent {
    std::shared_ptr<GPUMesh> Mesh;
    unsigned int             TextureID = 0;
    glm::vec3                Color     = {1, 1, 1};
    std::string              MeshPath;     // original path (for serialization)
    std::string              TexturePath;
    bool                     CastShadow    = true;
    bool                     ReceiveShadow = true;
};

// ── Lights ────────────────────────────────────────────────────────────────────
struct DirectionalLightComponent {
    glm::vec3 Color     = {1, 1, 1};
    float     Intensity = 1.0f;
};

struct PointLightComponent {
    glm::vec3 Color     = {1, 1, 1};
    float     Intensity = 1.0f;
    float     Range     = 10.0f;
};

// ── Camera ────────────────────────────────────────────────────────────────────
struct CameraComponent {
    float FOV    = 60.0f;
    float Near   = 0.1f;
    float Far    = 1000.0f;
    bool  IsMain = false;
};

// ── Rigidbody (simple, no full physics lib) ───────────────────────────────────
struct RigidbodyComponent {
    glm::vec3 Velocity     = {0, 0, 0};
    float     Mass         = 1.0f;
    bool      UseGravity   = true;
    bool      IsKinematic  = false;
    float     Drag         = 0.1f;
};

// ── Colliders ─────────────────────────────────────────────────────────────────
struct BoxColliderComponent {
    glm::vec3 Center  = {0, 0, 0};
    glm::vec3 HalfExt = {0.5f, 0.5f, 0.5f};
    bool      IsTrigger = false;
};

struct SphereColliderComponent {
    glm::vec3 Center  = {0, 0, 0};
    float     Radius  = 0.5f;
    bool      IsTrigger = false;
};

struct CapsuleColliderComponent {
    glm::vec3 Center  = {0, 0, 0};
    float     Radius  = 0.5f;
    float     Height  = 2.0f;
    bool      IsTrigger = false;
};

// ── Audio ─────────────────────────────────────────────────────────────────────
struct AudioSourceComponent {
    std::string  ClipPath;
    float        Volume    = 1.0f;
    float        Pitch     = 1.0f;
    bool         Loop      = false;
    bool         PlayOnStart = false;
    uint32_t     _handle   = 0;   // internal
};

// ── Script ────────────────────────────────────────────────────────────────────
struct ScriptContainerComponent {
    // Scripts are loaded as shared libs at runtime (game DLL)
    // or compiled-in for editor built-in scripts.
    std::vector<std::string> ScriptNames;  // e.g. "PlayerController"
};

} // namespace Pillar
