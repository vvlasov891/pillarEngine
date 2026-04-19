#include "MeshRegistry.h"
namespace Pillar {
    std::unordered_map<std::string, std::shared_ptr<GPUMesh>> MeshRegistry::s_Cache;
}
