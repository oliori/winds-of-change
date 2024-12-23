﻿#pragma once

#include "windsofchange.h"

namespace woc
{
    struct Window {
        u32 width;
        u32 height;
    };

    Window window_init();
    void window_close(Window& window);
    bool window_is_running(Window& window);
    void window_set_fullscreen(Window& window, bool fullscreen);
    bool window_is_fullscreen(Window& window);
    bool window_is_visible(Window& window);
    void window_deinit(Window& window);
    Vector2 window_size(Window& window);
    void window_set_size(Window& window, Vector2 size);
    f32 window_delta_seconds(Window& window);
    f32 window_seconds_since_init(Window& window);
}
