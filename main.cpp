#include "raylib.h"
#include "raymath.h"

#include <cstdint>
#include <iostream>

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

    constexpr Vector2 WORLD_MIN = Vector2{ -250, -500 };
    constexpr Vector2 WORLD_MAX = Vector2{ 250, 500 };
    constexpr f32 PLAYER_WORLD_Y = 400.f;

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

    f32 window_delta_seconds(Window& window) {
        return GetFrameTime();
    }

    f32 window_seconds_since_init(Window& window) {
        return GetTime();
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

    struct PlayerState {
        f32 pos_x;
    };

    struct InputState {
        f32 desired_movement;
    };

    struct GameState {
        InputState input;
        PlayerState player;
        Camera cam;
    };

    void game_input(InputState& input) {
        PollInputEvents();

        auto move_left = IsKeyDown(KEY_A);
        auto move_right = IsKeyDown(KEY_D);
        input.desired_movement = (f32)move_right - (f32)move_left;
    }

    void game_update(GameState& game_state, f32 delta_seconds) {
        constexpr f32 DEFAULT_MOVE_SPEED = 250.0f;
        game_state.player.pos_x += game_state.input.desired_movement * delta_seconds * DEFAULT_MOVE_SPEED;
        game_state.player.pos_x = Clamp(game_state.player.pos_x, WORLD_MIN.x, WORLD_MAX.x);
//        std::cout << "DS: " << delta_seconds << ", movement is " << game_state.input.desired_movement << std::endl;
    }

    struct Renderer {

    };

    void renderer_prepare_rendering([[maybe_unused]]Renderer& renderer) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
    }

    void renderer_render_world(Renderer& renderer, GameState& game_state, Vector2 framebuffer_size) {
        auto& cam = game_state.cam;
        auto& player = game_state.player;

        BeginMode2D(Camera2D{
            .offset = Vector2Scale(framebuffer_size, 0.5),
            .target = cam.pos,
            .rotation = cam.rot.val * RAD2DEG,
            .zoom = (framebuffer_size.y / cam.height) * cam.zoom
        });

        //auto screen_pos = Vector2Transform(Vector2{ 100.0, 0.0 }, camera_matrix);
        auto pos = Vector2{ 0, 0 };
        DrawText("Congrats! You created your first window!", (i32)pos.x, (i32)pos.y, 20, LIGHTGRAY);

        DrawRectangle((i32)player.pos_x, (i32)PLAYER_WORLD_Y, 50, 50, RED);

        EndMode2D();
    }

    void renderer_finalize_rendering([[maybe_unused]]Renderer& renderer) {
        EndMode2D();
        EndDrawing();
    }
}

int main()
{
    auto window = woc::window_init();
    auto renderer = woc::Renderer{};
    auto game_state = woc::GameState{
        .player = woc::PlayerState {
            .pos_x = 0.f
        },
        .cam = woc::Camera {
            .pos = Vector2 { 0.0, 0.0 },
            .height = woc::WORLD_MAX.y - woc::WORLD_MIN.y,
            .rot = woc::Radian { 0.0 },
            .zoom = 1.0f,
        }
    };

    // TODO: Edit raylib config and disable rmodels

    while (woc::window_is_running(window))
    {
        auto delta_seconds = woc::window_delta_seconds(window);

        woc::game_input(game_state.input);
        woc::game_update(game_state, delta_seconds);

        woc::renderer_prepare_rendering(renderer);
        woc::renderer_render_world(renderer, game_state, woc::window_size(window));
        woc::renderer_finalize_rendering(renderer);
    }

    // Likely unnecessary before a program exit. OS cleans up. 
    woc::window_deinit(window);

    return 0;
}