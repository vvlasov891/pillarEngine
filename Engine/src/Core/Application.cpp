#include "Application.h"
#include "Log.h"
#include "Input.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Audio/AudioSystem.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Pillar {

static void GLAPIENTRY GLDebugCallback(GLenum, GLenum type, GLuint, GLenum severity,
    GLsizei, const GLchar* msg, const void*)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    if (type == GL_DEBUG_TYPE_ERROR)
        PL_ERROR("GL Error: {}", msg);
    else
        PL_WARN("GL Warning: {}", msg);
}

static void FramebufferSizeCallback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
    Application::Get().GetActiveScene(); // notify resize if needed
}

Application& Application::Get() {
    static Application instance;
    return instance;
}

void Application::Init(const AppConfig& config) {
    Log::Init();
    PL_INFO("Initializing PillarEngine...");

    m_Width      = config.width;
    m_Height     = config.height;
    m_EditorMode = config.editorMode;

    // ── GLFW ─────────────────────────────────────────────────────────────────
    if (!glfwInit()) { PL_FATAL("GLFW init failed"); return; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef PILLAR_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    m_Window = glfwCreateWindow(m_Width, m_Height, config.title.c_str(), nullptr, nullptr);
    if (!m_Window) { PL_FATAL("Failed to create GLFW window"); return; }
    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(config.vsync ? 1 : 0);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);

    // ── GLAD ─────────────────────────────────────────────────────────────────
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        PL_FATAL("GLAD init failed"); return;
    }

#ifdef PILLAR_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugCallback, nullptr);
#endif

    // ── ImGui ────────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // ── Input ────────────────────────────────────────────────────────────────
    Input::Init(m_Window);

    // ── Renderer ─────────────────────────────────────────────────────────────
    Renderer::Init(m_Width, m_Height);

    // ── Audio ────────────────────────────────────────────────────────────────
    AudioSystem::Init();

    // ── Scene ────────────────────────────────────────────────────────────────
    if (!config.startScene.empty())
        LoadScene(config.startScene);
    else
        NewScene();

    m_Running = true;
    PL_INFO("PillarEngine ready. OpenGL {}", (const char*)glGetString(GL_VERSION));
}

void Application::Run() {
    while (m_Running && !glfwWindowShouldClose(m_Window)) {
        float currentTime = (float)glfwGetTime();
        m_DeltaTime = currentTime - m_LastFrame;
        m_LastFrame = currentTime;
        m_Time      = currentTime;

        glfwPollEvents();
        Input::Update();

        // ── ImGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ── Update
        if (m_ActiveScene) {
            if (!m_EditorMode)
                m_ActiveScene->OnUpdate(m_DeltaTime);
            else if (m_EditorUpdate)
                m_EditorUpdate(m_DeltaTime);
        }

        // ── Render
        Render();

        // ── Editor UI
        if (m_EditorRender)
            m_EditorRender();

        // ── ImGui render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Multi-viewport
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }

        glfwSwapBuffers(m_Window);
    }

    // Cleanup
    AudioSystem::Shutdown();
    Renderer::Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_Window);
    glfwTerminate();
    Log::Shutdown();
}

void Application::Render() {
    glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
    Renderer::BeginFrame(m_Width, m_Height);
    if (m_ActiveScene)
        Renderer::RenderScene(*m_ActiveScene);
    Renderer::EndFrame();
}

void Application::LoadScene(const std::string& path) {
    m_ActiveScene = std::make_shared<Scene>();
    m_ActiveScene->LoadFromFile(path);
    PL_INFO("Loaded scene: {}", path);
}

void Application::NewScene() {
    m_ActiveScene = std::make_shared<Scene>();
    m_ActiveScene->SetName("Untitled");
    PL_INFO("New empty scene created.");
}

} // namespace Pillar
