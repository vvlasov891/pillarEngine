#pragma once
#include "Renderer.h"
#include <string>
#include <memory>
#include <vector>

namespace Pillar {

struct LoadedModel {
    std::vector<std::shared_ptr<GPUMesh>> Meshes;
    bool Valid = false;
};

class ModelLoader {
public:
    // Loads OBJ / FBX / GLB / GLTF via Assimp.
    // Each Assimp mesh becomes one GPUMesh.
    static LoadedModel Load(const std::string& path);

    // Convenience: load and return first mesh only
    static std::shared_ptr<GPUMesh> LoadFirst(const std::string& path);
};

} // namespace Pillar
