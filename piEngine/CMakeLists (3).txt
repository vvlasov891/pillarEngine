#include "ScriptEngine.h"
#include "ScriptComponent.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Core/Log.h"

namespace Pillar {

std::unordered_map<uint32_t, std::vector<std::unique_ptr<ScriptComponent>>>
    ScriptEngine::s_Instances;

std::unordered_map<std::string, ScriptEngine::Factory>& ScriptEngine::Registry() {
    static std::unordered_map<std::string, Factory> reg;
    return reg;
}

void ScriptEngine::RegisterScript(const std::string& name, Factory factory) {
    Registry()[name] = factory;
    PL_TRACE("ScriptEngine: registered '{}'", name);
}

std::vector<std::string> ScriptEngine::GetRegisteredNames() {
    std::vector<std::string> names;
    for (auto& [k, _] : Registry()) names.push_back(k);
    return names;
}

std::unique_ptr<ScriptComponent> ScriptEngine::CreateScript(const std::string& name) {
    auto it = Registry().find(name);
    if (it == Registry().end()) {
        PL_WARN("ScriptEngine: unknown script '{}'", name);
        return nullptr;
    }
    return std::unique_ptr<ScriptComponent>(it->second());
}

void ScriptEngine::OnSceneStart(Scene& scene) {
    s_Instances.clear();
    auto& reg = scene.GetRegistry();
    auto view  = reg.view<ScriptContainerComponent, TagComponent>();
    for (auto e : view) {
        auto& sc  = view.get<ScriptContainerComponent>(e);
        auto& tag = view.get<TagComponent>(e);
        uint32_t id = (uint32_t)(entt::entity)e;
        for (auto& scriptName : sc.ScriptNames) {
            auto inst = CreateScript(scriptName);
            if (!inst) continue;
            inst->m_Entity = Entity(e, &scene);
            inst->m_Name   = scriptName;
            inst->OnStart();
            s_Instances[id].push_back(std::move(inst));
        }
    }
}

void ScriptEngine::OnUpdate(Scene& scene, float dt) {
    for (auto& [id, scripts] : s_Instances)
        for (auto& s : scripts)
            s->OnUpdate(dt);
}

void ScriptEngine::OnSceneStop(Scene& scene) {
    for (auto& [id, scripts] : s_Instances)
        for (auto& s : scripts)
            s->OnDestroy();
    s_Instances.clear();
}

} // namespace Pillar
