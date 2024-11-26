#pragma once

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>
#include <array>
#include <cassert>

#define woc_internal static
#define woc_global static
#define woc_local static

namespace woc
{
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
    
    constexpr f32 WIND_DURATION = 0.75f;
    constexpr u32 START_LEVEL = 1;
    constexpr u32 END_LEVEL = 1;
    constexpr Vector2 WORLD_MIN = Vector2{ -700, -500 };
    constexpr Vector2 WORLD_MAX = Vector2{ 700, 500 };
    constexpr f32 PLAYER_WORLD_Y = 400.f;
    constexpr i32 PLAYER_DEFAULT_WIDTH = 100;
    constexpr i32 PLAYER_DEFAULT_HEIGHT = 25;
    constexpr f32 BALL_DEFAULT_VELOCITY = 200.f;
    constexpr i32 BALL_DEFAULT_RADIUS = 10;
    constexpr i32 BALL_DEFAULT_Y_OFFSET = 25;
    
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

    struct WindAbility
    {
        f32 timer;
        Radian angle;
        f32 ball_current_velocity;
        f32 ball_target_velocity;
    };

    struct PlayerState {
        f32 pos_x;
        f32 vel;
        f32 accel;
        f32 ball_velocity = BALL_DEFAULT_VELOCITY;
        u32 balls_available;
        u32 wind_available;
        std::optional<WindAbility> active_wind_ability;
    };

    enum class EnemyType
    {
        Indestructible,
        Normal,
    };
    struct EnemyState {
        Vector2 pos;
        Vector2 size;
        i32 health;
        EnemyType type;
        bool contributes_to_win;
    };

    struct InputState {
        i32 move_dir;
        i32 wind_dir_x;
        i32 wind_dir_y;
        
        u32 send_ball;
        u32 new_game;
        u32 game_menu_swap;
    };

    struct Projectile
    {
        Vector2 pos;
        Vector2 dir;
    };

    enum class MenuPageType
    {
        MainMenu,
        Game,
        Settings,
        Quit
    };
    struct MenuState
    {
        MenuPageType current_page;
        MenuPageType last_menu_page;
    };

    enum class LevelStatus
    {
        InProgress,
        Lost,
        Won
    };

    struct GameState {
        u32 current_level = 0; 
        f32 time_scale = 1.0f;
        LevelStatus level_status;
        PlayerState player;
        Camera cam;
        std::vector<EnemyState> enemies;
        std::vector<Projectile> player_projectiles;
    };
    GameState game_init();
    void game_update(GameState& game_state, InputState& input, f32 delta_seconds);

    struct Renderer {
    };
    void renderer_update_and_render_menu(Renderer& renderer, MenuState& menu_state, std::optional<GameState>& game_state, Vector2 framebuffer_size);
    void renderer_update_and_render_settings(Renderer& renderer, MenuState& menu_state, Vector2 framebuffer_size);
    void renderer_render_world(Renderer& renderer, GameState& game_state, Vector2 framebuffer_size);
    void renderer_render_level_fail(Renderer& renderer, GameState& game_state, Vector2 framebuffer_size);
    void renderer_render_level_complete(Renderer& renderer, GameState& game_state, Vector2 framebuffer_size);
    void renderer_render_game_won(Renderer& renderer, MenuState& menu_state, GameState& game_state, Vector2 framebuffer_size);
    void renderer_finalize_rendering(Renderer& renderer);
    void renderer_prepare_rendering(Renderer& renderer);

}
