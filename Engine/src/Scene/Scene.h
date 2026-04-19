#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <entt/entt.hpp>
#include "Core/UUID.h"
#include "Renderer/Camera.h"

namespace Pillar {

class Entity;
struct TagComponent;

class Scene {
public:
    Scene();
    ~Scene();

    // Entity management
    Entity CreateEntity(const std::string& name = "Entity");
    Entity CreateEntityWithUUID(UUID uuid, const std::string& name = "Entity");
    void   DestroyEntity(Entity entity);
    Entity FindEntityByName(const std::string& name);
    Entity FindEntityByUUID(UUID uuid);

    // Lifecycle
    void OnStart();
    void OnUpdate(float dt);
    void OnStop();

    // Serialization (.pilevel)
    bool SaveToFile(const std::string& path);
    bool LoadFromFile(const std::string& path);

    // Accessors
    entt::registry& GetRegistry() { return m_Registry; }
    Camera*         GetActiveCamera();
    void            SetEditorCamera(Camera* cam) { m_EditorCamera = cam; }
    bool            IsEditorMode()  const { return m_EditorMode; }
    void            SetEditorMode(bool v)  { m_EditorMode = v; }
    const std::string& GetName()   const  { return m_Name; }
    void            SetName(const std::string& n) { m_Name = n; }
    const std::string& GetFilePath() const { return m_FilePath; }

    // Iterate all entities
    template<typename Fn>
    void EachEntity(Fn fn) {
        m_Registry.each([&](auto e) { fn(Entity(e, this)); });
    }

private:
    void UpdateScripts(float dt);
    void UpdatePhysics(float dt);
    void UpdateAudio();

    entt::registry                          m_Registry;
    std::unordered_map<UUID, entt::entity>  m_UUIDMap;
    std::string                             m_Name;
    std::string                             m_FilePath;
    bool                                    m_EditorMode = false;
    bool                                    m_Running    = false;
    Camera*                                 m_EditorCamera = nullptr;
};

} // namespace Pillar
