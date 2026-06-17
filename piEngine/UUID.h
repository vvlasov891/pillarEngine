#pragma once
#include <memory>
#include <string>
#include "Renderer/Camera.h"
#include "Scene/Entity.h"

namespace Pillar {

class Scene;

class EditorLayer {
public:
    void Init();
    void OnUpdate(float dt);
    void OnImGuiRender();

private:
    // ── Panels ────────────────────────────────────────────────────────────────
    void DrawMenuBar();
    void DrawHierarchy();
    void DrawInspector();
    void DrawViewport();
    void DrawContentBrowser();
    void DrawConsole();
    void DrawStats();

    // ── Scene actions ─────────────────────────────────────────────────────────
    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();
    void PlayScene();
    void StopScene();

    // ── Object creation ───────────────────────────────────────────────────────
    void CreateCube();
    void CreateSphere();
    void CreateCapsule();
    void CreatePlane();
    void CreatePointLight();
    void CreateDirectionalLight();
    void CreateCamera();
    void CreateEmpty();

    // ── Editor camera fly-around ──────────────────────────────────────────────
    void UpdateEditorCamera(float dt);

    // ── State ─────────────────────────────────────────────────────────────────
    std::shared_ptr<Scene>  m_ActiveScene;
    std::shared_ptr<Scene>  m_RuntimeScene;  // copy during play

    Entity   m_SelectedEntity;
    Camera   m_EditorCamera;
    bool     m_IsPlaying          = false;
    bool     m_ViewportFocused    = false;
    bool     m_ViewportHovered    = false;
    float    m_EditorCamSpeed     = 5.0f;

    // Viewport size
    float    m_ViewportWidth  = 1.0f;
    float    m_ViewportHeight = 1.0f;

    // Dialogs
    bool     m_ShowOpenDialog     = false;
    bool     m_ShowSaveAsDialog   = false;
    bool     m_ShowAddScriptDialog= false;
    char     m_ScenePathBuf[512]  = {};
    char     m_ScriptNameBuf[128] = {};
};

} // namespace Pillar
