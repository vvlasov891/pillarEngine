#pragma once
#include "Scene/Entity.h"
#include <string>

namespace Pillar {

// ─────────────────────────────────────────────────────────────────────────────
// Base class for all game scripts.
// Game code inherits from this and overrides OnStart / OnUpdate / OnDestroy.
//
// Usage example (PlayerController.h):
//
//   #include <PillarEngine.h>
//   class PlayerController : public Pillar::ScriptComponent {
//   public:
//       void OnStart()  override;
//       void OnUpdate(float dt) override;
//   };
//   PILLAR_REGISTER_SCRIPT(PlayerController)
// ─────────────────────────────────────────────────────────────────────────────
class ScriptComponent {
public:
    virtual ~ScriptComponent() = default;

    virtual void OnStart()          {}
    virtual void OnUpdate(float dt) { (void)dt; }
    virtual void OnDestroy()        {}
    virtual void OnCollisionEnter(Entity other) { (void)other; }
    virtual void OnTriggerEnter(Entity other)   { (void)other; }

    Entity GetEntity()  const { return m_Entity; }
    std::string GetName() const { return m_Name; }

    // Convenience
    template<typename T> T& GetComponent()       { return m_Entity.GetComponent<T>(); }
    template<typename T> bool HasComponent()      { return m_Entity.HasComponent<T>(); }

protected:
    Entity      m_Entity;
    std::string m_Name;

    friend class ScriptEngine;
};

// ── Registration macro ────────────────────────────────────────────────────────
// Put PILLAR_REGISTER_SCRIPT(ClassName) in the .cpp of your script.
// The ScriptEngine picks these up at startup.
#define PILLAR_REGISTER_SCRIPT(Type) \
    static bool _reg_##Type = []() -> bool { \
        ::Pillar::ScriptEngine::RegisterScript(#Type, []() -> ::Pillar::ScriptComponent* { \
            return new Type(); \
        }); \
        return true; \
    }();

} // namespace Pillar
