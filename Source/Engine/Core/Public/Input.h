
#pragma once

#include <unordered_map>
#include <vector>

#include "CoreTypes.h"

struct GLFWwindow;

namespace Engine {

    enum class KeyCode;
    enum class MouseButton;

    class Input {
      public:
        static void Init(GLFWwindow *window);
        static void Update();

        // Keyboard input
        static bool IsKeyPressed(KeyCode key);
        static bool IsKeyHeld(KeyCode key);
        static bool IsKeyReleased(KeyCode key);

        // Mouse input
        static bool IsMouseButtonPressed(MouseButton button);
        static bool IsMouseButtonHeld(MouseButton button);
        static bool IsMouseButtonReleased(MouseButton button);

        static void GetMousePosition(double &x, double &y);
        static void GetMouseDelta(double &dx, double &dy);
        static double GetMouseScrollDelta();

      private:
        static void KeyCallback(
            GLFWwindow *window, int key, int scancode, int action, int mods);
        static void MouseButtonCallback(GLFWwindow *window,
                                        int button,
                                        int action,
                                        int mods);
        static void MousePositionCallback(GLFWwindow *window,
                                          double xpos,
                                          double ypos);
        static void MouseScrollCallback(GLFWwindow *window,
                                        double xoffset,
                                        double yoffset);

        struct KeyState {
            bool pressed = false;
            bool held = false;
            bool released = false;
        };

        struct MouseState {
            bool pressed = false;
            bool held = false;
            bool released = false;
        };

        static std::unordered_map<int, KeyState> keyStates;
        static std::unordered_map<int, MouseState> mouseButtonStates;
        static double mouseX;
        static double mouseY;
        static double lastMouseX;
        static double lastMouseY;
        static double mouseDeltaX;
        static double mouseDeltaY;
        static double mouseScrollDelta;
        static bool firstMouse;
    };

}  // namespace Engine
