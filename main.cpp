#include <thread>

#include "windsofchange.cpp"
#include "window.cpp"

// KNOWN ISSUES:
// - Input lag. Move to another thread?

namespace woc {
    woc_internal Vector2 player_pos(PlayerState& player_state)
    {
        return Vector2 { player_state.pos_x, PLAYER_WORLD_Y };
    }

    woc_internal Vector2 enemy_pos_from_row_column(u16 row, u16 column)
    {
        assert(row < ENEMIES_MAX_ROWS);
        assert(column < ENEMIES_MAX_COLUMNS);
        constexpr f32 X_MARGIN = 25.f;
        constexpr f32 TOTAL_MARGIN = X_MARGIN * static_cast<f32>(ENEMIES_MAX_COLUMNS + 1); // +1 as the final element will need spacing in both sides
        constexpr f32 X_DISTANCE_TO_NEXT = (WORLD_MAX.x - WORLD_MIN.x - TOTAL_MARGIN) / static_cast<f32>(ENEMIES_MAX_COLUMNS);
        constexpr f32 Y_DISTANCE_TO_NEXT = 100.f;
        auto result = Vector2 {
            .x = WORLD_MIN.x + X_MARGIN + static_cast<f32>(column) * (X_MARGIN + X_DISTANCE_TO_NEXT),
            .y = ENEMIES_WORLD_Y + static_cast<f32>(row) * Y_DISTANCE_TO_NEXT
        };
        return result;
    }
    
    void game_load_level(GameState& game_state)
    {
        auto& l = LEVELS.at(game_state.current_level);
        game_state.enemies = l.enemies;
    }

    GameState game_init()
    {
        auto result = woc::GameState{
            .current_level = 0,
            .time_scale = 1.0,
            .player = woc::PlayerState {
                .health = 100.f,
                .pos_x = 0.f,
                .size = PLAYER_DEFAULT_SIZE,
                .ricochet_state = {
                    .size = 50.f
                },
            },
            .cam = woc::Camera {
                .pos = Vector2 { 0.0, 0.0 },
                .height = woc::WORLD_MAX.y - woc::WORLD_MIN.y,
                .rot = woc::Radian { 0.0 },
                .zoom = 1.0f,
            },
            .player_projectiles = {},
            .enemy_projectiles{},
        };
        game_load_level(result);
        
        return result;
    }
    
    void game_update(GameState& game_state, InputState& input, f32 delta_seconds) {
        constexpr f32 PLAYER_MIN_VEL = -250.0f;
        constexpr f32 PLAYER_MAX_VEL = 250.0f;
        constexpr f32 PLAYER_ACCELERATION = 500.0f;
        constexpr f32 GROUND_FRICTION = 200.0f;

        if (game_state.player.ricochet_state.is_active)
        {
            game_state.time_scale = 0.5f;
        } else
        {
            game_state.time_scale = 1.0f;
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
        game_state.player.pos_x = Clamp(game_state.player.pos_x, WORLD_MIN.x, WORLD_MAX.x);

        auto& ricochet = game_state.player.ricochet_state;
        ricochet.is_active |= input.start_ricochet_ability > 0;
        if (input.stop_ricochet_ability || ricochet.channel_alpha >= 1.0f)
        {
            ricochet.is_active = false;
            std::erase_if(game_state.enemy_projectiles, [player_pos = player_pos(game_state.player), player_size = game_state.player.size, &ricochet, &pp = game_state.player_projectiles] (Projectile& p)
            {
                auto actual_size = Lerp(player_size + ricochet.size * 0.25f, player_size + ricochet.size, ricochet.channel_alpha);
                auto dist_to_player_sqr = Vector2DistanceSqr(p.pos, player_pos);
                if (dist_to_player_sqr < (actual_size + p.size) * (actual_size + p.size))
                {
                    auto dir_to_p = Vector2Normalize(Vector2Subtract(p.pos, player_pos));
                    p.dir = dir_to_p;
                    // TODO: Change damage
                    pp.emplace_back(p);
                    return true;
                }
                return false;
            });
            ricochet.channel_alpha = 0.f;
        }
        if (ricochet.is_active)
        {
            ricochet.channel_alpha = std::min(1.0f, ricochet.channel_alpha + delta_seconds);
            
        }

        std::erase_if(game_state.enemy_projectiles, [delta_seconds, &player = game_state.player] (Projectile& projectile)
        {
            constexpr f32 PROJECTILE_SPEED = 300.f;
            projectile.pos = Vector2Add(projectile.pos, Vector2Scale(projectile.dir, delta_seconds * PROJECTILE_SPEED));

            if (Vector2DistanceSqr(projectile.pos, player_pos(player)) < (player.size + projectile.size) * (player.size + projectile.size))
            {
                player.health -= projectile.damage;
                return true;
            }
            return false;
        });

        std::erase_if(game_state.player_projectiles, [delta_seconds, &enemies = game_state.enemies] (Projectile& projectile)
        {
            constexpr f32 PROJECTILE_SPEED = 300.f;
            projectile.pos = Vector2Add(projectile.pos, Vector2Scale(projectile.dir, delta_seconds * PROJECTILE_SPEED));

            for (u16 r = 0; r < ENEMIES_MAX_ROWS; r++)
            {
                for (u16 c = 0; c < ENEMIES_MAX_COLUMNS; c++)
                {
                    auto& e = enemies[r][c];
                    if (e && Vector2DistanceSqr(projectile.pos, enemy_pos_from_row_column(r, c)) < (ENEMY_DEFAULT_SIZE + projectile.size) * (ENEMY_DEFAULT_SIZE + projectile.size))
                    {
                        e->health -= projectile.damage;
                        return true;
                    }
                }
            }

            return false;
        });

        constexpr f32 DEFAULT_ENEMY_SHOOT_CD = 15.f;
        for (u16 r = 0; r < ENEMIES_MAX_ROWS; r++)
        {
            for (u16 c = 0; c < ENEMIES_MAX_COLUMNS; c++)
            {
                if (auto& enemy = game_state.enemies[r][c])
                {
                    if (enemy->health <= 0.f)
                    {
                        enemy.reset();
                        continue;
                    }
                    
                    enemy->shoot_cd -= delta_seconds;
                    if (enemy->shoot_cd <= 0.f)
                    {
                        enemy->shoot_cd = DEFAULT_ENEMY_SHOOT_CD;
                        auto enemy_pos = enemy_pos_from_row_column(r, c);
                        auto projectile_dir = Vector2Normalize(Vector2Subtract(player_pos(game_state.player), enemy_pos));
                        game_state.enemy_projectiles.emplace_back(Projectile {
                            .damage = 10.f,
                            .pos = enemy_pos,
                            .dir = projectile_dir
                        });
                    }
                }
            }
        }

        bool all_enemies_dead = true;
        for (auto& enemy_row : game_state.enemies)
        {
            for (auto& enemy : enemy_row)
            {
                all_enemies_dead &= !enemy;
            }
        }
        if (all_enemies_dead)
        {
            std::cout << "LEVEL COMPLETE" << std::endl;
            game_state.current_level++;
            if (game_state.current_level >= LEVELS.size())
            {
                // TODO: Win screen
                assert(false);
            } else
            {
                game_load_level(game_state);
            }
        }
    }

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

        auto& ricochet = game_state.player.ricochet_state;
        if (ricochet.is_active)
        {
            auto ricochet_size = Lerp(game_state.player.size + ricochet.size * 0.25f, game_state.player.size + ricochet.size, ricochet.channel_alpha);
            DrawCircle(static_cast<i32>(player.pos_x), static_cast<i32>(PLAYER_WORLD_Y), ricochet_size, PINK);
        }
        DrawCircle(static_cast<i32>(player.pos_x), static_cast<i32>(PLAYER_WORLD_Y), player.size, RED);


        for (u16 r = 0; r < ENEMIES_MAX_ROWS; r++)
        {
            for (u16 c = 0; c < ENEMIES_MAX_COLUMNS; c++)
            {
                if (game_state.enemies[r][c])
                {
                    auto pos = enemy_pos_from_row_column(r, c);
                    auto px = static_cast<i32>(pos.x);
                    auto py = static_cast<i32>(pos.y);
                    DrawCircle(px, py, ENEMY_DEFAULT_SIZE, BLUE);
                }
            }
        }

        for (auto& projectile : game_state.enemy_projectiles)
        {
            auto px = static_cast<i32>(projectile.pos.x);
            auto py = static_cast<i32>(projectile.pos.y);
            DrawCircle(px, py, 10, SKYBLUE);
        }
        
        for (auto& projectile : game_state.player_projectiles)
        {
            auto px = static_cast<i32>(projectile.pos.x);
            auto py = static_cast<i32>(projectile.pos.y);
            DrawCircle(px, py, 10, PINK);
        }

        EndMode2D();
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
    
    void renderer_update_and_render_menu(Renderer& renderer, MenuState& menu_state, std::optional<GameState>& game_state, Vector2 framebuffer_size)
    {
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

    void renderer_finalize_rendering([[maybe_unused]]Renderer& renderer) {
        EndDrawing();
    }
}

int main()
{
    // Doesn't need smart pointers as when it goes out of scope, the app exits and OS cleans it up
    auto menu_state = woc::MenuState {
        .current_page = woc::MenuPageType::MainMenu,
        .last_menu_page = woc::MenuPageType::MainMenu,
    };
    auto window = woc::window_init();
    auto renderer = woc::Renderer{};
    auto game_state = std::optional<woc::GameState>{};

    // TODO: Edit raylib config and disable rmodels

    bool keep_running_app = true;
    bool is_window_visible = true;
    Vector2 window_size = woc::window_size(window);
    woc::InputState app_input_state{};
    auto update_app = [&window = window, &keep_running_app, &is_window_visible, &window_size, &app_input_state] ()
    {
        PollInputEvents();
        
        app_input_state.game_menu_swap += IsKeyPressed(KEY_ESCAPE);
        app_input_state.new_game += IsKeyPressed(KEY_Y);
        
        auto move_left = IsKeyDown(KEY_A);
        auto move_right = IsKeyDown(KEY_D);
        app_input_state.move_dir = static_cast<woc::i32>(move_right) - static_cast<woc::i32>(move_left);
        
        auto wind_up = IsKeyDown(KEY_UP);
        auto wind_down = IsKeyDown(KEY_DOWN);
        auto wind_left = IsKeyDown(KEY_LEFT);
        auto wind_right = IsKeyDown(KEY_RIGHT);
        app_input_state.wind_dir_x = static_cast<woc::i32>(wind_right) - static_cast<woc::i32>(wind_left);
        app_input_state.wind_dir_y = static_cast<woc::i32>(wind_up) - static_cast<woc::i32>(wind_down);
        
        app_input_state.start_ricochet_ability += IsKeyPressed(KEY_SPACE);
        app_input_state.stop_ricochet_ability += IsKeyReleased(KEY_SPACE);

        is_window_visible = woc::window_is_visible(window);
        window_size = woc::window_size(window);

        if (!woc::window_is_running(window))
        {
            keep_running_app = false;
        }
    };

    auto update_game = [&menu_state, &keep_running_app, &game_state, visible = &is_window_visible, window_size = &window_size, &renderer] (woc::InputState input)
    {
        if (input.new_game)
        {
            game_state = woc::game_init();
        }
        
        if (input.game_menu_swap % 2 == 1)
        {
            if (menu_state.current_page == woc::MenuPageType::Game)
            {
                menu_state.current_page = menu_state.last_menu_page;
            } else
            {
                menu_state.last_menu_page = menu_state.current_page;
                menu_state.current_page = woc::MenuPageType::Game;
            }
        }
        
        auto delta_seconds = GetFrameTime();
        switch (menu_state.current_page)
        {
            case woc::MenuPageType::MainMenu:
            {
                if (*visible)
                {
                    woc::renderer_prepare_rendering(renderer);
                    woc::renderer_update_and_render_menu(renderer, menu_state, game_state, *window_size);
                    woc::renderer_finalize_rendering(renderer);
                }
                break;
            }
            case woc::MenuPageType::Game:
            {
                if (game_state->player.health > 0.f)
                {
                    woc::game_update(*game_state, input, delta_seconds);
                }
                if (*visible)
                {
                    woc::renderer_prepare_rendering(renderer);
                    woc::renderer_render_world(renderer, *game_state, *window_size);
                    woc::renderer_finalize_rendering(renderer);
                }
                break;
            }
            case woc::MenuPageType::Settings:
            {
                if (*visible)
                {
                    woc::renderer_prepare_rendering(renderer);
                    woc::renderer_update_and_render_settings(renderer, menu_state, *window_size);
                    woc::renderer_finalize_rendering(renderer);
                }
                break;
            }
            case woc::MenuPageType::Quit:
            {
                keep_running_app = false;
                break;
            }
        }
    };

    woc::InputState game_input_state{};
    while (keep_running_app)
    {
        update_app();
        
        // Create delta from current app thread's state
        auto temp = app_input_state;
        auto input = temp;
        input.game_menu_swap -= game_input_state.game_menu_swap;
        input.new_game -= game_input_state.new_game;
        input.start_ricochet_ability -= game_input_state.start_ricochet_ability;
        input.stop_ricochet_ability -= game_input_state.stop_ricochet_ability;
        game_input_state = temp;
        
        update_game(input);
    }

    // Unnecessary before a program exit. OS cleans up. 
    woc::window_deinit(window);

    return 0;
}