#pragma once
#include <glm/glm.hpp>
#include <unordered_set>

// Forward declare to avoid GLFW include in header
struct GLFWwindow;

namespace Pillar {

class Input {
public:
    static void Init(GLFWwindow* window);

    // Keyboard
    static bool IsKeyDown(int keycode);
    static bool IsKeyPressed(int keycode);   // true only on first frame
    static bool IsKeyReleased(int keycode);  // true only on release frame

    // Mouse
    static bool       IsMouseDown(int button);
    static bool       IsMousePressed(int button);
    static glm::vec2  GetMousePos();
    static glm::vec2  GetMouseDelta();
    static float      GetScrollDelta();

    // Called once per frame to update prev-state
    static void Update();
    static void SetCursorLocked(bool locked);
    static bool IsCursorLocked() { return s_CursorLocked; }

    // GLFW callbacks (registered internally)
    static void OnKey(GLFWwindow*, int key, int scancode, int action, int mods);
    static void OnMouseButton(GLFWwindow*, int button, int action, int mods);
    static void OnMouseMove(GLFWwindow*, double x, double y);
    static void OnScroll(GLFWwindow*, double xo, double yo);

private:
    static GLFWwindow*              s_Window;
    static std::unordered_set<int>  s_KeysDown;
    static std::unordered_set<int>  s_KeysPressed;
    static std::unordered_set<int>  s_KeysReleased;
    static std::unordered_set<int>  s_MouseDown;
    static std::unordered_set<int>  s_MousePressed;
    static glm::vec2                s_MousePos;
    static glm::vec2                s_MouseLastPos;
    static glm::vec2                s_MouseDelta;
    static float                    s_ScrollDelta;
    static bool                     s_CursorLocked;
    static bool                     s_FirstMouse;
};

// Key codes mirror GLFW
#define PL_KEY_W      87
#define PL_KEY_A      65
#define PL_KEY_S      83
#define PL_KEY_D      68
#define PL_KEY_SPACE  32
#define PL_KEY_LSHIFT 340
#define PL_KEY_LCTRL  341
#define PL_KEY_ESCAPE 256
#define PL_KEY_F1     290
#define PL_KEY_F5     294
#define PL_MOUSE_LEFT  0
#define PL_MOUSE_RIGHT 1

} // namespace Pillar
