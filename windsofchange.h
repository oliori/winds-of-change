#pragma once

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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
    
    constexpr Vector2 WORLD_MIN = Vector2{ -400, -500 };
    constexpr Vector2 WORLD_MAX = Vector2{ 400, 500 };
    constexpr f32 PLAYER_WORLD_Y = 400.f;
    constexpr f32 PLAYER_DEFAULT_SIZE = 25.f;
    constexpr f32 ENEMY_DEFAULT_SIZE = 25.f;
    constexpr f32 ENEMIES_WORLD_Y = -400.f;
    constexpr u16 ENEMIES_MAX_ROWS = 3;
    constexpr u16 ENEMIES_MAX_COLUMNS = 10;
    
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
    using EnemyLayout = std::array<std::array<std::optional<EnemyState>, ENEMIES_MAX_COLUMNS>, ENEMIES_MAX_ROWS>;

    struct InputState {
        i32 move_dir;
        i32 wind_dir_x;
        i32 wind_dir_y;
        
        u32 start_ricochet_ability;
        u32 stop_ricochet_ability;
        u32 new_game;
        u32 game_menu_swap;
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
        Game,
        Settings,
        Quit
    };
    struct MenuState
    {
        MenuPageType current_page;
        MenuPageType last_menu_page;
    };

    struct GameState {
        u32 current_level = 0; 
        f32 time_scale = 1.0f;
        PlayerState player;
        Camera cam;
        EnemyLayout enemies;
        std::vector<Projectile> player_projectiles;
        std::vector<Projectile> enemy_projectiles;
    };

    struct GameFrameTempData
    {
        std::vector<u32> hit_indices;
    };
    
    struct Renderer {
    };
    

    struct Level
    {
        EnemyLayout enemies;
    };
    woc_internal constexpr Level level_1();
    const std::array LEVELS = {
        level_1(),
    };

}
