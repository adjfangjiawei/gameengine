
#pragma once

#include <memory>
#include <string>

#include "CoreTypes.h"

struct GLFWwindow;

namespace Engine {

    class Window {
      public:
        Window(const std::string &title, int width, int height);
        ~Window();

        // Prevent copying
        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        // Window control
        void Close();
        bool ShouldClose() const;
        void SwapBuffers();
        void PollEvents();

        // Getters
        GLFWwindow *GetNativeWindow() const { return window; }
        const std::string &GetTitle() const { return title; }
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        bool IsVSync() const { return vSync; }

        // Setters
        void SetVSync(bool enabled);
        void SetTitle(const std::string &newTitle);

      private:
        GLFWwindow *window;
        std::string title;
        int width;
        int height;
        bool vSync;
    };

}  // namespace Engine
