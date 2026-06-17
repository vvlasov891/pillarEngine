#include "EditorLayer.h"
#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Scripting/ScriptEngine.h"
#include "Renderer/Renderer.h"
#include "VPK/VPKArchive.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
namespace Pillar {

// ── Helpers ───────────────────────────────────────────────────────────────────
static void DrawVec3Control(const std::string& label, glm::vec3& values,
                             float resetVal = 0.0f, float colWidth = 80.0f)
{
    ImGui::PushID(label.c_str());
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, colWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    float lineH = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 btnSz = { lineH + 3.0f, lineH };
    auto resetBtn = [&](const char* lbl, float& v, ImVec4 col){
        ImGui::PushStyleColor(ImGuiCol_Button, col);
        if (ImGui::Button(lbl, btnSz)) v = resetVal;
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::DragFloat(("##"+std::string(lbl)).c_str(), &v, 0.1f);
        ImGui::PopItemWidth();
    };
    resetBtn("X", values.x, {0.8f,0.1f,0.1f,1});
    ImGui::SameLine();
    resetBtn("Y", values.y, {0.1f,0.7f,0.1f,1});
    ImGui::SameLine();
    resetBtn("Z", values.z, {0.1f,0.2f,0.8f,1});
    ImGui::Columns(1);
    ImGui::PopID();
}

// ── Init ─────────────────────────────────────────────────────────────────────
void EditorLayer::Init() {
    m_EditorCamera = Camera(60.0f, 0.1f, 1000.0f);
    m_EditorCamera.SetPosition({0, 3, 8});
    m_EditorCamera.SetRotation(-90.0f, -15.0f);

    NewScene();

    // Apply dark theme
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 4.0f;
    style.FrameRounding    = 3.0f;
    style.PopupRounding    = 3.0f;
    style.ScrollbarRounding= 3.0f;
    style.Colors[ImGuiCol_WindowBg]       = {0.12f,0.12f,0.12f,1.0f};
    style.Colors[ImGuiCol_Header]         = {0.20f,0.20f,0.20f,1.0f};
    style.Colors[ImGuiCol_HeaderHovered]  = {0.30f,0.30f,0.30f,1.0f};
    style.Colors[ImGuiCol_Button]         = {0.20f,0.20f,0.20f,1.0f};
    style.Colors[ImGuiCol_ButtonHovered]  = {0.28f,0.28f,0.28f,1.0f};
    style.Colors[ImGuiCol_FrameBg]        = {0.16f,0.16f,0.16f,1.0f};
    style.Colors[ImGuiCol_TitleBgActive]  = {0.10f,0.10f,0.10f,1.0f};
    style.Colors[ImGuiCol_Tab]            = {0.15f,0.15f,0.15f,1.0f};
    style.Colors[ImGuiCol_TabActive]      = {0.22f,0.22f,0.22f,1.0f};
    style.Colors[ImGuiCol_CheckMark]      = {0.40f,0.75f,0.40f,1.0f};
    style.Colors[ImGuiCol_SliderGrab]     = {0.40f,0.40f,0.40f,1.0f};
}

// ── Update ────────────────────────────────────────────────────────────────────
void EditorLayer::OnUpdate(float dt) {
    if (!m_IsPlaying) {
        UpdateEditorCamera(dt);
        m_ActiveScene->SetEditorMode(true);
        m_ActiveScene->SetEditorCamera(&m_EditorCamera);
    }
}

void EditorLayer::UpdateEditorCamera(float dt) {
    if (!m_ViewportFocused) return;
    bool rmb = Input::IsMouseDown(PL_MOUSE_RIGHT);
    if (rmb) {
        Input::SetCursorLocked(true);
        auto delta = Input::GetMouseDelta();
        m_EditorCamera.ProcessMouseMovement(delta.x, delta.y, 0.15f);
        m_EditorCamera.ProcessKeyboard(dt,
            Input::IsKeyDown(PL_KEY_W), Input::IsKeyDown(PL_KEY_S),
            Input::IsKeyDown(PL_KEY_A), Input::IsKeyDown(PL_KEY_D),
            Input::IsKeyDown(PL_KEY_SPACE),
            Input::IsKeyDown(PL_KEY_LSHIFT),
            m_EditorCamSpeed);
    } else {
        Input::SetCursorLocked(false);
    }
    m_EditorCamera.ProcessMouseScroll(Input::GetScrollDelta());
}

// ── ImGui Render ──────────────────────────────────────────────────────────────
void EditorLayer::OnImGuiRender() {
    // Full-window dockspace
    ImGuiWindowFlags wf = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::Begin("DockSpace", nullptr, wf);
    ImGui::PopStyleVar();
    ImGui::DockSpace(ImGui::GetID("MainDS"), {0,0},
                     ImGuiDockNodeFlags_PassthruCentralNode);
    DrawMenuBar();
    ImGui::End();

    DrawHierarchy();
    DrawInspector();
    DrawViewport();
    DrawContentBrowser();
    DrawConsole();
    DrawStats();

    // ── Open scene dialog
    if (m_ShowOpenDialog) {
        ImGui::OpenPopup("Open Scene");
        m_ShowOpenDialog = false;
    }
    if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Scene path (.pilevel):");
        ImGui::InputText("##path", m_ScenePathBuf, sizeof(m_ScenePathBuf));
        if (ImGui::Button("Open", {120,0})) {
            m_ActiveScene = std::make_shared<Scene>();
            m_ActiveScene->LoadFromFile(m_ScenePathBuf);
            m_SelectedEntity = Entity();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel",{120,0})) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ── Save As dialog
    if (m_ShowSaveAsDialog) {
        ImGui::OpenPopup("Save Scene As");
        m_ShowSaveAsDialog = false;
    }
    if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save path (.pilevel):");
        ImGui::InputText("##savepath", m_ScenePathBuf, sizeof(m_ScenePathBuf));
        if (ImGui::Button("Save", {120,0})) {
            m_ActiveScene->SaveToFile(m_ScenePathBuf);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel",{120,0})) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ── Add Script dialog
    if (m_ShowAddScriptDialog) {
        ImGui::OpenPopup("Add Script");
        m_ShowAddScriptDialog = false;
    }
    if (ImGui::BeginPopupModal("Add Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Script name:");
        ImGui::InputText("##sname", m_ScriptNameBuf, sizeof(m_ScriptNameBuf));
        ImGui::Separator();
        ImGui::Text("Registered scripts:");
        for (auto& n : ScriptEngine::GetRegisteredNames()) {
            if (ImGui::Selectable(n.c_str()))
                strncpy(m_ScriptNameBuf, n.c_str(), sizeof(m_ScriptNameBuf)-1);
        }
        ImGui::Separator();
        if (ImGui::Button("Add", {120,0})) {
            if (m_SelectedEntity && strlen(m_ScriptNameBuf) > 0) {
                if (!m_SelectedEntity.HasComponent<ScriptContainerComponent>())
                    m_SelectedEntity.AddComponent<ScriptContainerComponent>();
                auto& sc = m_SelectedEntity.GetComponent<ScriptContainerComponent>();
                sc.ScriptNames.push_back(m_ScriptNameBuf);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel",{120,0})) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ── Menu Bar ──────────────────────────────────────────────────────────────────
void EditorLayer::DrawMenuBar() {
    if (!ImGui::BeginMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene",  "Ctrl+N")) NewScene();
        if (ImGui::MenuItem("Open Scene", "Ctrl+O")) OpenScene();
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) SaveScene();
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) SaveSceneAs();
        ImGui::Separator();
        if (ImGui::MenuItem("Build Game (VPK)")) {
            auto& filePath = m_ActiveScene->GetFilePath();
            SaveScene();
            // Pack assets folder into game.vpk
            VPKArchive::Pack("assets", "build/game.vpk");
            PL_INFO("Build complete. game.vpk created.");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) Application::Get().Quit();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Empty Entity"))       CreateEmpty();
        ImGui::Separator();
        if (ImGui::MenuItem("Cube"))               CreateCube();
        if (ImGui::MenuItem("Sphere"))             CreateSphere();
        if (ImGui::MenuItem("Capsule"))            CreateCapsule();
        if (ImGui::MenuItem("Plane"))              CreatePlane();
        ImGui::Separator();
        if (ImGui::MenuItem("Point Light"))        CreatePointLight();
        if (ImGui::MenuItem("Directional Light"))  CreateDirectionalLight();
        ImGui::Separator();
        if (ImGui::MenuItem("Camera"))             CreateCamera();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene")) {
        if (!m_IsPlaying && ImGui::MenuItem("Play",  "F5")) PlayScene();
        if ( m_IsPlaying && ImGui::MenuItem("Stop",  "F5")) StopScene();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Pack Assets to VPK")) {
            VPKArchive::Pack("assets", "game.vpk");
        }
        if (ImGui::MenuItem("Extract VPK...")) {
            VPKArchive::Extract("game.vpk", "extracted");
        }
        ImGui::EndMenu();
    }

    // Play/Stop buttons center
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120) * 0.5f);
    if (!m_IsPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.1f,0.6f,0.1f,1.0f});
        if (ImGui::Button("  ▶  Play  ", {120,0})) PlayScene();
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.7f,0.1f,0.1f,1.0f});
        if (ImGui::Button("  ■  Stop  ", {120,0})) StopScene();
        ImGui::PopStyleColor();
    }

    ImGui::EndMenuBar();

    // Keyboard shortcuts
    if (Input::IsKeyDown(341) && Input::IsKeyPressed('N')) NewScene();
    if (Input::IsKeyDown(341) && Input::IsKeyPressed('O')) OpenScene();
    if (Input::IsKeyDown(341) && Input::IsKeyPressed('S')) SaveScene();
    if (Input::IsKeyPressed(PL_KEY_F5)) {
        if (m_IsPlaying) StopScene(); else PlayScene();
    }
}

// ── Hierarchy ────────────────────────────────────────────────────────────────
void EditorLayer::DrawHierarchy() {
    ImGui::Begin("Hierarchy");

    if (ImGui::Button("+ Add Entity")) CreateEmpty();
    ImGui::Separator();

    auto& reg = m_ActiveScene->GetRegistry();
    reg.each([&](auto e) {
        Entity ent(e, m_ActiveScene.get());
        if (!ent.HasComponent<TagComponent>()) return;
        auto& tag = ent.GetComponent<TagComponent>();
        bool selected = (m_SelectedEntity == ent);
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | (selected ? ImGuiTreeNodeFlags_Selected : 0);
        bool open = ImGui::TreeNodeEx((void*)(uint64_t)e, flags, "%s", tag.Name.c_str());
        if (ImGui::IsItemClicked()) m_SelectedEntity = ent;
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                m_ActiveScene->DestroyEntity(ent);
                if (m_SelectedEntity == ent) m_SelectedEntity = Entity();
            }
            ImGui::EndPopup();
        }
        if (open) ImGui::TreePop();
    });

    ImGui::End();
}

// ── Inspector ─────────────────────────────────────────────────────────────────
void EditorLayer::DrawInspector() {
    ImGui::Begin("Inspector");
    if (!m_SelectedEntity || !m_SelectedEntity.IsValid()) {
        ImGui::Text("No entity selected.");
        ImGui::End(); return;
    }

    // Tag
    if (m_SelectedEntity.HasComponent<TagComponent>()) {
        auto& tag = m_SelectedEntity.GetComponent<TagComponent>();
        char buf[256]; strncpy(buf, tag.Name.c_str(), 255);
        if (ImGui::InputText("Name##tag", buf, 255)) tag.Name = buf;
        ImGui::SameLine(); ImGui::Checkbox("Active", &tag.Active);
        ImGui::TextDisabled("UUID: %llu", (unsigned long long)(uint64_t)tag.ID);
        ImGui::Separator();
    }

    // Transform
    if (m_SelectedEntity.HasComponent<TransformComponent>()) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
            DrawVec3Control("Position", tc.Position);
            DrawVec3Control("Rotation", tc.Rotation);
            DrawVec3Control("Scale",    tc.Scale, 1.0f);
        }
    }

    // MeshRenderer
    if (m_SelectedEntity.HasComponent<MeshRendererComponent>()) {
        if (ImGui::CollapsingHeader("Mesh Renderer")) {
            auto& mr = m_SelectedEntity.GetComponent<MeshRendererComponent>();
            ImGui::ColorEdit3("Color", glm::value_ptr(mr.Color));
            char meshBuf[256]; strncpy(meshBuf, mr.MeshPath.c_str(), 255);
            if (ImGui::InputText("Mesh##mr", meshBuf, 255)) mr.MeshPath = meshBuf;
            char texBuf[256]; strncpy(texBuf, mr.TexturePath.c_str(), 255);
            if (ImGui::InputText("Texture##mr", texBuf, 255)) mr.TexturePath = texBuf;
        }
    }

    // DirectionalLight
    if (m_SelectedEntity.HasComponent<DirectionalLightComponent>()) {
        if (ImGui::CollapsingHeader("Directional Light")) {
            auto& dl = m_SelectedEntity.GetComponent<DirectionalLightComponent>();
            ImGui::ColorEdit3("Color##dl", glm::value_ptr(dl.Color));
            ImGui::DragFloat("Intensity##dl", &dl.Intensity, 0.01f, 0.0f, 10.0f);
        }
    }

    // PointLight
    if (m_SelectedEntity.HasComponent<PointLightComponent>()) {
        if (ImGui::CollapsingHeader("Point Light")) {
            auto& pl = m_SelectedEntity.GetComponent<PointLightComponent>();
            ImGui::ColorEdit3("Color##pl",    glm::value_ptr(pl.Color));
            ImGui::DragFloat("Intensity##pl", &pl.Intensity, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Range##pl",     &pl.Range,     0.1f,  0.0f, 100.0f);
        }
    }

    // Camera
    if (m_SelectedEntity.HasComponent<CameraComponent>()) {
        if (ImGui::CollapsingHeader("Camera")) {
            auto& cam = m_SelectedEntity.GetComponent<CameraComponent>();
            ImGui::DragFloat("FOV",  &cam.FOV,  0.5f, 10.0f, 170.0f);
            ImGui::DragFloat("Near", &cam.Near, 0.01f, 0.01f, 10.0f);
            ImGui::DragFloat("Far",  &cam.Far,  1.0f, 10.0f, 10000.0f);
            ImGui::Checkbox("Is Main", &cam.IsMain);
        }
    }

    // Rigidbody
    if (m_SelectedEntity.HasComponent<RigidbodyComponent>()) {
        if (ImGui::CollapsingHeader("Rigidbody")) {
            auto& rb = m_SelectedEntity.GetComponent<RigidbodyComponent>();
            ImGui::DragFloat("Mass",   &rb.Mass,  0.1f, 0.01f, 100.0f);
            ImGui::DragFloat("Drag",   &rb.Drag,  0.01f, 0.0f, 1.0f);
            ImGui::Checkbox("Use Gravity",  &rb.UseGravity);
            ImGui::Checkbox("Is Kinematic", &rb.IsKinematic);
        }
    }

    // BoxCollider
    if (m_SelectedEntity.HasComponent<BoxColliderComponent>()) {
        if (ImGui::CollapsingHeader("Box Collider")) {
            auto& bc = m_SelectedEntity.GetComponent<BoxColliderComponent>();
            DrawVec3Control("Center##bc",  bc.Center);
            DrawVec3Control("HalfExt##bc", bc.HalfExt, 0.5f);
            ImGui::Checkbox("Is Trigger##bc", &bc.IsTrigger);
        }
    }

    // AudioSource
    if (m_SelectedEntity.HasComponent<AudioSourceComponent>()) {
        if (ImGui::CollapsingHeader("Audio Source")) {
            auto& as = m_SelectedEntity.GetComponent<AudioSourceComponent>();
            char clipBuf[256]; strncpy(clipBuf, as.ClipPath.c_str(), 255);
            if (ImGui::InputText("Clip##as", clipBuf, 255)) as.ClipPath = clipBuf;
            ImGui::DragFloat("Volume##as", &as.Volume, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Pitch##as",  &as.Pitch,  0.01f, 0.1f, 3.0f);
            ImGui::Checkbox("Loop##as",        &as.Loop);
            ImGui::Checkbox("Play On Start##as",&as.PlayOnStart);
        }
    }

    // Scripts
    if (m_SelectedEntity.HasComponent<ScriptContainerComponent>()) {
        if (ImGui::CollapsingHeader("Scripts")) {
            auto& sc = m_SelectedEntity.GetComponent<ScriptContainerComponent>();
            for (int i = 0; i < (int)sc.ScriptNames.size(); i++) {
                ImGui::PushID(i);
                ImGui::Text("%s", sc.ScriptNames[i].c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    sc.ScriptNames.erase(sc.ScriptNames.begin() + i);
                    ImGui::PopID(); break;
                }
                ImGui::PopID();
            }
        }
    }

    ImGui::Separator();

    // Add component buttons
    if (ImGui::Button("+ Add Component", {-1,0}))
        ImGui::OpenPopup("AddComp");
    if (ImGui::BeginPopup("AddComp")) {
        auto& e = m_SelectedEntity;
        if (!e.HasComponent<MeshRendererComponent>() && ImGui::MenuItem("Mesh Renderer"))
            e.AddComponent<MeshRendererComponent>();
        if (!e.HasComponent<DirectionalLightComponent>() && ImGui::MenuItem("Directional Light"))
            e.AddComponent<DirectionalLightComponent>();
        if (!e.HasComponent<PointLightComponent>() && ImGui::MenuItem("Point Light"))
            e.AddComponent<PointLightComponent>();
        if (!e.HasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
            e.AddComponent<CameraComponent>();
        if (!e.HasComponent<RigidbodyComponent>() && ImGui::MenuItem("Rigidbody"))
            e.AddComponent<RigidbodyComponent>();
        if (!e.HasComponent<BoxColliderComponent>() && ImGui::MenuItem("Box Collider"))
            e.AddComponent<BoxColliderComponent>();
        if (!e.HasComponent<SphereColliderComponent>() && ImGui::MenuItem("Sphere Collider"))
            e.AddComponent<SphereColliderComponent>();
        if (!e.HasComponent<AudioSourceComponent>() && ImGui::MenuItem("Audio Source"))
            e.AddComponent<AudioSourceComponent>();
        ImGui::Separator();
        if (ImGui::MenuItem("Attach Script...")) {
            m_ShowAddScriptDialog = true;
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

// ── Viewport ──────────────────────────────────────────────────────────────────
void EditorLayer::DrawViewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::Begin("Viewport");

    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsWindowHovered();

    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (sz.x > 1 && sz.y > 1) {
        m_ViewportWidth  = sz.x;
        m_ViewportHeight = sz.y;
        Renderer::ResizeSceneFramebuffer((int)sz.x, (int)sz.y);
    }

    ImTextureID texID = (ImTextureID)(uintptr_t)Renderer::GetSceneColorTexture();
    ImGui::Image(texID, sz, {0,1}, {1,0});  // flip Y for OpenGL

    // Gizmo hint
    ImGui::SetCursorPos({8, 30});
    ImGui::TextDisabled("RMB + WASD = fly camera | Scroll = zoom");

    ImGui::End();
    ImGui::PopStyleVar();
}

// ── Content Browser ───────────────────────────────────────────────────────────
void EditorLayer::DrawContentBrowser() {
    ImGui::Begin("Content Browser");
    static std::string currentPath = "assets";
    if (ImGui::Button("assets")) currentPath = "assets";
    ImGui::SameLine();
    ImGui::TextDisabled("|  %s", currentPath.c_str());
    ImGui::Separator();

    if (!fs::exists(currentPath)) {
        ImGui::TextColored({1,0.5f,0,1}, "Folder '%s' not found.", currentPath.c_str());
        ImGui::End(); return;
    }

    float cellSize = 80.0f;
    int cols = (int)(ImGui::GetContentRegionAvail().x / cellSize);
    if (cols < 1) cols = 1;
    ImGui::Columns(cols, nullptr, false);

    for (auto& entry : fs::directory_iterator(currentPath)) {
        std::string name = entry.path().filename().string();
        bool isDir = entry.is_directory();
        ImGui::PushID(name.c_str());
        if (isDir) {
            ImGui::PushStyleColor(ImGuiCol_Button, {0.3f,0.25f,0.1f,1.0f});
            if (ImGui::Button((std::string("📁 ")+name).c_str(), {cellSize-8,cellSize-8}))
                currentPath = entry.path().generic_string();
            ImGui::PopStyleColor();
        } else {
            if (ImGui::Button((std::string("📄 ")+name).c_str(), {cellSize-8,cellSize-8})) {
                // Double-click to open .pilevel
                std::string ext = entry.path().extension().string();
                if (ext == ".pilevel") {
                    m_ActiveScene = std::make_shared<Scene>();
                    m_ActiveScene->LoadFromFile(entry.path().generic_string());
                    m_SelectedEntity = Entity();
                }
            }
        }
        ImGui::TextWrapped("%s", name.c_str());
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::End();
}

// ── Console ───────────────────────────────────────────────────────────────────
void EditorLayer::DrawConsole() {
    ImGui::Begin("Console");
    if (ImGui::Button("Clear")) Log::Clear();
    ImGui::SameLine();
    ImGui::Text("%d entries", (int)Log::GetEntries().size());
    ImGui::Separator();

    ImGui::BeginChild("##console_scroll", {0,0}, false, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto& entry : Log::GetEntries()) {
        ImVec4 col = {1,1,1,1};
        if (entry.level == LogLevel::Warn)  col = {1,1,0,1};
        if (entry.level == LogLevel::Error) col = {1,0.3f,0.3f,1};
        if (entry.level == LogLevel::Fatal) col = {1,0,0,1};
        ImGui::TextColored(col, "[%s] %s", entry.timestamp.c_str(), entry.message.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::End();
}

// ── Stats ─────────────────────────────────────────────────────────────────────
void EditorLayer::DrawStats() {
    ImGui::Begin("Stats");
    ImGui::Text("Draw calls : %d", Renderer::GetDrawCalls());
    ImGui::Text("Triangles  : %d", Renderer::GetTriangles());
    ImGui::Text("FPS        : %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame time : %.2f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("Play mode  : %s", m_IsPlaying ? "PLAYING" : "EDITING");
    ImGui::End();
}

// ── Scene actions ─────────────────────────────────────────────────────────────
void EditorLayer::NewScene() {
    if (m_IsPlaying) StopScene();
    m_ActiveScene = std::make_shared<Scene>();
    m_ActiveScene->SetName("Untitled");
    // Add default directional light
    auto light = m_ActiveScene->CreateEntity("Directional Light");
    light.AddComponent<DirectionalLightComponent>();
    auto& ltc = light.GetComponent<TransformComponent>();
    ltc.Rotation = {-45, -45, 0};
    m_SelectedEntity = Entity();
    PL_INFO("New scene created.");
}

void EditorLayer::OpenScene()   { m_ShowOpenDialog = true; }
void EditorLayer::SaveSceneAs() { m_ShowSaveAsDialog = true; }

void EditorLayer::SaveScene() {
    auto& path = m_ActiveScene->GetFilePath();
    if (path.empty()) { SaveSceneAs(); return; }
    m_ActiveScene->SaveToFile(path);
}

void EditorLayer::PlayScene() {
    m_IsPlaying = true;
    // Save a snapshot so we can restore on Stop
    m_RuntimeScene = m_ActiveScene;
    // Deep copy via serialize/deserialize
    m_ActiveScene->SaveToFile("__temp_play__.pilevel");
    auto playCopy = std::make_shared<Scene>();
    playCopy->LoadFromFile("__temp_play__.pilevel");
    m_ActiveScene = playCopy;
    m_ActiveScene->SetEditorMode(false);
    m_ActiveScene->OnStart();
    PL_INFO("Scene PLAY started.");
}

void EditorLayer::StopScene() {
    m_IsPlaying = false;
    m_ActiveScene->OnStop();
    if (m_RuntimeScene) {
        m_ActiveScene = m_RuntimeScene;
        m_RuntimeScene.reset();
    }
    m_ActiveScene->SetEditorMode(true);
    m_ActiveScene->SetEditorCamera(&m_EditorCamera);
    PL_INFO("Scene STOPPED.");
}

// ── Object creation ───────────────────────────────────────────────────────────
void EditorLayer::CreateEmpty() {
    m_SelectedEntity = m_ActiveScene->CreateEntity("Empty Entity");
}

void EditorLayer::CreateCube() {
    auto e = m_ActiveScene->CreateEntity("Cube");
    auto& mr = e.AddComponent<MeshRendererComponent>();
    mr.Mesh = std::make_shared<GPUMesh>(Renderer::CreateCube());
    mr.Color = {1,1,1};
    m_SelectedEntity = e;
}

void EditorLayer::CreateSphere() {
    auto e = m_ActiveScene->CreateEntity("Sphere");
    auto& mr = e.AddComponent<MeshRendererComponent>();
    mr.Mesh = std::make_shared<GPUMesh>(Renderer::CreateSphere());
    m_SelectedEntity = e;
}

void EditorLayer::CreateCapsule() {
    auto e = m_ActiveScene->CreateEntity("Capsule");
    auto& mr = e.AddComponent<MeshRendererComponent>();
    mr.Mesh = std::make_shared<GPUMesh>(Renderer::CreateCapsule());
    m_SelectedEntity = e;
}

void EditorLayer::CreatePlane() {
    auto e = m_ActiveScene->CreateEntity("Plane");
    auto& mr = e.AddComponent<MeshRendererComponent>();
    mr.Mesh = std::make_shared<GPUMesh>(Renderer::CreatePlane(10.0f, 4));
    mr.Color = {0.5f, 0.5f, 0.5f};
    m_SelectedEntity = e;
}

void EditorLayer::CreatePointLight() {
    auto e = m_ActiveScene->CreateEntity("Point Light");
    e.AddComponent<PointLightComponent>();
    auto& tc = e.GetComponent<TransformComponent>();
    tc.Position = {0, 3, 0};
    m_SelectedEntity = e;
}

void EditorLayer::CreateDirectionalLight() {
    auto e = m_ActiveScene->CreateEntity("Directional Light");
    e.AddComponent<DirectionalLightComponent>();
    m_SelectedEntity = e;
}

void EditorLayer::CreateCamera() {
    auto e = m_ActiveScene->CreateEntity("Camera");
    auto& cam = e.AddComponent<CameraComponent>();
    cam.IsMain = true;
    auto& tc = e.GetComponent<TransformComponent>();
    tc.Position = {0, 2, 5};
    m_SelectedEntity = e;
}

} // namespace Pillar
