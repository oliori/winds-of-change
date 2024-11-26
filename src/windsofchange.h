#pragma once

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "gui_styles/style_bluish.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>
#include <array>
#include <cassert>
#include <variant>

#include "windsofchange.h"

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

    struct MenuState;
    
    enum class MenuPageType
    {
        MainMenu,
        Game,
        Settings,
        Quit,
        Credits
    };
    
    constexpr u32 MAX_BUTTONS_PER_PAGE = 12;
    enum class MainMenuButtonType
    {
        NewGame,
        Continue,
        Settings,
        Credits,
        Quit
    };
    static_assert(static_cast<u32>(MainMenuButtonType::Quit) < MAX_BUTTONS_PER_PAGE);
    
    enum class SettingsButtonType
    {
        Back,
    };
    static_assert(static_cast<u32>(SettingsButtonType::Back) < MAX_BUTTONS_PER_PAGE);
    
    enum class CreditsButtonType
    {
        Back,
    };
    static_assert(static_cast<u32>(CreditsButtonType::Back) < MAX_BUTTONS_PER_PAGE);
    
    enum class GameButtonType
    {
        Menu,
    };
    static_assert(static_cast<u32>(GameButtonType::Menu) < MAX_BUTTONS_PER_PAGE);
    
    struct MenuState
    {
        MenuPageType current_page;
        std::array<bool, MAX_BUTTONS_PER_PAGE> buttons_hover_state;
    };
    MenuState menu_init(MenuPageType page);
    
    enum class AudioType : u32
    {
        MusicBackground = 0,
        UIButtonHover,
        UIButtonClick,
        UIPageChange,
        MAX_AUDIO_TYPE
    };
    struct AudioState
    {
        std::array<Sound, static_cast<size_t>(AudioType::MAX_AUDIO_TYPE)> sounds;
    };
    AudioState audio_init();
    void audio_deinit(AudioState& audio_state);
    void audio_play_sound(AudioState& audio_state, AudioType sound_type);

    struct Renderer {
    };
    
    void renderer_finalize_rendering(Renderer& renderer);
    void renderer_prepare_rendering(Renderer& renderer);
    void renderer_update_and_render_menu(Renderer& renderer, MenuState& menu_state, std::optional<GameState>& game_state, AudioState& audio_state, Vector2 framebuffer_size);
    void renderer_update_and_render_settings(Renderer& renderer, MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size);
    void renderer_render_world(Renderer& renderer, GameState& game_state, AudioState& audio_state, Vector2 framebuffer_size);
    void renderer_render_level_fail(Renderer& renderer, GameState& game_state, AudioState& audio_state, Vector2 framebuffer_size);
    void renderer_render_level_complete(Renderer& renderer, GameState& game_state, AudioState& audio_state, Vector2 framebuffer_size);
    void renderer_render_game_won(Renderer& renderer, MenuState& menu_state, GameState& game_state, AudioState& audio_state, Vector2 framebuffer_size);

}
