#include "ModelLoader.h"
#include "Core/Log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>

namespace Pillar {

static std::shared_ptr<GPUMesh> ProcessMesh(aiMesh* mesh) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Position
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        // Normal
        if (mesh->HasNormals()) {
            vertices.push_back(mesh->mNormals[i].x);
            vertices.push_back(mesh->mNormals[i].y);
            vertices.push_back(mesh->mNormals[i].z);
        } else {
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
        }
        // UV
        if (mesh->mTextureCoords[0]) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0); vertices.push_back(0);
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    MeshData data;
    data.vertices = std::move(vertices);
    data.indices  = std::move(indices);
    return std::make_shared<GPUMesh>(Renderer::CreateMeshFromData(data));
}

static void ProcessNode(aiNode* node, const aiScene* scene,
                         std::vector<std::shared_ptr<GPUMesh>>& out)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        out.push_back(ProcessMesh(mesh));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ProcessNode(node->mChildren[i], scene, out);
}

LoadedModel ModelLoader::Load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate        |
        aiProcess_GenSmoothNormals   |
        aiProcess_FlipUVs            |
        aiProcess_CalcTangentSpace   |
        aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        PL_ERROR("ModelLoader: failed to load '{}': {}", path, importer.GetErrorString());
        return {};
    }

    LoadedModel result;
    result.Valid = true;
    ProcessNode(scene->mRootNode, scene, result.Meshes);
    PL_INFO("ModelLoader: loaded '{}' ({} meshes)", path, result.Meshes.size());
    return result;
}

std::shared_ptr<GPUMesh> ModelLoader::LoadFirst(const std::string& path) {
    auto model = Load(path);
    if (!model.Valid || model.Meshes.empty()) return nullptr;
    return model.Meshes[0];
}

} // namespace Pillar
