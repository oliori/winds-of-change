#include "window.h"

namespace woc
{
    Window window_init()
    {
        constexpr u32 w = 1920;
        constexpr u32 h = 1080;
        InitWindow(w, h, "Winds of Change");
        // ESC is used for menu navigation, so it should not be the exit key
        SetExitKey(0);
        return Window{ .width = w, .height = h };
    }

    void window_close(Window& window)
    {
        CloseWindow();
    }

    bool window_is_running(Window& window)
    {
        return !WindowShouldClose();
    }

    bool window_is_visible(Window& window)
    {
        return !IsWindowHidden();
    }

    void window_deinit(Window& window)
    {
        CloseWindow();
    }

    Vector2 window_size(Window& window)
    {
        auto result = Vector2{ static_cast<f32>(window.width), static_cast<f32>(window.height) };
        return result;
    }

    f32 window_delta_seconds(Window& window)
    {
        return GetFrameTime();
    }

    f32 window_seconds_since_init(Window& window)
    {
        return static_cast<f32>(GetTime());
    }
}
