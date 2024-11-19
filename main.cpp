#include "raylib.h"
#include "raymath.h"

#include <cstdint>


namespace woc {
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    using f32 = float;
    using f64 = double;

    struct Window {
        u32 width;
        u32 height;
    };

    Window window_init() {
        constexpr u32 w = 1280;
        constexpr u32 h = 720;
        InitWindow(static_cast<i32>(w), static_cast<i32>(h), "Winds of Change");
        return Window{ .width = w, .height = h };
    }

    bool window_is_running([[maybe_unusued]]Window& window) {
        return !WindowShouldClose();
    }

    void window_deinit(Window& window) {
        CloseWindow();
    }

    Vector2 window_size(Window& window) {
        auto result = Vector2{ (f32) window.width, (f32) window.height };
        return result;
    }

    struct Radian {
        f32 val;
    };
    struct Degree {
        f32 val;
    };

    struct Camera {
        Vector2 pos;
        f32 height;
        Radian rot;
        f32 zoom;
    };

    struct Renderer {

    };

    void renderer_prepare_rendering([[maybe_unused]]Renderer& renderer) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
    }

    void renderer_render_world(Renderer& renderer, Camera& cam, Vector2 framebuffer_size) {
        BeginMode2D(Camera2D{
            .offset = Vector2Scale(framebuffer_size, 0.5),
            .target = cam.pos,
            .rotation = cam.rot.val * RAD2DEG,
            .zoom = (framebuffer_size.y / cam.height) * cam.zoom
        });

        //auto screen_pos = Vector2Transform(Vector2{ 100.0, 0.0 }, camera_matrix);
        auto pos = Vector2{ 0, 0 };
        DrawText("Congrats! You created your first window!", (i32)pos.x, (i32)pos.y, 20, LIGHTGRAY);
        DrawRectangle(pos.x, pos.y, 20, 20, RED);

        EndMode2D();
    }

    void renderer_finalize_rendering([[maybe_unused]]Renderer& renderer) {
        EndMode2D();
        EndDrawing();
    }

    struct GameState {
        Camera cam;
    };
}


int main()
{
    auto window = woc::window_init();
    auto renderer = woc::Renderer{};
    auto game_state = woc::GameState{
        .cam = woc::Camera {
            .pos = Vector2 { 0.0, 0.0 },
            .height = 1000,
            .rot = woc::Radian { PI / 2.0f },
            .zoom = 1.0f,
        }
    };

    // TODO: Edit raylib config and disable rmodels

    while (woc::window_is_running(window))
    {
        woc::renderer_prepare_rendering(renderer);

        woc::renderer_render_world(renderer, game_state.cam, woc::window_size(window));

        woc::renderer_finalize_rendering(renderer);
        
    }

    // Likely unnecessary before a program exit. OS cleans up. 
    woc::window_deinit(window);

    return 0;
}