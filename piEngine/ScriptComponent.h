#pragma once
#include <string>
#include <memory>
#include <functional>

struct GLFWwindow;
namespace Pillar {

class Scene;
class Renderer;

struct AppConfig {
    std::string title      = "PillarEngine";
    int         width      = 1280;
    int         height     = 720;
    bool        vsync      = true;
    bool        editorMode = false;
    std::string startScene = "";  // path to .pilevel
};

class Application {
public:
    static Application& Get();

    void Init(const AppConfig& config);
    void Run();
    void Quit() { m_Running = false; }

    GLFWwindow* GetWindow()   const { return m_Window; }
    int         GetWidth()    const { return m_Width; }
    int         GetHeight()   const { return m_Height; }
    float       GetDeltaTime()const { return m_DeltaTime; }
    float       GetTime()     const { return m_Time; }

    Scene*    GetActiveScene()  const { return m_ActiveScene.get(); }
    void      LoadScene(const std::string& path);
    void      NewScene();

    // Called from editor
    void      SetEditorUpdateCallback(std::function<void(float)> cb) { m_EditorUpdate = cb; }
    void      SetEditorRenderCallback(std::function<void()> cb)      { m_EditorRender = cb; }

private:
    Application() = default;
    void ProcessInput();
    void Update(float dt);
    void Render();

    GLFWwindow*                    m_Window     = nullptr;
    bool                           m_Running    = false;
    int                            m_Width      = 1280;
    int                            m_Height     = 720;
    float                          m_DeltaTime  = 0.0f;
    float                          m_Time       = 0.0f;
    float                          m_LastFrame  = 0.0f;
    bool                           m_EditorMode = false;

    std::shared_ptr<Scene>         m_ActiveScene;
    std::function<void(float)>     m_EditorUpdate;
    std::function<void()>          m_EditorRender;
};

} // namespace Pillar
