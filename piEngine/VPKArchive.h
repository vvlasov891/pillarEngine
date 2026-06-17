#include "Input.h"
#include <GLFW/glfw3.h>

namespace Pillar {

GLFWwindow*              Input::s_Window      = nullptr;
std::unordered_set<int>  Input::s_KeysDown;
std::unordered_set<int>  Input::s_KeysPressed;
std::unordered_set<int>  Input::s_KeysReleased;
std::unordered_set<int>  Input::s_MouseDown;
std::unordered_set<int>  Input::s_MousePressed;
glm::vec2                Input::s_MousePos    = {0,0};
glm::vec2                Input::s_MouseLastPos= {0,0};
glm::vec2                Input::s_MouseDelta  = {0,0};
float                    Input::s_ScrollDelta = 0.0f;
bool                     Input::s_CursorLocked= false;
bool                     Input::s_FirstMouse  = true;

void Input::Init(GLFWwindow* window) {
    s_Window = window;
    glfwSetKeyCallback(window,         OnKey);
    glfwSetMouseButtonCallback(window, OnMouseButton);
    glfwSetCursorPosCallback(window,   OnMouseMove);
    glfwSetScrollCallback(window,      OnScroll);
}

void Input::Update() {
    s_KeysPressed.clear();
    s_KeysReleased.clear();
    s_MousePressed.clear();
    s_MouseDelta  = s_MousePos - s_MouseLastPos;
    s_MouseLastPos= s_MousePos;
    s_ScrollDelta = 0.0f;
}

bool Input::IsKeyDown(int k)      { return s_KeysDown.count(k) > 0; }
bool Input::IsKeyPressed(int k)   { return s_KeysPressed.count(k) > 0; }
bool Input::IsKeyReleased(int k)  { return s_KeysReleased.count(k) > 0; }
bool Input::IsMouseDown(int b)    { return s_MouseDown.count(b) > 0; }
bool Input::IsMousePressed(int b) { return s_MousePressed.count(b) > 0; }
glm::vec2 Input::GetMousePos()    { return s_MousePos; }
glm::vec2 Input::GetMouseDelta()  { return s_MouseDelta; }
float Input::GetScrollDelta()     { return s_ScrollDelta; }

void Input::SetCursorLocked(bool locked) {
    s_CursorLocked = locked;
    s_FirstMouse   = true;
    glfwSetInputMode(s_Window, GLFW_CURSOR,
        locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Input::OnKey(GLFWwindow*, int key, int, int action, int) {
    if (action == GLFW_PRESS)   { s_KeysDown.insert(key);  s_KeysPressed.insert(key); }
    if (action == GLFW_RELEASE) { s_KeysDown.erase(key);   s_KeysReleased.insert(key); }
}

void Input::OnMouseButton(GLFWwindow*, int button, int action, int) {
    if (action == GLFW_PRESS)   { s_MouseDown.insert(button);  s_MousePressed.insert(button); }
    if (action == GLFW_RELEASE) { s_MouseDown.erase(button); }
}

void Input::OnMouseMove(GLFWwindow*, double x, double y) {
    glm::vec2 pos = { (float)x, (float)y };
    if (s_FirstMouse) { s_MouseLastPos = pos; s_FirstMouse = false; }
    s_MousePos = pos;
}

void Input::OnScroll(GLFWwindow*, double, double yo) {
    s_ScrollDelta += (float)yo;
}

} // namespace Pillar
