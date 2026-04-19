#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// MeshRegistry — отдаёт кэшированные примитивы по имени.
// Используется при загрузке .pilevel:
//   meshPath = "__cube__"    → CreateCube()
//   meshPath = "__sphere__"  → CreateSphere()
//   meshPath = "__capsule__" → CreateCapsule()
//   meshPath = "__plane__"   → CreatePlane()
//   meshPath = "path/to/model.glb" → ModelLoader::LoadFirst()
// ─────────────────────────────────────────────────────────────────────────────
#include "Renderer.h"
#include "ModelLoader.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace Pillar {

class MeshRegistry {
public:
    static std::shared_ptr<GPUMesh> Get(const std::string& path) {
        if (path.empty()) return nullptr;
        auto it = s_Cache.find(path);
        if (it != s_Cache.end()) return it->second;

        std::shared_ptr<GPUMesh> mesh;
        if      (path == "__cube__")    mesh = std::make_shared<GPUMesh>(Renderer::CreateCube());
        else if (path == "__sphere__")  mesh = std::make_shared<GPUMesh>(Renderer::CreateSphere());
        else if (path == "__capsule__") mesh = std::make_shared<GPUMesh>(Renderer::CreateCapsule());
        else if (path == "__plane__")   mesh = std::make_shared<GPUMesh>(Renderer::CreatePlane(10.0f, 4));
        else                            mesh = ModelLoader::LoadFirst(path);

        if (mesh) s_Cache[path] = mesh;
        return mesh;
    }

    static void Clear() { s_Cache.clear(); }

private:
    static std::unordered_map<std::string, std::shared_ptr<GPUMesh>> s_Cache;
};

} // namespace Pillar
