#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>

namespace Pillar {

class Scene;
class Camera;
struct MeshData;
struct Material;

// ── Mesh handle (GPU side) ───────────────────────────────────────────────────
struct GPUMesh {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    unsigned int indexCount  = 0;
    unsigned int vertexCount = 0;
};

// ── Light ───────────────────────────────────────────────────────────────────
struct DirectionalLight {
    glm::vec3 direction = {-0.3f, -1.0f, -0.3f};
    glm::vec3 color     = {1.0f, 1.0f, 1.0f};
    float     intensity = 1.0f;
};

struct PointLight {
    glm::vec3 position  = {0,0,0};
    glm::vec3 color     = {1,1,1};
    float     intensity = 1.0f;
    float     range     = 10.0f;
};

// ── Renderer ────────────────────────────────────────────────────────────────
class Renderer {
public:
    static void Init(int width, int height);
    static void Shutdown();

    static void BeginFrame(int width, int height);
    static void RenderScene(Scene& scene);
    static void EndFrame();

    // Mesh creation helpers
    static GPUMesh CreateMeshFromData(const MeshData& data);
    static GPUMesh CreateCube(float size = 1.0f);
    static GPUMesh CreateSphere(float radius = 0.5f, int stacks = 16, int slices = 16);
    static GPUMesh CreateCapsule(float radius = 0.5f, float height = 2.0f, int segments = 16);
    static GPUMesh CreatePlane(float size = 10.0f, int subdivisions = 1);
    static void    DestroyMesh(GPUMesh& mesh);

    // Texture
    static unsigned int LoadTexture(const std::string& path);
    static unsigned int CreateWhiteTexture();

    // Stats
    static int GetDrawCalls()  { return s_DrawCalls; }
    static int GetTriangles()  { return s_Triangles; }

    // Framebuffer for editor viewport
    static unsigned int GetSceneFramebuffer()       { return s_SceneFBO; }
    static unsigned int GetSceneColorTexture()      { return s_SceneColorTex; }
    static void         ResizeSceneFramebuffer(int w, int h);

private:
    static void SetupFramebuffer(int w, int h);
    static void DrawMesh(const GPUMesh& mesh, const glm::mat4& transform,
                         unsigned int textureID, const glm::vec3& color,
                         const glm::vec3& camPos,
                         const DirectionalLight& dirLight,
                         const std::vector<PointLight>& pointLights);

    static unsigned int s_LitShader;
    static unsigned int s_GridShader;
    static unsigned int s_SceneFBO;
    static unsigned int s_SceneColorTex;
    static unsigned int s_SceneDepthRBO;
    static unsigned int s_WhiteTexture;
    static int          s_DrawCalls;
    static int          s_Triangles;
    static int          s_FBOWidth;
    static int          s_FBOHeight;
};

// ── Raw mesh data ─────────────────────────────────────────────────────────────
struct MeshData {
    std::vector<float>        vertices;  // pos(3) normal(3) uv(2) per vertex
    std::vector<unsigned int> indices;
};

} // namespace Pillar
