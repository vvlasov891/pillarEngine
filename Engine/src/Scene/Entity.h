#pragma once
#include <entt/entt.hpp>
#include "Core/Log.h"

namespace Pillar {

class Scene;

class Entity {
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene) : m_Handle(handle), m_Scene(scene) {}

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args) {
        PL_ASSERT(!HasComponent<T>(), "Entity already has this component!");
        return GetRegistry().emplace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template<typename T>
    T& GetComponent() {
        PL_ASSERT(HasComponent<T>(), "Entity does not have this component!");
        return GetRegistry().get<T>(m_Handle);
    }

    template<typename T>
    bool HasComponent() {
        return GetRegistry().all_of<T>(m_Handle);
    }

    template<typename T>
    void RemoveComponent() {
        GetRegistry().remove<T>(m_Handle);
    }

    bool IsValid() const { return m_Handle != entt::null && m_Scene != nullptr; }
    entt::entity GetHandle() const { return m_Handle; }
    operator entt::entity() const  { return m_Handle; }
    operator bool() const          { return IsValid(); }

private:
    entt::registry& GetRegistry();

    entt::entity m_Handle = entt::null;
    Scene*       m_Scene  = nullptr;
};

} // namespace Pillar
