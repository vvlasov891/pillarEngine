#include "Renderer.h"
#include "Camera.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Core/Log.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>
#include <sstream>
#include <cmath>

namespace Pillar {

unsigned int Renderer::s_LitShader      = 0;
unsigned int Renderer::s_GridShader     = 0;
unsigned int Renderer::s_SceneFBO       = 0;
unsigned int Renderer::s_SceneColorTex  = 0;
unsigned int Renderer::s_SceneDepthRBO  = 0;
unsigned int Renderer::s_WhiteTexture   = 0;
int          Renderer::s_DrawCalls      = 0;
int          Renderer::s_Triangles      = 0;
int          Renderer::s_FBOWidth       = 0;
int          Renderer::s_FBOHeight      = 0;

// ── Shader helpers ────────────────────────────────────────────────────────────
static unsigned int CompileShader(GLenum type, const char* src) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024]; glGetShaderInfoLog(id, 1024, nullptr, buf);
        PL_ERROR("Shader compile error: {}", buf);
    }
    return id;
}

static unsigned int LinkProgram(unsigned int vs, unsigned int fs) {
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    int ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[1024]; glGetProgramInfoLog(prog, 1024, nullptr, buf);
        PL_ERROR("Program link error: {}", buf);
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

static std::string ReadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { PL_ERROR("Cannot open shader: {}", path); return ""; }
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

static unsigned int LoadShaderFromFiles(const std::string& vertPath,
                                        const std::string& fragPath) {
    std::string vs = ReadFile(vertPath);
    std::string fs = ReadFile(fragPath);
    return LinkProgram(CompileShader(GL_VERTEX_SHADER,   vs.c_str()),
                       CompileShader(GL_FRAGMENT_SHADER, fs.c_str()));
}

// Inline shader strings (fallback if files not found)
static const char* k_LitVert = R"GLSL(
#version 450 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
uniform mat4 uModel; uniform mat4 uView; uniform mat4 uProjection;
out vec3 FragPos; out vec3 Normal; out vec2 TexCoord;
void main() {
    vec4 wp = uModel * vec4(aPos,1.0);
    FragPos  = wp.xyz;
    Normal   = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aUV;
    gl_Position = uProjection * uView * wp;
})GLSL";

static const char* k_LitFrag = R"GLSL(
#version 450 core
#define MAX_PL 8
in vec3 FragPos; in vec3 Normal; in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D uAlbedoMap;
uniform vec3  uColor;
uniform float uShininess;
uniform vec3  uCamPos;
uniform vec3  uDirLightDir; uniform vec3 uDirLightColor; uniform float uDirLightIntensity;
uniform int   uNumPointLights;
uniform vec3  uPointLightPos[MAX_PL]; uniform vec3 uPointLightColor[MAX_PL];
uniform float uPointLightIntensity[MAX_PL]; uniform float uPointLightRange[MAX_PL];
vec3 Dir(vec3 n, vec3 v, vec3 a){
    vec3 l=normalize(-uDirLightDir);
    float d=max(dot(n,l),0.0);
    vec3 h=normalize(l+v);
    float s=pow(max(dot(n,h),0.0),uShininess);
    return 0.15*uDirLightColor*a + d*uDirLightColor*a*uDirLightIntensity + s*uDirLightColor*0.3*uDirLightIntensity;
}
vec3 Pt(int i,vec3 n,vec3 v,vec3 a){
    vec3 tl=uPointLightPos[i]-FragPos; float dist=length(tl);
    if(dist>uPointLightRange[i]) return vec3(0);
    vec3 l=normalize(tl); float at=1.0/(1.0+0.09*dist+0.032*dist*dist);
    float d=max(dot(n,l),0.0); vec3 h=normalize(l+v);
    float s=pow(max(dot(n,h),0.0),uShininess);
    return (d*uPointLightColor[i]*a + s*uPointLightColor[i]*0.3)*uPointLightIntensity[i]*at;
}
void main(){
    vec3 a=texture(uAlbedoMap,TexCoord).rgb*uColor;
    vec3 n=normalize(Normal); vec3 v=normalize(uCamPos-FragPos);
    vec3 r=Dir(n,v,a);
    for(int i=0;i<uNumPointLights&&i<MAX_PL;i++) r+=Pt(i,n,v,a);
    FragColor=vec4(r,1.0);
})GLSL";

static const char* k_GridVert = R"GLSL(
#version 450 core
layout(location=0) in vec3 aPos;
uniform mat4 uViewProjection;
out vec3 WorldPos;
void main(){ WorldPos=aPos; gl_Position=uViewProjection*vec4(aPos,1.0); })GLSL";

static const char* k_GridFrag = R"GLSL(
#version 450 core
in vec3 WorldPos; out vec4 FragColor;
uniform vec3 uCamPos; uniform float uGridSize; uniform vec3 uLineColor;
float Grid(vec2 p,float s){
    vec2 g=abs(fract(p/s-0.5)-0.5)/fwidth(p/s);
    return 1.0-min(min(g.x,g.y),1.0);
}
void main(){
    float dist=length(WorldPos.xz-uCamPos.xz);
    float a=1.0-smoothstep(30.0,60.0,dist);
    float line=Grid(WorldPos.xz,uGridSize);
    FragColor=vec4(uLineColor,line*a*0.55);
})GLSL";

// ── Grid mesh ─────────────────────────────────────────────────────────────────
static unsigned int s_GridVAO = 0, s_GridVBO = 0;

static void CreateGridMesh() {
    float verts[] = {
        -500,0,-500,  500,0,-500,  500,0, 500,
        -500,0,-500,  500,0, 500, -500,0, 500
    };
    glGenVertexArrays(1, &s_GridVAO);
    glGenBuffers(1, &s_GridVBO);
    glBindVertexArray(s_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_GridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, (void*)0);
    glBindVertexArray(0);
}

// ── Init / Shutdown ───────────────────────────────────────────────────────────
void Renderer::Init(int width, int height) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    s_LitShader  = LinkProgram(CompileShader(GL_VERTEX_SHADER,   k_LitVert),
                                CompileShader(GL_FRAGMENT_SHADER, k_LitFrag));
    s_GridShader = LinkProgram(CompileShader(GL_VERTEX_SHADER,   k_GridVert),
                                CompileShader(GL_FRAGMENT_SHADER, k_GridFrag));
    CreateGridMesh();

    // White 1x1 texture
    s_WhiteTexture = CreateWhiteTexture();

    SetupFramebuffer(width, height);
    PL_INFO("Renderer initialized ({}x{})", width, height);
}

void Renderer::Shutdown() {
    glDeleteProgram(s_LitShader);
    glDeleteProgram(s_GridShader);
    glDeleteVertexArrays(1, &s_GridVAO);
    glDeleteBuffers(1, &s_GridVBO);
    glDeleteFramebuffers(1, &s_SceneFBO);
    glDeleteTextures(1, &s_SceneColorTex);
    glDeleteRenderbuffers(1, &s_SceneDepthRBO);
    glDeleteTextures(1, &s_WhiteTexture);
}

// ── Framebuffer ───────────────────────────────────────────────────────────────
void Renderer::SetupFramebuffer(int w, int h) {
    if (s_SceneFBO) {
        glDeleteFramebuffers(1, &s_SceneFBO);
        glDeleteTextures(1, &s_SceneColorTex);
        glDeleteRenderbuffers(1, &s_SceneDepthRBO);
    }
    s_FBOWidth = w; s_FBOHeight = h;
    glGenFramebuffers(1, &s_SceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s_SceneFBO);

    glGenTextures(1, &s_SceneColorTex);
    glBindTexture(GL_TEXTURE_2D, s_SceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_SceneColorTex, 0);

    glGenRenderbuffers(1, &s_SceneDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, s_SceneDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s_SceneDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        PL_ERROR("Framebuffer incomplete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ResizeSceneFramebuffer(int w, int h) {
    if (w == s_FBOWidth && h == s_FBOHeight) return;
    SetupFramebuffer(w, h);
}

// ── Frame ─────────────────────────────────────────────────────────────────────
void Renderer::BeginFrame(int w, int h) {
    s_DrawCalls = 0; s_Triangles = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, s_SceneFBO);
    glViewport(0, 0, s_FBOWidth, s_FBOHeight);
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ── Scene rendering ───────────────────────────────────────────────────────────
void Renderer::RenderScene(Scene& scene) {
    Camera* cam = scene.GetActiveCamera();
    if (!cam) return;

    glm::mat4 view = cam->GetViewMatrix();
    glm::mat4 proj = cam->GetProjectionMatrix((float)s_FBOWidth / (float)s_FBOHeight);
    glm::vec3 camPos = cam->GetPosition();

    // Collect lights
    DirectionalLight dirLight;
    std::vector<PointLight> pointLights;

    auto& registry = scene.GetRegistry();

    // Directional light
    {
        auto view2 = registry.view<DirectionalLightComponent, TransformComponent>();
        for (auto e : view2) {
            auto& lc = view2.get<DirectionalLightComponent>(e);
            auto& tc = view2.get<TransformComponent>(e);
            dirLight.direction = glm::normalize(tc.Rotation);
            dirLight.color     = lc.Color;
            dirLight.intensity = lc.Intensity;
            break;
        }
    }
    // Point lights
    {
        auto view2 = registry.view<PointLightComponent, TransformComponent>();
        for (auto e : view2) {
            if (pointLights.size() >= 8) break;
            auto& lc = view2.get<PointLightComponent>(e);
            auto& tc = view2.get<TransformComponent>(e);
            pointLights.push_back({ tc.Position, lc.Color, lc.Intensity, lc.Range });
        }
    }

    // Draw meshes
    auto meshView = registry.view<MeshRendererComponent, TransformComponent>();
    for (auto entity : meshView) {
        auto& mr = meshView.get<MeshRendererComponent>(entity);
        auto& tc = meshView.get<TransformComponent>(entity);

        if (!mr.Mesh || mr.Mesh->VAO == 0) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, tc.Position);
        model = glm::rotate(model, glm::radians(tc.Rotation.y), {0,1,0});
        model = glm::rotate(model, glm::radians(tc.Rotation.x), {1,0,0});
        model = glm::rotate(model, glm::radians(tc.Rotation.z), {0,0,1});
        model = glm::scale(model, tc.Scale);

        glUseProgram(s_LitShader);
        glUniformMatrix4fv(glGetUniformLocation(s_LitShader,"uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(s_LitShader,"uView"),  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(s_LitShader,"uProjection"), 1, GL_FALSE, glm::value_ptr(proj));

        DrawMesh(*mr.Mesh, model, mr.TextureID ? mr.TextureID : s_WhiteTexture,
                 mr.Color, camPos, dirLight, pointLights);
    }

    // Draw grid (editor)
    if (scene.IsEditorMode()) {
        glm::mat4 vp = proj * view;
        glDisable(GL_CULL_FACE);
        glUseProgram(s_GridShader);
        glUniformMatrix4fv(glGetUniformLocation(s_GridShader,"uViewProjection"),1,GL_FALSE,glm::value_ptr(vp));
        glUniform3fv(glGetUniformLocation(s_GridShader,"uCamPos"),   1, glm::value_ptr(camPos));
        glUniform1f (glGetUniformLocation(s_GridShader,"uGridSize"),  1.0f);
        glUniform3f (glGetUniformLocation(s_GridShader,"uLineColor"), 0.4f,0.4f,0.4f);
        glBindVertexArray(s_GridVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        s_DrawCalls++;
        glEnable(GL_CULL_FACE);
    }
}

void Renderer::DrawMesh(const GPUMesh& mesh, const glm::mat4& transform,
                        unsigned int textureID, const glm::vec3& color,
                        const glm::vec3& camPos,
                        const DirectionalLight& dl,
                        const std::vector<PointLight>& pls)
{
    glUseProgram(s_LitShader);
    glUniformMatrix4fv(glGetUniformLocation(s_LitShader,"uModel"), 1, GL_FALSE, glm::value_ptr(transform));
    glUniform3fv(glGetUniformLocation(s_LitShader,"uColor"),    1, glm::value_ptr(color));
    glUniform1f (glGetUniformLocation(s_LitShader,"uShininess"), 32.0f);
    glUniform3fv(glGetUniformLocation(s_LitShader,"uCamPos"),   1, glm::value_ptr(camPos));
    glUniform3fv(glGetUniformLocation(s_LitShader,"uDirLightDir"),   1, glm::value_ptr(dl.direction));
    glUniform3fv(glGetUniformLocation(s_LitShader,"uDirLightColor"), 1, glm::value_ptr(dl.color));
    glUniform1f (glGetUniformLocation(s_LitShader,"uDirLightIntensity"), dl.intensity);

    int n = (int)pls.size();
    glUniform1i(glGetUniformLocation(s_LitShader,"uNumPointLights"), n);
    for (int i = 0; i < n && i < 8; ++i) {
        std::string base = "uPointLight";
        glUniform3fv(glGetUniformLocation(s_LitShader,("uPointLightPos["+std::to_string(i)+"]").c_str()),     1,glm::value_ptr(pls[i].position));
        glUniform3fv(glGetUniformLocation(s_LitShader,("uPointLightColor["+std::to_string(i)+"]").c_str()),   1,glm::value_ptr(pls[i].color));
        glUniform1f (glGetUniformLocation(s_LitShader,("uPointLightIntensity["+std::to_string(i)+"]").c_str()),pls[i].intensity);
        glUniform1f (glGetUniformLocation(s_LitShader,("uPointLightRange["+std::to_string(i)+"]").c_str()),    pls[i].range);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(s_LitShader,"uAlbedoMap"), 0);

    glBindVertexArray(mesh.VAO);
    if (mesh.indexCount > 0) {
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        s_Triangles += mesh.indexCount / 3;
    } else {
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
        s_Triangles += mesh.vertexCount / 3;
    }
    glBindVertexArray(0);
    s_DrawCalls++;
}

// ── Mesh creation ─────────────────────────────────────────────────────────────
static unsigned int UploadMesh(GPUMesh& m, const std::vector<float>& verts,
                               const std::vector<unsigned int>& idx) {
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*4, verts.data(), GL_STATIC_DRAW);
    if (!idx.empty()) {
        glGenBuffers(1, &m.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*4, idx.data(), GL_STATIC_DRAW);
        m.indexCount = (unsigned int)idx.size();
    }
    m.vertexCount = (unsigned int)verts.size() / 8; // pos(3)+norm(3)+uv(2)
    // layout
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,32,(void*)12);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,32,(void*)24);
    glBindVertexArray(0);
    return m.VAO;
}

GPUMesh Renderer::CreateCube(float s) {
    float h = s * 0.5f;
    // 6 faces * 4 verts, each: pos(3) normal(3) uv(2)
    std::vector<float> v = {
        // Front (+Z)
        -h,-h, h, 0,0,1, 0,0,   h,-h, h, 0,0,1, 1,0,   h, h, h, 0,0,1, 1,1,  -h, h, h, 0,0,1, 0,1,
        // Back  (-Z)
         h,-h,-h, 0,0,-1,0,0,  -h,-h,-h, 0,0,-1,1,0,  -h, h,-h, 0,0,-1,1,1,   h, h,-h, 0,0,-1,0,1,
        // Left  (-X)
        -h,-h,-h,-1,0,0, 0,0,  -h,-h, h,-1,0,0, 1,0,  -h, h, h,-1,0,0, 1,1,  -h, h,-h,-1,0,0, 0,1,
        // Right (+X)
         h,-h, h, 1,0,0, 0,0,   h,-h,-h, 1,0,0, 1,0,   h, h,-h, 1,0,0, 1,1,   h, h, h, 1,0,0, 0,1,
        // Top   (+Y)
        -h, h, h, 0,1,0, 0,0,   h, h, h, 0,1,0, 1,0,   h, h,-h, 0,1,0, 1,1,  -h, h,-h, 0,1,0, 0,1,
        // Bottom(-Y)
        -h,-h,-h, 0,-1,0,0,0,   h,-h,-h, 0,-1,0,1,0,   h,-h, h, 0,-1,0,1,1,  -h,-h, h, 0,-1,0,0,1,
    };
    std::vector<unsigned int> idx;
    for (unsigned int i = 0; i < 6; ++i) {
        unsigned int b = i*4;
        idx.insert(idx.end(), {b,b+1,b+2, b,b+2,b+3});
    }
    GPUMesh m; UploadMesh(m, v, idx); return m;
}

GPUMesh Renderer::CreateSphere(float radius, int stacks, int slices) {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    for (int i = 0; i <= stacks; ++i) {
        float phi = (float)i / stacks * glm::pi<float>();
        for (int j = 0; j <= slices; ++j) {
            float theta = (float)j / slices * 2.0f * glm::pi<float>();
            float x = sinf(phi)*cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi)*sinf(theta);
            v.insert(v.end(), {x*radius, y*radius, z*radius, x,y,z,
                               (float)j/slices, (float)i/stacks});
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            unsigned int a=i*(slices+1)+j, b=a+slices+1;
            idx.insert(idx.end(),{a,b,a+1, b,b+1,a+1});
        }
    GPUMesh m; UploadMesh(m, v, idx); return m;
}

GPUMesh Renderer::CreateCapsule(float radius, float height, int segs) {
    // Build cylinder body + two sphere caps
    std::vector<float> v;
    std::vector<unsigned int> idx;
    float halfH = height * 0.5f - radius;
    if (halfH < 0) halfH = 0;

    int hemisphereStacks = 8;
    // Top cap
    for (int i = 0; i <= hemisphereStacks; ++i) {
        float phi = (float)i / hemisphereStacks * glm::half_pi<float>();
        for (int j = 0; j <= segs; ++j) {
            float theta = (float)j / segs * 2.0f * glm::pi<float>();
            float x = cosf(phi)*cosf(theta);
            float y = sinf(phi);
            float z = cosf(phi)*sinf(theta);
            v.insert(v.end(),{x*radius, y*radius+halfH, z*radius, x,y,z,
                              (float)j/segs, (float)i/hemisphereStacks*0.25f});
        }
    }
    // Bottom cap
    for (int i = 0; i <= hemisphereStacks; ++i) {
        float phi = (float)i / hemisphereStacks * glm::half_pi<float>();
        for (int j = 0; j <= segs; ++j) {
            float theta = (float)j / segs * 2.0f * glm::pi<float>();
            float x = cosf(phi)*cosf(theta);
            float y = -sinf(phi);
            float z = cosf(phi)*sinf(theta);
            v.insert(v.end(),{x*radius, y*radius-halfH, z*radius, x,y,z,
                              (float)j/segs, 0.75f+(float)i/hemisphereStacks*0.25f});
        }
    }
    int n = segs+1;
    int topOff = 0, botOff = (hemisphereStacks+1)*n;
    for (int i = 0; i < hemisphereStacks; ++i)
        for (int j = 0; j < segs; ++j) {
            unsigned int a=topOff+i*n+j, b=a+n;
            idx.insert(idx.end(),{a,b,a+1,b,b+1,a+1});
            unsigned int c=botOff+i*n+j, d=c+n;
            idx.insert(idx.end(),{c,d,c+1,d,d+1,c+1});
        }
    // Cylinder band
    int cylOff = (int)(v.size()/8);
    for (int j = 0; j <= segs; ++j) {
        float theta=(float)j/segs*2.0f*glm::pi<float>();
        float x=cosf(theta), z=sinf(theta);
        v.insert(v.end(),{x*radius, halfH, z*radius, x,0,z,(float)j/segs,0.5f});
        v.insert(v.end(),{x*radius,-halfH, z*radius, x,0,z,(float)j/segs,0.75f});
    }
    for (int j = 0; j < segs; ++j) {
        unsigned int a=cylOff+j*2, b=a+2;
        idx.insert(idx.end(),{a,a+1,b, a+1,b+1,b});
    }
    GPUMesh m; UploadMesh(m, v, idx); return m;
}

GPUMesh Renderer::CreatePlane(float size, int sub) {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    float step = size / sub;
    float start= -size * 0.5f;
    for (int i = 0; i <= sub; ++i)
        for (int j = 0; j <= sub; ++j) {
            float x = start + j*step, z = start + i*step;
            v.insert(v.end(),{x,0,z, 0,1,0, (float)j/sub,(float)i/sub});
        }
    for (int i = 0; i < sub; ++i)
        for (int j = 0; j < sub; ++j) {
            unsigned int a=i*(sub+1)+j;
            idx.insert(idx.end(),{a,a+sub+1,a+1, a+1,a+sub+1,a+sub+2});
        }
    GPUMesh m; UploadMesh(m, v, idx); return m;
}

GPUMesh Renderer::CreateMeshFromData(const MeshData& data) {
    GPUMesh m; UploadMesh(m, data.vertices, data.indices); return m;
}

void Renderer::DestroyMesh(GPUMesh& m) {
    if (m.EBO) glDeleteBuffers(1,&m.EBO);
    if (m.VBO) glDeleteBuffers(1,&m.VBO);
    if (m.VAO) glDeleteVertexArrays(1,&m.VAO);
    m = {};
}

// ── Texture ───────────────────────────────────────────────────────────────────
unsigned int Renderer::LoadTexture(const std::string& path) {
    int w,h,ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(),&w,&h,&ch,4);
    if (!data) { PL_WARN("Texture not found: {}", path); return s_WhiteTexture; }
    unsigned int id;
    glGenTextures(1,&id);
    glBindTexture(GL_TEXTURE_2D,id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(data);
    return id;
}

unsigned int Renderer::CreateWhiteTexture() {
    unsigned int id;
    glGenTextures(1,&id);
    glBindTexture(GL_TEXTURE_2D,id);
    unsigned char white[4]={255,255,255,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);
    return id;
}

} // namespace Pillar
