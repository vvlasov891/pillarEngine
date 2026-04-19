#include "EditorLayer.h"
#include "Core/Application.h"
#include "Core/Log.h"

int main() {
    Pillar::AppConfig cfg;
    cfg.title      = "PillarEngine Editor";
    cfg.width      = 1440;
    cfg.height     = 900;
    cfg.vsync      = true;
    cfg.editorMode = true;

    auto& app = Pillar::Application::Get();
    app.Init(cfg);

    auto editor = std::make_shared<Pillar::EditorLayer>();
    editor->Init();

    app.SetEditorUpdateCallback([&](float dt) { editor->OnUpdate(dt); });
    app.SetEditorRenderCallback([&]()          { editor->OnImGuiRender(); });

    app.Run();
    return 0;
}
