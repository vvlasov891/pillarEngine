#include <Core/Application.h>
#include <Core/PillarEngine.h>

// Include scripts so PILLAR_REGISTER_SCRIPT macros run before main()
#include "../scripts/PlayerController.cpp"
#include "../scripts/EnemyAI.cpp"

int main(int argc, char** argv) {
    // Determine start scene
    // Priority: command-line arg > default "assets/levels/main.pilevel"
    std::string startScene = "assets/levels/main.pilevel";
    if (argc > 1) startScene = argv[1];

    Pillar::AppConfig cfg;
    cfg.title      = "My Pillar Game";
    cfg.width      = 1280;
    cfg.height     = 720;
    cfg.vsync      = true;
    cfg.editorMode = false;
    cfg.startScene = startScene;

    auto& app = Pillar::Application::Get();
    app.Init(cfg);
    app.Run();
    return 0;
}
