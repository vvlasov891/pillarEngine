#pragma once
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Pillar {

class Scene;
class ScriptComponent;

class ScriptEngine {
public:
    using Factory = std::function<ScriptComponent*()>;

    // Called by PILLAR_REGISTER_SCRIPT macro
    static void RegisterScript(const std::string& name, Factory factory);

    // Scene lifecycle
    static void OnSceneStart(Scene& scene);
    static void OnUpdate(Scene& scene, float dt);
    static void OnSceneStop(Scene& scene);

    // Get registered script names (for editor UI)
    static std::vector<std::string> GetRegisteredNames();

    // Create by name
    static std::unique_ptr<ScriptComponent> CreateScript(const std::string& name);

private:
    static std::unordered_map<std::string, Factory>& Registry();

    // Per-entity live instances: entityID -> list of scripts
    static std::unordered_map<uint32_t,
           std::vector<std::unique_ptr<ScriptComponent>>> s_Instances;
};

} // namespace Pillar
