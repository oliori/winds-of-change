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

    void window_set_fullscreen(Window& window, bool fullscreen)
    {
        if (fullscreen != window_is_fullscreen(window))
        {
            ToggleFullscreen();
        }
    }

    bool window_is_fullscreen(Window& window)
    {
        return IsWindowFullscreen();
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

    void window_set_size(Window& window, Vector2 size)
    {
        assert(size.x > 0.f);
        assert(size.y > 0.f);
        window.width = static_cast<u32>(static_cast<i32>(size.x));
        window.height = static_cast<u32>(static_cast<i32>(size.y));
        SetWindowSize(static_cast<i32>(window.width), static_cast<i32>(window.height));
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
