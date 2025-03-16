
#pragma once

#include <string>

#include "Events/EventSystem.h"
#include "CoreTypes.h"

namespace Engine {

    /**
     * @brief Common event type IDs used throughout the engine
     */
    enum class CommonEventTypes : EventTypeId {
        // Engine lifecycle events
        EngineInitialized = 1000,
        EngineShutdown,

        // Window events
        WindowCreated,
        WindowResized,
        WindowClosed,
        WindowFocusChanged,

        // Input events
        KeyPressed,
        KeyReleased,
        MouseMoved,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseWheelScrolled,
        GamepadConnected,
        GamepadDisconnected,

        // Resource events
        ResourceLoaded,
        ResourceUnloaded,
        ResourceReloaded,

        // Scene events
        SceneLoading,
        SceneLoaded,
        SceneUnloading,
        SceneUnloaded,

        // Game state events
        GamePaused,
        GameResumed,
        GameSaved,
        GameLoaded,

        // Custom event types should start from here
        UserDefined = 10000
    };

    /**
     * @brief Base window event data
     */
    struct WindowEventData : public EventData {
        uint32_t windowId;
        explicit WindowEventData(uint32_t id) : windowId(id) {}
    };

    /**
     * @brief Window resize event data
     */
    struct WindowResizeEventData : public WindowEventData {
        uint32_t width;
        uint32_t height;
        WindowResizeEventData(uint32_t id, uint32_t w, uint32_t h)
            : WindowEventData(id), width(w), height(h) {}
    };

    /**
     * @brief Window focus event data
     */
    struct WindowFocusEventData : public WindowEventData {
        bool focused;
        WindowFocusEventData(uint32_t id, bool hasFocus)
            : WindowEventData(id), focused(hasFocus) {}
    };

    /**
     * @brief Key event data
     */
    struct KeyEventData : public EventData {
        int32_t keyCode;
        bool alt;
        bool control;
        bool shift;
        KeyEventData(int32_t key, bool altDown, bool ctrlDown, bool shiftDown)
            : keyCode(key), alt(altDown), control(ctrlDown), shift(shiftDown) {}
    };

    /**
     * @brief Mouse event data
     */
    struct MouseEventData : public EventData {
        float x;
        float y;
        MouseEventData(float posX, float posY) : x(posX), y(posY) {}
    };

    /**
     * @brief Mouse button event data
     */
    struct MouseButtonEventData : public MouseEventData {
        int32_t button;
        MouseButtonEventData(float posX, float posY, int32_t btn)
            : MouseEventData(posX, posY), button(btn) {}
    };

    /**
     * @brief Resource event data
     */
    struct ResourceEventData : public EventData {
        std::string resourcePath;
        std::string resourceType;
        explicit ResourceEventData(const std::string &path,
                                   const std::string &type)
            : resourcePath(path), resourceType(type) {}
    };

    /**
     * @brief Scene event data
     */
    struct SceneEventData : public EventData {
        std::string sceneName;
        explicit SceneEventData(const std::string &name) : sceneName(name) {}
    };

}  // namespace Engine
