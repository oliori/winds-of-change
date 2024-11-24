#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>
#include <cassert>

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

#define woc_internal static
#define woc_global static
#define woc_local static

    constexpr Vector2 WORLD_MIN = Vector2{ -400, -500 };
    constexpr Vector2 WORLD_MAX = Vector2{ 400, 500 };
    constexpr f32 PLAYER_WORLD_Y = 400.f;
    constexpr f32 PLAYER_DEFAULT_SIZE = 50.f;
    constexpr f32 ENEMIES_WORLD_Y = -400.f;
    constexpr u16 ENEMIES_MAX_ROWS = 3;
    constexpr u16 ENEMIES_MAX_COLUMNS = 10;

    struct Window {
        u32 width;
        u32 height;
    };

    Window window_init() {
        constexpr u32 w = 1920;
        constexpr u32 h = 1080;
        InitWindow(w, h, "Winds of Change");
        // ESC is used for menu navigation, so it should not be the exit key
        SetExitKey(0);
        return Window{ .width = w, .height = h };
    }
    
    void window_close([[maybe_unusued]]Window& window) {
        CloseWindow();
    }

    bool window_is_running([[maybe_unusued]]Window& window) {
        return !WindowShouldClose();
    }
    
    bool window_is_visible([[maybe_unusued]]Window& window) {
        return !IsWindowHidden();
    }

    void window_deinit(Window& window) {
        CloseWindow();
    }

    Vector2 window_size(Window& window) {
        auto result = Vector2{ static_cast<f32>(window.width), static_cast<f32>(window.height) };
        return result;
    }

    f32 window_delta_seconds(Window& window) {
        return GetFrameTime();
    }

    f32 window_seconds_since_init(Window& window) {
        return static_cast<f32>(GetTime());
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
        f32 health;
        f32 pos_x;
        f32 vel;
        f32 accel;
        f32 size;
    };
    
    struct EnemyState {
        f32 health;
        f32 shoot_cd;
    };

    struct GameInputState {
        i32 move_dir;
        bool use_ability_1;
    };

    struct Projectile
    {
        f32 damage;
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
                .size = PLAYER_DEFAULT_SIZE
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
        input.use_ability_1 = IsKeyPressed(KEY_SPACE);
    }

    void game_update(GameState& game_state, f32 delta_seconds) {
        constexpr f32 PLAYER_MIN_VEL = -250.0f;
        constexpr f32 PLAYER_MAX_VEL = 250.0f;
        constexpr f32 PLAYER_ACCELERATION = 500.0f;
        constexpr f32 GROUND_FRICTION = 200.0f;

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

        if (game_state.input.use_ability_1)
        {
            std::optional<u32> chosen_projectile{};
            auto& enemy_projectiles = game_state.enemy_projectiles; 
            constexpr f32 RICHOCHET_RADIUS = 100.f;
            auto closest_radius_sqr = RICHOCHET_RADIUS * RICHOCHET_RADIUS;
            for (size_t i = 0; i < enemy_projectiles.size(); i++)
            {
                auto& p = enemy_projectiles[i];
                auto dist_to_player_sqr = Vector2DistanceSqr(p.pos, player_pos(game_state.player));
                if (dist_to_player_sqr < closest_radius_sqr)
                {
                    closest_radius_sqr = dist_to_player_sqr;
                    chosen_projectile = i;
                }
            }

            if (chosen_projectile)
            {
                auto& projectile = enemy_projectiles[*chosen_projectile];
                auto projectile_dir = Vector2Normalize(Vector2Subtract(projectile.pos, player_pos(game_state.player)));
                projectile.dir = projectile_dir;
                game_state.player_projectiles.emplace_back(projectile);
                
                std::swap(projectile, enemy_projectiles.back());
                enemy_projectiles.pop_back();
            }
        }

        for (auto& projectile : game_state.enemy_projectiles)
        {
            constexpr f32 PROJECTILE_SPEED = 150.f;
            projectile.pos = Vector2Add(projectile.pos, Vector2Scale(projectile.dir, delta_seconds * PROJECTILE_SPEED));
            
            if (Vector2DistanceSqr(projectile.pos, player_pos(game_state.player)) < game_state.player.size)
            {
                game_state.player.health -= projectile.damage;
            }
        }

        constexpr f32 DEFAULT_ENEMY_SHOOT_CD = 15.f;
        for (u16 r = 0; r < ENEMIES_MAX_ROWS; r++)
        {
            for (u16 c = 0; c < ENEMIES_MAX_COLUMNS; c++)
            {
                if (auto& enemy = game_state.enemies[r][c])
                {
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

        DrawCircle(static_cast<i32>(player.pos_x), static_cast<i32>(PLAYER_WORLD_Y), player.size / 2.0f, RED);


        for (u16 r = 0; r < ENEMIES_MAX_ROWS; r++)
        {
            for (u16 c = 0; c < ENEMIES_MAX_COLUMNS; c++)
            {
                if (auto enemy = game_state.enemies[r][c])
                {
                    auto pos = enemy_pos_from_row_column(r, c);
                    auto px = static_cast<i32>(pos.x);
                    auto py = static_cast<i32>(pos.y);
                    DrawCircle(px, py, 25, BLUE);
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
                default: assert(false);
            }
        }

        keep_running_app &= woc::window_is_running(app.window);
    }

    // Unnecessary before a program exit. OS cleans up. 
    woc::window_deinit(app.window);

    return 0;
}