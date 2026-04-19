#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Scripting/ScriptEngine.h"
#include "Audio/AudioSystem.h"
#include "Core/Log.h"
#include "Renderer/MeshRegistry.h"
#include "Renderer/Renderer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

using json = nlohmann::json;

namespace Pillar {

// ── JSON helpers ──────────────────────────────────────────────────────────────
static json Vec3ToJson(const glm::vec3& v) { return {v.x, v.y, v.z}; }
static glm::vec3 JsonToVec3(const json& j) {
    return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
}

// ── Scene ─────────────────────────────────────────────────────────────────────
Scene::Scene() { m_Name = "Untitled"; }
Scene::~Scene() { OnStop(); }

Entity Scene::CreateEntity(const std::string& name) {
    return CreateEntityWithUUID(UUID(), name);
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name) {
    entt::entity handle = m_Registry.create();
    Entity e(handle, this);
    auto& tag = e.AddComponent<TagComponent>(name);
    tag.ID    = uuid;
    auto& tc  = e.AddComponent<TransformComponent>();
    (void)tc;
    m_UUIDMap[uuid] = handle;
    return e;
}

void Scene::DestroyEntity(Entity entity) {
    auto& tag = entity.GetComponent<TagComponent>();
    m_UUIDMap.erase(tag.ID);
    m_Registry.destroy(entity.GetHandle());
}

Entity Scene::FindEntityByName(const std::string& name) {
    auto view = m_Registry.view<TagComponent>();
    for (auto e : view)
        if (view.get<TagComponent>(e).Name == name)
            return Entity(e, this);
    return Entity();
}

Entity Scene::FindEntityByUUID(UUID uuid) {
    auto it = m_UUIDMap.find(uuid);
    if (it != m_UUIDMap.end()) return Entity(it->second, this);
    return Entity();
}

Camera* Scene::GetActiveCamera() {
    if (m_EditorMode && m_EditorCamera)
        return m_EditorCamera;
    // Find entity with CameraComponent marked IsMain
    auto view = m_Registry.view<CameraComponent, TransformComponent>();
    for (auto e : view) {
        auto& cc = view.get<CameraComponent>(e);
        if (cc.IsMain) {
            // Return or create a transient Camera from component data
            // For simplicity we cache one per entity
            static Camera s_RuntimeCam;
            auto& tc = view.get<TransformComponent>(e);
            s_RuntimeCam.SetPosition(tc.Position);
            s_RuntimeCam.SetFOV(cc.FOV);
            s_RuntimeCam.SetNearFar(cc.Near, cc.Far);
            return &s_RuntimeCam;
        }
    }
    // Fallback default camera
    static Camera s_Default;
    return &s_Default;
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────
void Scene::OnStart() {
    m_Running = true;
    // Audio: play-on-start
    auto audioView = m_Registry.view<AudioSourceComponent>();
    for (auto e : audioView) {
        auto& as = audioView.get<AudioSourceComponent>(e);
        if (as.PlayOnStart && !as.ClipPath.empty())
            AudioSystem::Play(as.ClipPath, as.Volume, as.Loop);
    }
    // Scripts: OnStart
    ScriptEngine::OnSceneStart(*this);
}

void Scene::OnUpdate(float dt) {
    if (!m_Running) return;
    UpdateScripts(dt);
    UpdatePhysics(dt);
}

void Scene::OnStop() {
    if (!m_Running) return;
    m_Running = false;
    ScriptEngine::OnSceneStop(*this);
}

void Scene::UpdateScripts(float dt) {
    ScriptEngine::OnUpdate(*this, dt);
}

void Scene::UpdatePhysics(float dt) {
    // Simple gravity + velocity integration (no full physics engine)
    auto view = m_Registry.view<RigidbodyComponent, TransformComponent>();
    for (auto e : view) {
        auto& rb = view.get<RigidbodyComponent>(e);
        auto& tc = view.get<TransformComponent>(e);
        if (rb.IsKinematic) continue;
        if (rb.UseGravity) rb.Velocity.y -= 9.81f * dt;
        rb.Velocity    *= (1.0f - rb.Drag * dt);
        tc.Position    += rb.Velocity * dt;
        // Simple floor
        if (tc.Position.y < 0.0f) { tc.Position.y = 0.0f; rb.Velocity.y = 0.0f; }
    }
}

// ── Serialization ─────────────────────────────────────────────────────────────
bool Scene::SaveToFile(const std::string& path) {
    json root;
    root["name"]    = m_Name;
    root["version"] = 1;
    json entities   = json::array();

    m_Registry.each([&](auto e) {
        Entity ent(e, this);
        json jEnt;

        if (ent.HasComponent<TagComponent>()) {
            auto& t = ent.GetComponent<TagComponent>();
            jEnt["tag"]    = t.Name;
            jEnt["uuid"]   = (uint64_t)t.ID;
            jEnt["active"] = t.Active;
        }
        if (ent.HasComponent<TransformComponent>()) {
            auto& tc = ent.GetComponent<TransformComponent>();
            jEnt["transform"]["position"] = Vec3ToJson(tc.Position);
            jEnt["transform"]["rotation"] = Vec3ToJson(tc.Rotation);
            jEnt["transform"]["scale"]    = Vec3ToJson(tc.Scale);
        }
        if (ent.HasComponent<MeshRendererComponent>()) {
            auto& mr = ent.GetComponent<MeshRendererComponent>();
            jEnt["meshRenderer"]["meshPath"]    = mr.MeshPath;
            jEnt["meshRenderer"]["texturePath"] = mr.TexturePath;
            jEnt["meshRenderer"]["color"]       = Vec3ToJson(mr.Color);
        }
        if (ent.HasComponent<DirectionalLightComponent>()) {
            auto& dl = ent.GetComponent<DirectionalLightComponent>();
            jEnt["dirLight"]["color"]     = Vec3ToJson(dl.Color);
            jEnt["dirLight"]["intensity"] = dl.Intensity;
        }
        if (ent.HasComponent<PointLightComponent>()) {
            auto& pl = ent.GetComponent<PointLightComponent>();
            jEnt["pointLight"]["color"]     = Vec3ToJson(pl.Color);
            jEnt["pointLight"]["intensity"] = pl.Intensity;
            jEnt["pointLight"]["range"]     = pl.Range;
        }
        if (ent.HasComponent<CameraComponent>()) {
            auto& cam = ent.GetComponent<CameraComponent>();
            jEnt["camera"]["fov"]    = cam.FOV;
            jEnt["camera"]["near"]   = cam.Near;
            jEnt["camera"]["far"]    = cam.Far;
            jEnt["camera"]["isMain"] = cam.IsMain;
        }
        if (ent.HasComponent<RigidbodyComponent>()) {
            auto& rb = ent.GetComponent<RigidbodyComponent>();
            jEnt["rigidbody"]["mass"]        = rb.Mass;
            jEnt["rigidbody"]["useGravity"]  = rb.UseGravity;
            jEnt["rigidbody"]["isKinematic"] = rb.IsKinematic;
            jEnt["rigidbody"]["drag"]        = rb.Drag;
        }
        if (ent.HasComponent<BoxColliderComponent>()) {
            auto& bc = ent.GetComponent<BoxColliderComponent>();
            jEnt["boxCollider"]["center"]    = Vec3ToJson(bc.Center);
            jEnt["boxCollider"]["halfExt"]   = Vec3ToJson(bc.HalfExt);
            jEnt["boxCollider"]["isTrigger"] = bc.IsTrigger;
        }
        if (ent.HasComponent<AudioSourceComponent>()) {
            auto& as = ent.GetComponent<AudioSourceComponent>();
            jEnt["audioSource"]["clip"]       = as.ClipPath;
            jEnt["audioSource"]["volume"]     = as.Volume;
            jEnt["audioSource"]["loop"]       = as.Loop;
            jEnt["audioSource"]["playOnStart"]= as.PlayOnStart;
        }
        if (ent.HasComponent<ScriptContainerComponent>()) {
            auto& sc = ent.GetComponent<ScriptContainerComponent>();
            jEnt["scripts"] = sc.ScriptNames;
        }
        entities.push_back(jEnt);
    });

    root["entities"] = entities;
    std::ofstream f(path);
    if (!f.is_open()) { PL_ERROR("Cannot write scene: {}", path); return false; }
    f << root.dump(4);
    m_FilePath = path;
    PL_INFO("Scene saved: {}", path);
    return true;
}

bool Scene::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { PL_ERROR("Cannot open scene: {}", path); return false; }
    json root;
    try { f >> root; } catch (json::exception& e) {
        PL_ERROR("Scene parse error: {}", e.what()); return false;
    }

    m_Registry.clear();
    m_UUIDMap.clear();
    m_Name     = root.value("name", "Untitled");
    m_FilePath = path;

    for (auto& jEnt : root["entities"]) {
        UUID  uuid(jEnt.value("uuid", (uint64_t)0));
        std::string tag = jEnt.value("tag", "Entity");
        Entity ent = CreateEntityWithUUID(uuid, tag);
        auto& tagC = ent.GetComponent<TagComponent>();
        tagC.Active = jEnt.value("active", true);

        if (jEnt.contains("transform")) {
            auto& tc = ent.GetComponent<TransformComponent>();
            tc.Position = JsonToVec3(jEnt["transform"]["position"]);
            tc.Rotation = JsonToVec3(jEnt["transform"]["rotation"]);
            tc.Scale    = JsonToVec3(jEnt["transform"]["scale"]);
        }
        if (jEnt.contains("meshRenderer")) {
            auto& mr = ent.AddComponent<MeshRendererComponent>();
            mr.MeshPath    = jEnt["meshRenderer"].value("meshPath","");
            mr.TexturePath = jEnt["meshRenderer"].value("texturePath","");
            mr.Color       = JsonToVec3(jEnt["meshRenderer"]["color"]);
            // Resolve mesh
            if (!mr.MeshPath.empty())
                mr.Mesh = MeshRegistry::Get(mr.MeshPath);
            // Resolve texture
            if (!mr.TexturePath.empty())
                mr.TextureID = Renderer::LoadTexture(mr.TexturePath);
        }
        if (jEnt.contains("dirLight")) {
            auto& dl = ent.AddComponent<DirectionalLightComponent>();
            dl.Color     = JsonToVec3(jEnt["dirLight"]["color"]);
            dl.Intensity = jEnt["dirLight"].value("intensity", 1.0f);
        }
        if (jEnt.contains("pointLight")) {
            auto& pl = ent.AddComponent<PointLightComponent>();
            pl.Color     = JsonToVec3(jEnt["pointLight"]["color"]);
            pl.Intensity = jEnt["pointLight"].value("intensity", 1.0f);
            pl.Range     = jEnt["pointLight"].value("range", 10.0f);
        }
        if (jEnt.contains("camera")) {
            auto& cam = ent.AddComponent<CameraComponent>();
            cam.FOV    = jEnt["camera"].value("fov",  60.0f);
            cam.Near   = jEnt["camera"].value("near",  0.1f);
            cam.Far    = jEnt["camera"].value("far", 1000.0f);
            cam.IsMain = jEnt["camera"].value("isMain", false);
        }
        if (jEnt.contains("rigidbody")) {
            auto& rb = ent.AddComponent<RigidbodyComponent>();
            rb.Mass        = jEnt["rigidbody"].value("mass", 1.0f);
            rb.UseGravity  = jEnt["rigidbody"].value("useGravity", true);
            rb.IsKinematic = jEnt["rigidbody"].value("isKinematic", false);
            rb.Drag        = jEnt["rigidbody"].value("drag", 0.1f);
        }
        if (jEnt.contains("boxCollider")) {
            auto& bc = ent.AddComponent<BoxColliderComponent>();
            bc.Center    = JsonToVec3(jEnt["boxCollider"]["center"]);
            bc.HalfExt   = JsonToVec3(jEnt["boxCollider"]["halfExt"]);
            bc.IsTrigger = jEnt["boxCollider"].value("isTrigger", false);
        }
        if (jEnt.contains("audioSource")) {
            auto& as = ent.AddComponent<AudioSourceComponent>();
            as.ClipPath    = jEnt["audioSource"].value("clip","");
            as.Volume      = jEnt["audioSource"].value("volume",1.0f);
            as.Loop        = jEnt["audioSource"].value("loop",false);
            as.PlayOnStart = jEnt["audioSource"].value("playOnStart",false);
        }
        if (jEnt.contains("scripts")) {
            auto& sc = ent.AddComponent<ScriptContainerComponent>();
            sc.ScriptNames = jEnt["scripts"].get<std::vector<std::string>>();
        }
    }

    PL_INFO("Scene loaded: {} ({} entities)", path, (int)m_UUIDMap.size());
    return true;
}

} // namespace Pillar
