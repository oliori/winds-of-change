﻿#include "windsofchange.h"

#include <algorithm>

namespace woc
{
    woc_internal Vector2 player_pos(PlayerState& player_state)
    {
        return Vector2 { player_state.pos_x, PLAYER_WORLD_Y };
    }
    
    woc_internal Vector2 player_size()
    {
        return Vector2 { static_cast<f32>(PLAYER_DEFAULT_WIDTH), static_cast<f32>(PLAYER_DEFAULT_HEIGHT) };
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
        auto& enemies = game_state.enemies;
        enemies.clear();
        
        //constexpr Vector2 WORLD_MIN = Vector2{ -700, -500 };
        //constexpr Vector2 WORLD_MAX = Vector2{ 700, 500 };
        switch (game_state.current_level)
        {
            case 0:
            {
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -0.f, -300.f },
                    .size = Vector2 { 100.f, 25.f },
                    .health = 1,
                    .contributes_to_win = true
                });
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -0.f, -100.f },
                    .size = Vector2 { 100.f, 25.f },
                    .health = 1,
                    .contributes_to_win = true
                });
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
                .vel = 0.f,
                .accel = 0.f,
                .balls_available = 1,
            },
            .cam = woc::Camera {
                .pos = Vector2 { 0.0, 0.0 },
                .height = woc::WORLD_MAX.y - woc::WORLD_MIN.y,
                .rot = woc::Radian { 0.0 },
                .zoom = 1.0f,
            },
            .enemies = {},
            .player_projectiles = {},
        };
        game_load_level(result);
        
        return result;
    }

    woc_internal bool sphere_collides_sphere(
        Vector2 sphere_pos1, f32 sphere_radius1,
        Vector2 sphere_pos2, f32 sphere_radius2)
    {
        auto combined_r = sphere_radius1 + sphere_radius2;
        auto dist_sqr = Vector2DistanceSqr(sphere_pos1, sphere_pos2);
        return dist_sqr <= combined_r * combined_r;
    }

    // Returns normal of collision or zero vector if no collision.
    woc_internal Vector2 sphere_collides_rectangle(
        Vector2 sphere_pos, f32 sphere_radius,
        Vector2 rectangle_pos, Vector2 rectangle_size, Radian rectangle_rotation)
    {
        
        auto result = Vector2Zero();
        
        auto half_size = Vector2Scale(rectangle_size, 0.5f);
        auto s_pos2 = Vector2Subtract(sphere_pos, rectangle_pos);
        s_pos2 = Vector2Rotate(s_pos2, -rectangle_rotation.val);

        bool inside_x = false;
        bool inside_y = false;
        Vector2 test_point = s_pos2;
        if (s_pos2.x < -half_size.x) // LEFT
        {
            test_point.x = -half_size.x;
            result.x = -1.f;
        } else if (s_pos2.x > half_size.x) // RIGHT
        {
            test_point.x = half_size.x;
            result.x = 1.f;
        } else
        {
            inside_x = true;
        }

        if (s_pos2.y > half_size.y) // ABOVE
        {
            test_point.y = half_size.y;
            result.y = 1.f;
        } else if (s_pos2.y < -half_size.y) // BELOW
        {
            test_point.y = -half_size.y;
            if (Vector2DistanceSqr(s_pos2, Vector2 { s_pos2.x, -half_size.y }) < sphere_radius * sphere_radius)
            {
                result.y = -1.f;
            }
        } else
        {
            inside_y = true;
        }

        // INSIDE
        if (inside_y && inside_x)
        {
            // For simplicity just return up vector now if we get inside the rectangle
            // Ideally this should find the intersection point using the velocity
            result = Vector2 { 0.0f, 1.0f };
        }
        
        if (Vector2DistanceSqr(s_pos2, test_point) > sphere_radius * sphere_radius)
        {
            return Vector2Zero();
        }

        return Vector2Rotate(result, rectangle_rotation.val);
    }

    void game_update(GameState& game_state, InputState& input, f32 delta_seconds)
    {
        constexpr f32 PLAYER_MIN_VEL = -750.0f;
        constexpr f32 PLAYER_MAX_VEL = 750.0f;
        constexpr f32 PLAYER_ACCELERATION = 1500.0f;
        constexpr f32 GROUND_FRICTION = 750.0f;

        if (game_state.level_complete)
        {
            game_state.time_scale = std::max(game_state.time_scale - delta_seconds, 0.f);
        }
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
        game_state.player.pos_x = Clamp(game_state.player.pos_x, WORLD_MIN.x + static_cast<f32>(PLAYER_DEFAULT_WIDTH) * 0.5f, WORLD_MAX.x - static_cast<f32>(PLAYER_DEFAULT_WIDTH) * 0.5f);

        for (auto& p : game_state.player_projectiles)
        {
            p.pos = Vector2Add(p.pos, Vector2Scale(p.dir, BALL_DEFAULT_VELOCITY * delta_seconds));

            for (auto& e : game_state.enemies)
            {
                auto collision_normal = sphere_collides_rectangle(p.pos, BALL_DEFAULT_RADIUS, e.pos, e.size, Radian { 0.0f });
                if (!Vector2Equals(collision_normal, Vector2Zero()))
                {
                    p.dir = Vector2Reflect(p.dir, collision_normal);
                    e.health--;
                    break;
                }
            }
            
            auto collision_normal = sphere_collides_rectangle(p.pos, BALL_DEFAULT_RADIUS, player_pos(game_state.player), player_size(), Radian { 0.0f });
            if (!Vector2Equals(collision_normal, Vector2Zero()))
            {
                p.dir = Vector2Reflect(p.dir, collision_normal);
                break;
            }
        }

        std::erase_if(game_state.enemies, [] (EnemyState& e)
        {
            return e.health <= 0;
        });

        if (input.send_ball & game_state.player.balls_available)
        {
            game_state.player_projectiles.emplace_back(Projectile {
                .pos = Vector2Add(player_pos(game_state.player), Vector2 { 0.f, static_cast<f32>(-BALL_DEFAULT_Y_OFFSET) }),
                .dir = Vector2 { 0, -1 }
            });
            game_state.player.balls_available--;
        }
        
        game_state.level_complete = !std::ranges::any_of(game_state.enemies, [] (EnemyState& e) { return e.contributes_to_win; });
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
        
        for (auto& e : game_state.enemies)
        {
            auto ex = static_cast<i32>(e.pos.x);
            auto ey = static_cast<i32>(e.pos.y);
            auto half_size = Vector2Scale(e.size, 0.5f);
            DrawRectangle(ex - static_cast<i32>(half_size.x), ey - static_cast<i32>(half_size.y), static_cast<i32>(e.size.x), static_cast<i32>(e.size.y), RED);
        }
        
        if (game_state.player.balls_available)
        {
            DrawCircleLines(px, py - BALL_DEFAULT_Y_OFFSET, BALL_DEFAULT_RADIUS, PINK);
        }
        
        for (auto& projectile : game_state.player_projectiles)
        {
            DrawCircle(static_cast<i32>(projectile.pos.x), static_cast<i32>(projectile.pos.y), BALL_DEFAULT_RADIUS, PINK);
        }

        EndMode2D();

        i32 balls_available_spacing = 20;
        i32 balls_available_size = 30;
        i32 balls_available_pos_x = framebuffer_size.x - balls_available_spacing - balls_available_size;
        i32 balls_available_pos_y = framebuffer_size.y - balls_available_spacing - balls_available_size;
        for (u32 i = 0; i < game_state.player.balls_available; i++)
        {
            DrawCircle(balls_available_pos_x, balls_available_pos_y, balls_available_size, PINK);
            balls_available_pos_x -= balls_available_spacing + balls_available_size;
        }
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
