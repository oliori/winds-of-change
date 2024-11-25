#include "windsofchange.h"

namespace woc
{
    woc_internal Vector2 player_pos(PlayerState& player_state)
    {
        return Vector2 { player_state.pos_x, PLAYER_WORLD_Y };
    }

    woc_internal Rectangle ui_rectangle_from_anchor(Vector2 framebuffer_size, Vector2 anchor, Vector2 size, Vector2 origin = Vector2{0.5f, 0.5f})
    {
        auto scaled_anchor = Vector2Multiply(framebuffer_size, anchor);
        auto size_part_from_origin = Vector2Multiply(origin, size);
        auto pos = Vector2Subtract(scaled_anchor, size_part_from_origin);
        auto result = Rectangle {
            .x = pos.x,
            .y = pos.y,
            .width = size.x,
            .height = size.y
        };
        return result;
    }

    void game_load_level(GameState& game_state)
    {
        switch (game_state.current_level)
        {
            case 0:
            {
                break;
            }
            default:
            {
                break;
            }
        }
    }

    GameState game_init()
    {
        auto result = woc::GameState{
            .current_level = 0,
            .time_scale = 1.0,
            .player = woc::PlayerState {
                .pos_x = 0.f,
            },
            .cam = woc::Camera {
                .pos = Vector2 { 0.0, 0.0 },
                .height = woc::WORLD_MAX.y - woc::WORLD_MIN.y,
                .rot = woc::Radian { 0.0 },
                .zoom = 1.0f,
            },
            .player_projectiles = {},
        };
        game_load_level(result);
        
        return result;
    }

    void game_update(GameState& game_state, InputState& input, f32 delta_seconds)
    {
        constexpr f32 PLAYER_MIN_VEL = -750.0f;
        constexpr f32 PLAYER_MAX_VEL = 750.0f;
        constexpr f32 PLAYER_ACCELERATION = 1500.0f;
        constexpr f32 GROUND_FRICTION = 750.0f;

        delta_seconds *= game_state.time_scale;

        game_state.player.accel = static_cast<f32>(input.move_dir) * PLAYER_ACCELERATION;
        // TODO: Friction should let you go in the opposite direction.
        if (game_state.player.vel < 0.0f) {
            game_state.player.accel += GROUND_FRICTION;
        } else {
            game_state.player.accel -= GROUND_FRICTION;
        }

        game_state.player.vel += game_state.player.accel * delta_seconds;
        game_state.player.vel = Clamp(game_state.player.vel, PLAYER_MIN_VEL, PLAYER_MAX_VEL);
        game_state.player.pos_x += game_state.player.vel * delta_seconds;
        game_state.player.pos_x = Clamp(game_state.player.pos_x, WORLD_MIN.x + static_cast<f32>(PLAYER_DEFAULT_WIDTH / 2), WORLD_MAX.x - static_cast<f32>(PLAYER_DEFAULT_WIDTH / 2));
    }

    void renderer_update_and_render_menu(Renderer& renderer, MenuState& menu_state, std::optional<GameState>& game_state, Vector2 framebuffer_size) {
        auto button_spacing = 10.f;
        auto button_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 200.f, 50.f }, Vector2{0.5f, 0.0f});
        if (GuiButton(button_rect, "NEW GAME"))
        {
            menu_state.current_page = MenuPageType::Game;
            game_state = game_init();
        }
        button_rect.y += button_rect.height + button_spacing;
        if (game_state)
        {
            if (GuiButton(button_rect, "CONTINUE"))
            {
                menu_state.current_page = MenuPageType::Game;
            }
            button_rect.y += button_rect.height + button_spacing;
        }
        if (GuiButton(button_rect, "SETTINGS"))
        {
            menu_state.current_page = MenuPageType::Settings;
        }
        button_rect.y += button_rect.height + button_spacing;
        if (GuiButton(button_rect, "QUIT"))
        {
            menu_state.current_page = MenuPageType::Quit;
        }
    }

    void renderer_update_and_render_settings(Renderer& renderer, MenuState& menu_state, Vector2 framebuffer_size)
    {
        auto button_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 200.f, 50.f }, Vector2{0.5f, 0.0f});
        if (GuiButton(button_rect, "BACK"))
        {
            menu_state.current_page = MenuPageType::MainMenu;
        }
    }

    void renderer_render_world(Renderer& renderer, GameState& game_state, Vector2 framebuffer_size)
    {
        auto& cam = game_state.cam;
        auto& player = game_state.player;

        BeginMode2D(Camera2D{
            .offset = Vector2Scale(framebuffer_size, 0.5),
            .target = cam.pos,
            .rotation = cam.rot.val * RAD2DEG,
            .zoom = (framebuffer_size.y / cam.height) * cam.zoom
        });

        auto diff = Vector2Subtract(WORLD_MAX, WORLD_MIN);
        DrawRectangle(static_cast<i32>(WORLD_MIN.x), static_cast<i32>(WORLD_MIN.y), static_cast<i32>(diff.x), static_cast<i32>(diff.y), LIGHTGRAY);

        auto px = static_cast<i32>(player.pos_x);
        auto py = static_cast<i32>(PLAYER_WORLD_Y);
        DrawRectangle(px - PLAYER_DEFAULT_WIDTH / 2, py - PLAYER_DEFAULT_HEIGHT / 2, PLAYER_DEFAULT_WIDTH, PLAYER_DEFAULT_HEIGHT, RED);
        
        for (auto& projectile : game_state.player_projectiles)
        {
            auto px = static_cast<i32>(projectile.pos.x);
            auto py = static_cast<i32>(projectile.pos.y);
            DrawCircle(px, py, 10, PINK);
        }

        EndMode2D();
    }

    void renderer_finalize_rendering(Renderer& renderer)
    {
        EndDrawing();
    }

    void renderer_prepare_rendering(Renderer& renderer)
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
    }
}
