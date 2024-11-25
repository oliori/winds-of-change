#include "windsofchange.cpp"
#include "window.cpp"

// KNOWN ISSUES:
// - Input lag. Move to another thread?

namespace woc {
    constexpr Vector2 WORLD_MIN = Vector2{ -400, -500 };
    constexpr Vector2 WORLD_MAX = Vector2{ 400, 500 };
    constexpr f32 PLAYER_WORLD_Y = 400.f;
    constexpr f32 PLAYER_DEFAULT_SIZE = 25.f;
    constexpr f32 ENEMY_DEFAULT_SIZE = 25.f;
    constexpr f32 ENEMIES_WORLD_Y = -400.f;
    constexpr u16 ENEMIES_MAX_ROWS = 3;
    constexpr u16 ENEMIES_MAX_COLUMNS = 10;

    struct Camera {
        Vector2 pos;
        f32 height;
        Radian rot;
        f32 zoom;
    };
    
    struct RicochetState
    {
        bool is_active;
        f32 channel_alpha;
        f32 size;
    };
    struct ChangeWindState
    {
        f32 channel_time;
        f32 size;
    };

    struct PlayerState {
        f32 health;
        f32 pos_x;
        f32 vel;
        f32 accel;
        f32 size;

        RicochetState ricochet_state;
        ChangeWindState change_wind_state;
    };
    
    struct EnemyState {
        f32 health;
        f32 shoot_cd;
    };

    struct GameInputState {
        i32 move_dir;
        bool start_ricochet_ability;
        bool stop_ricochet_ability;
        i32 wind_dir_x;
        i32 wind_dir_y;
    };

    struct Projectile
    {
        f32 damage;
        f32 size;
        Vector2 pos;
        Vector2 dir;
    };

    enum class MenuPageType
    {
        MainMenu,
        Settings,
        Quit
    };
    struct MenuState
    {
        MenuPageType current_page;
    };

    struct GameState {
        f32 time_scale = 1.0f;
        GameInputState input;
        PlayerState player;
        Camera cam;
        std::optional<EnemyState> enemies[ENEMIES_MAX_ROWS][ENEMIES_MAX_COLUMNS];
        std::vector<Projectile> player_projectiles;
        std::vector<Projectile> enemy_projectiles;
    };

    struct GameFrameTempData
    {
        std::vector<u32> hit_indices;
    };
    
    struct Renderer {
    };

    enum class AppFlags : u32
    {
        None = 0,
        HasRunningGame = 1 << 0,
        IsInGame = 1 << 1,
        NewGame = 1 << 2,
    };
    AppFlags app_flags_if(AppFlags flags, bool condition)
    {
        return static_cast<AppFlags>(static_cast<u32>(condition) * static_cast<u32>(flags));
    }
    bool app_has_all_flags(AppFlags& flags, AppFlags value)
    {
        return (static_cast<u32>(flags) & static_cast<u32>(value)) == static_cast<u32>(value);
    }
    bool app_has_any_flags(AppFlags& flags, AppFlags value)
    {
        return (static_cast<u32>(flags) & static_cast<u32>(value));
    }
    void app_add_flags(AppFlags& flags, AppFlags value)
    {
        flags = static_cast<AppFlags>(static_cast<u32>(flags) | static_cast<u32>(value));
    }
    void app_remove_flags(AppFlags& flags, AppFlags value)
    {
        flags = static_cast<AppFlags>(static_cast<u32>(flags) & (~static_cast<u32>(value)));
    }
    void app_toggle_flags(AppFlags& flags, AppFlags value)
    {
        flags = static_cast<AppFlags>(static_cast<u32>(flags) ^ static_cast<u32>(value));
    }
    
    struct AppState
    {
        AppFlags flags;
        Renderer renderer;
        MenuState menu_state;
        GameState game_state;
        Window window;
    };
    
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

    GameState game_init()
    {
        auto result = woc::GameState{
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
            }
        };
        
        for (auto i = 0; i < woc::ENEMIES_MAX_ROWS; i++)
        {
            for (auto j = 0; j < woc::ENEMIES_MAX_COLUMNS; j++)
            {
                result.enemies[i][j] = woc::EnemyState {
                    .health = 10.f
                };
            }
        }

        return result;
    }

    void game_input(GameInputState& input) {
        auto move_left = IsKeyDown(KEY_A);
        auto move_right = IsKeyDown(KEY_D);
        input.move_dir = static_cast<i32>(move_right) - static_cast<i32>(move_left);
        
        auto wind_up = IsKeyDown(KEY_UP);
        auto wind_down = IsKeyDown(KEY_DOWN);
        auto wind_left = IsKeyDown(KEY_LEFT);
        auto wind_right = IsKeyDown(KEY_RIGHT);
        input.wind_dir_x = static_cast<i32>(wind_right) - static_cast<i32>(wind_left);
        input.wind_dir_y = static_cast<i32>(wind_up) - static_cast<i32>(wind_down);
        
        input.start_ricochet_ability = IsKeyPressed(KEY_SPACE);
        input.stop_ricochet_ability = IsKeyReleased(KEY_SPACE);
    }

    void game_update(GameState& game_state, f32 delta_seconds) {
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

        game_state.player.accel = (f32)game_state.input.move_dir * PLAYER_ACCELERATION;
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
        ricochet.is_active |= game_state.input.start_ricochet_ability;
        if (game_state.input.stop_ricochet_ability || ricochet.channel_alpha >= 1.0f)
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
        if (ricochet.is_active | game_state.input.start_ricochet_ability)
        {
            auto ricochet_size = Lerp(game_state.player.size + ricochet.size * 0.25f, game_state.player.size + ricochet.size, ricochet.channel_alpha);
            DrawCircle(static_cast<i32>(player.pos_x), static_cast<i32>(PLAYER_WORLD_Y), ricochet_size, PINK);
        }
        DrawCircle(static_cast<i32>(player.pos_x), static_cast<i32>(PLAYER_WORLD_Y), player.size, RED);


        for (u16 r = 0; r < ENEMIES_MAX_ROWS; r++)
        {
            for (u16 c = 0; c < ENEMIES_MAX_COLUMNS; c++)
            {
                if (auto enemy = game_state.enemies[r][c])
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
    
    void renderer_update_and_render_menu(Renderer& renderer, MenuState& menu_state, Vector2 framebuffer_size, AppFlags& flags)
    {
        auto button_spacing = 10.f;
        auto button_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 200.f, 50.f }, Vector2{0.5f, 0.0f});
        if (GuiButton(button_rect, "NEW GAME"))
        {
            app_add_flags(flags, AppFlags::NewGame);
            app_add_flags(flags, AppFlags::IsInGame);
        }
        button_rect.y += button_rect.height + button_spacing;
        if (app_has_any_flags(flags, AppFlags::HasRunningGame))
        {
            if (GuiButton(button_rect, "CONTINUE"))
            {
                app_add_flags(flags, AppFlags::IsInGame);
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
    auto app = woc::AppState {
        .menu_state = woc::MenuState {
            .current_page = woc::MenuPageType::MainMenu,
        },
        .window = woc::window_init(),
    };

    // TODO: Edit raylib config and disable rmodels

    bool keep_running_app = woc::window_is_running(app.window);
    while (keep_running_app)
    {
        auto delta_seconds = woc::window_delta_seconds(app.window);
        PollInputEvents();
        if (IsKeyPressed(KEY_ESCAPE))
        {
            woc::app_toggle_flags(app.flags, woc::app_flags_if(woc::AppFlags::IsInGame, woc::app_has_any_flags(app.flags, woc::AppFlags::HasRunningGame)));
        }
        if (IsKeyPressed(KEY_Y))
        {
            woc::app_add_flags(app.flags, woc::AppFlags::NewGame);
            woc::app_add_flags(app.flags, woc::AppFlags::IsInGame);
        }

        
        if (woc::app_has_any_flags(app.flags, woc::AppFlags::NewGame))
        {
            app.game_state = woc::game_init();
            woc::app_add_flags(app.flags, woc::AppFlags::HasRunningGame);
            woc::app_remove_flags(app.flags, woc::AppFlags::NewGame);
        }
        
        if (woc::app_has_any_flags(app.flags, woc::AppFlags::IsInGame))
        {
            if (app.game_state.player.health > 0.f)
            {
                woc::game_input(app.game_state.input);
                woc::game_update(app.game_state, delta_seconds);
            }
            if (woc::window_is_visible(app.window))
            {
                auto window_size = woc::window_size(app.window);
                woc::renderer_prepare_rendering(app.renderer);
                woc::renderer_render_world(app.renderer, app.game_state, window_size);
                woc::renderer_finalize_rendering(app.renderer);
            }
        } else
        {
            switch (app.menu_state.current_page)
            {
                case woc::MenuPageType::MainMenu:
                {
                    if (woc::window_is_visible(app.window))
                    {
                        auto window_size = woc::window_size(app.window);
                        woc::renderer_prepare_rendering(app.renderer);
                        woc::renderer_update_and_render_menu(app.renderer, app.menu_state, window_size, app.flags);
                        woc::renderer_finalize_rendering(app.renderer);
                    }
                    break;
                }
                case woc::MenuPageType::Settings:
                {
                    if (woc::window_is_visible(app.window))
                    {
                        auto window_size = woc::window_size(app.window);
                        woc::renderer_prepare_rendering(app.renderer);
                        woc::renderer_update_and_render_settings(app.renderer, app.menu_state, window_size);
                        woc::renderer_finalize_rendering(app.renderer);
                    }
                    break;
                }
                case woc::MenuPageType::Quit:
                {
                    keep_running_app = false;
                    break;
                }
            }
        }

        keep_running_app &= woc::window_is_running(app.window);
    }

    // Unnecessary before a program exit. OS cleans up. 
    woc::window_deinit(app.window);

    return 0;
}