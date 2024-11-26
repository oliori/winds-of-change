#include "windsofchange.h"

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
    
    woc_internal constexpr Rectangle ui_rectangle_from_anchor(Vector2 framebuffer_size, Vector2 anchor, Vector2 size, Vector2 origin = Vector2{0.5f, 0.5f})
    {
        auto scaled_anchor = Vector2 { framebuffer_size.x * anchor.x, framebuffer_size.y * anchor.y };
        auto size_part_from_origin  = Vector2 { origin.x * size.x, origin.y * size.y };
        auto pos = Vector2 { scaled_anchor.x - size_part_from_origin.x, scaled_anchor.y - size_part_from_origin.y  }; 
        auto result = Rectangle {
            .x = pos.x,
            .y = pos.y,
            .width = size.x,
            .height = size.y
        };
        return result;
    }
    
    woc_internal constexpr Rectangle ui_rectangle_translate(Rectangle rect, Vector2 translation)
    {
        rect.x += translation.x;
        rect.y += translation.y;
        return rect;
    }

    woc_internal void game_load_level(GameState& game_state)
    {
        auto& enemies = game_state.enemies;
        //constexpr Vector2 WORLD_MIN = Vector2{ -700, -500 };
        //constexpr Vector2 WORLD_MAX = Vector2{ 700, 500 };
        switch (game_state.current_level)
        {
            case 0:
            {
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -200.f, -300.f },
                    .size = Vector2 { 100.f, 25.f },
                    .health = 1,
                    .type = EnemyType::Normal,
                    .contributes_to_win = true
                });
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -200.f, -100.f },
                    .size = Vector2 { 100.f, 25.f },
                    .health = 1,
                    .type = EnemyType::Normal,
                    .contributes_to_win = true
                });
                game_state.player.balls_available = 1;
                break;
            }
            case 1:
            {
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -200.f, -300.f },
                    .size = Vector2 { 300.f, 25.f },
                    .health = 1,
                    .type = EnemyType::Normal,
                    .contributes_to_win = true
                });
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -200.f, 300.f },
                    .size = Vector2 { 1000.f, 25.f },
                    .health = 0,
                    .type = EnemyType::Indestructible,
                    .contributes_to_win = false
                });
                game_state.player.balls_available = 1;
                game_state.player.wind_available = 1;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    GameState game_init(u32 level)
    {
        auto result = woc::GameState{
            .current_level = level,
            .time_scale = 1.0,
            .level_status = LevelStatus::InProgress,
            .player = woc::PlayerState {
                .pos_x = 0.f,
                .vel = 0.f,
                .accel = 0.f,
                .ball_velocity = BALL_DEFAULT_VELOCITY,
                .balls_available = 0,
                .wind_available = 0,
                .active_wind_ability = std::nullopt
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

        if (game_state.level_status != LevelStatus::InProgress)
        {
            game_state.time_scale = std::max(0.0f, game_state.time_scale - delta_seconds);
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

        std::erase_if(game_state.player_projectiles, [] (Projectile& p)
        {
            return p.pos.x < WORLD_MIN.x | p.pos.x > WORLD_MAX.x | p.pos.y < WORLD_MIN.y | p.pos.y > WORLD_MAX.y;
        });

        for (auto& p : game_state.player_projectiles)
        {
            p.pos = Vector2Add(p.pos, Vector2Scale(p.dir, game_state.player.ball_velocity * delta_seconds));

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
            return e.health <= 0 && e.type != EnemyType::Indestructible;
        });

        if (input.send_ball & game_state.player.balls_available)
        {
            game_state.player_projectiles.emplace_back(Projectile {
                .pos = Vector2Add(player_pos(game_state.player), Vector2 { 0.f, static_cast<f32>(-BALL_DEFAULT_Y_OFFSET) }),
                .dir = Vector2 { 0, -1 }
            });
            game_state.player.balls_available--;
        }

        if (!game_state.player.active_wind_ability && game_state.player.wind_available)
        {
            if (input.wind_dir_x) {
                game_state.player.active_wind_ability = WindAbility {
                    .timer = WIND_DURATION,
                    .angle = Radian { .val = static_cast<f32>(input.wind_dir_x) * PI / 4 },
                    .ball_current_velocity = game_state.player.ball_velocity,
                    .ball_target_velocity = game_state.player.ball_velocity
                };
                game_state.player.wind_available--;
            } else if (input.wind_dir_y == 1) {
                game_state.player.active_wind_ability = WindAbility {
                    .timer = WIND_DURATION,
                    .angle = Radian { .val = 0 },
                    .ball_current_velocity = game_state.player.ball_velocity,
                    .ball_target_velocity = game_state.player.ball_velocity * 1.5f
                };
                game_state.player.wind_available--;
            } else if (input.wind_dir_y == -1) {
                game_state.player.active_wind_ability = WindAbility {
                    .timer = WIND_DURATION,
                    .angle = Radian { 0.0f },
                    .ball_current_velocity = game_state.player.ball_velocity,
                    .ball_target_velocity = game_state.player.ball_velocity * -1.0f
                };
                game_state.player.wind_available--;
            }
        }
        if (auto& wind = game_state.player.active_wind_ability)
        {
            auto wind_delta = std::min(delta_seconds, wind->timer);
            f32 delta_decimal = Clamp(wind_delta / WIND_DURATION, 0.0f, 1.0f);
            f32 total_delta_velocity = wind->ball_target_velocity - wind->ball_current_velocity;
            game_state.player.ball_velocity += total_delta_velocity * delta_decimal;

            for (auto& p : game_state.player_projectiles)
            {
                p.dir = Vector2Rotate(p.dir, delta_decimal * wind->angle.val);
            }

            wind->timer -= wind_delta;
            if (wind->timer < 0.f)
            {
                wind = std::nullopt;
            } 
        }

        if (game_state.level_status == LevelStatus::InProgress && !std::ranges::any_of(game_state.enemies, [] (EnemyState& e) { return e.contributes_to_win; })) 
        {
            game_state.level_status = LevelStatus::Won;
        } else if (game_state.level_status == LevelStatus::InProgress && !game_state.player.balls_available && game_state.player_projectiles.empty())
        {
            game_state.level_status = LevelStatus::Lost;
        }
    }
    
    woc_internal bool renderer_ui_button(
        Rectangle bounds, std::string_view text, bool already_hovered,
        AudioState& audio_state,
        bool& out_hover)
    {
        out_hover = CheckCollisionPointRec(GetMousePosition(), bounds);
        if (!already_hovered && out_hover)
        {
            audio_play_sound(audio_state, AudioType::UIButtonHover);
        }
        
        if (GuiButton(bounds, text.data()))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            return true;
        }

        return false;
    }

    void renderer_update_and_render_menu(Renderer& renderer, MenuState& menu_state, std::optional<GameState>& game_state, AudioState& audio_state, Vector2 framebuffer_size) {
        constexpr auto button_spacing = 10.f;

        auto buttons_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 200.f, 50.f }, Vector2{0.5f, 0.0f});
        auto& continue_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Continue));
        if (!game_state)
        {
            GuiSetState(STATE_DISABLED);
        }
        if (renderer_ui_button(buttons_rect, "CONTINUE", continue_hover, audio_state, continue_hover))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_state = menu_init(MenuPageType::Game);
        }
        GuiSetState(STATE_NORMAL);
        
        buttons_rect.y += buttons_rect.height + button_spacing;
        auto& new_game_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::NewGame));
        if (renderer_ui_button(buttons_rect, "NEW GAME", new_game_hover, audio_state, new_game_hover))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_state = menu_init(MenuPageType::Game);
            game_state = game_init(START_LEVEL);
        }
        
        buttons_rect.y += buttons_rect.height + button_spacing;
        auto& settings_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Settings));
        if (renderer_ui_button(buttons_rect, "SETTINGS", settings_hover, audio_state, settings_hover))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_state = menu_init(MenuPageType::Settings);
        }
        
        buttons_rect.y += buttons_rect.height + button_spacing;
        auto& credits_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Credits));
        if (renderer_ui_button(buttons_rect, "CREDITS", credits_hover, audio_state, credits_hover))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_state = menu_init(MenuPageType::Credits);
        }
        
        buttons_rect.y += buttons_rect.height + button_spacing;
        auto& quit_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Quit));
        if (renderer_ui_button(buttons_rect, "QUIT", quit_hover, audio_state, quit_hover))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_state = menu_init(MenuPageType::Quit);
        }
    }

    void renderer_update_and_render_settings(Renderer& renderer, MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size)
    {
        auto button_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 200.f, 50.f }, Vector2{0.5f, 0.0f});
        if (GuiButton(button_rect, "BACK"))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_state.current_page = MenuPageType::MainMenu;
        }
    }
    
    void renderer_update_and_render_credits(Renderer& renderer, MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size)
    {
        auto button_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 200.f, 50.f }, Vector2{0.5f, 0.0f});
        if (GuiButton(button_rect, "BACK"))
        {
            audio_play_sound(audio_state, AudioType::UIButtonClick);
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_state.current_page = MenuPageType::MainMenu;
        }

        // Background music credits
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

        //auto diff = Vector2Subtract(WORLD_MAX, WORLD_MIN);
        //DrawRectangle(static_cast<i32>(WORLD_MIN.x), static_cast<i32>(WORLD_MIN.y), static_cast<i32>(diff.x), static_cast<i32>(diff.y), LIGHTGRAY);

        auto player_half_size = Vector2Scale(player_size(), 0.5f);
        auto player_rect = Rectangle { player.pos_x - player_half_size.x, PLAYER_WORLD_Y - player_half_size.y, PLAYER_DEFAULT_WIDTH, PLAYER_DEFAULT_HEIGHT };
        DrawRectanglePro(player_rect, Vector2Zero(), 0.f, PLAYER_COLOR);
        DrawRectangleLinesEx(player_rect, 1.0f, BLACK);
        
        for (auto& e : game_state.enemies)
        {
            auto half_size = Vector2Scale(e.size, 0.5f);
            auto e_rect = Rectangle {e.pos.x - half_size.x, e.pos.y - half_size.y, e.size.x, e.size.y };
            switch (e.type)
            {
                case EnemyType::Indestructible: {
                    DrawRectanglePro(e_rect, Vector2Zero(), 0.f, INDESTRUCTIBLE_WALL_COLOR);
                    DrawRectangleLinesEx(e_rect, 1.0f, BLACK);
                    break;
                }
                case EnemyType::Normal: {
                    DrawRectanglePro(e_rect, Vector2Zero(), 0.f, WALL_COLOR);
                    auto health_rect = e_rect;
                    for (auto i = 0; i < e.health; i++)
                    {
                        DrawRectangleLinesEx(health_rect, 1.0f, WHITE);
                        health_rect.height += 2;
                        health_rect.width += 2;
                    }
                    break;
                }
            }
        }
        
        if (game_state.player.balls_available)
        {
            DrawCircleLinesV(Vector2Add(player_pos(player), Vector2 { 0.f, -BALL_DEFAULT_Y_OFFSET}), BALL_DEFAULT_RADIUS, BALL_COLOR);
        }
        
        for (auto& projectile : game_state.player_projectiles)
        {
            DrawCircleV(projectile.pos, BALL_DEFAULT_RADIUS, BALL_COLOR);
        }

        EndMode2D();

        i32 fbx = static_cast<i32>(framebuffer_size.x);
        i32 fby = static_cast<i32>(framebuffer_size.y);
        
        i32 balls_available_spacing = 20;
        i32 balls_available_size = 30;
        i32 balls_available_pos_x = fbx - balls_available_spacing - balls_available_size;
        i32 balls_available_pos_y = fby - balls_available_spacing - balls_available_size;
        for (u32 i = 0; i < game_state.player.balls_available; i++)
        {
            DrawCircle(balls_available_pos_x, balls_available_pos_y, static_cast<f32>(balls_available_size), BALL_COLOR);
            balls_available_pos_y -= balls_available_spacing + balls_available_size;
        }
        
        i32 wind_available_spacing = 20;
        i32 wind_available_size = 30;
        i32 wind_available_pos_x = balls_available_pos_x - balls_available_spacing - balls_available_size * 2;
        i32 wind_available_pos_y = fby - wind_available_spacing - wind_available_size;
        for (u32 i = 0; i < game_state.player.wind_available; i++)
        {
            DrawCircle(wind_available_pos_x, wind_available_pos_y, static_cast<f32>(wind_available_size), YELLOW);
            wind_available_pos_y -= wind_available_spacing + wind_available_size;
        }
    }

    void renderer_render_level_complete(Renderer& renderer, GameState& game_state, Vector2 framebuffer_size)
    {
        i32 fbx = static_cast<i32>(framebuffer_size.x);
        i32 fby = static_cast<i32>(framebuffer_size.y);
        DrawRectangle(0, 0, fbx, fby, Color{0, 0, 0, static_cast<u8>(Lerp(225.f, 0.f, game_state.time_scale * game_state.time_scale))});
        auto buttons_rect = ui_rectangle_from_anchor(framebuffer_size, { 0.5, 0.5 }, { 200.f, 50.f});
        if (GuiButton(buttons_rect, "NEXT LEVEL"))
        {
            game_state = game_init(game_state.current_level+1);
        }
    }
    
    void renderer_render_level_fail(Renderer& renderer, GameState& game_state, Vector2 framebuffer_size)
    {
        i32 fbx = static_cast<i32>(framebuffer_size.x);
        i32 fby = static_cast<i32>(framebuffer_size.y);
        DrawRectangle(0, 0, fbx, fby, Color{0, 0, 0, static_cast<u8>(Lerp(225.f, 0.f, game_state.time_scale * game_state.time_scale))});
        auto buttons_rect = ui_rectangle_from_anchor(framebuffer_size, { 0.5, 0.5 }, { 200.f, 50.f});
        if (GuiButton(buttons_rect, "TRY AGAIN"))
        {
            game_state = game_init(game_state.current_level);
        }
    }
    
    void renderer_render_game_won(Renderer& renderer, MenuState& menu_state, std::optional<GameState>& game_state, Vector2 framebuffer_size)
    {
        i32 fbx = static_cast<i32>(framebuffer_size.x);
        i32 fby = static_cast<i32>(framebuffer_size.y);
        DrawRectangle(0, 0, fbx, fby, Color{0, 0, 0, static_cast<u8>(Lerp(225.f, 0.f, game_state->time_scale * game_state->time_scale))});
        auto buttons_rect = ui_rectangle_from_anchor(framebuffer_size, { 0.5, 0.5 }, { 200.f, 50.f});
        if (GuiButton(buttons_rect, "MAIN MENU"))
        {
            menu_state.current_page = MenuPageType::MainMenu;
            game_state = std::nullopt;
        }
    }

    void renderer_finalize_rendering(Renderer& renderer)
    {
        EndDrawing();
    }

    void renderer_prepare_rendering(Renderer& renderer)
    {
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
    }

    MenuState menu_init(MenuPageType page)
    {
        auto result = MenuState {
            .current_page = page,
            .buttons_hover_state = {}
        };
        return result;
    }

    AudioState audio_init()
    {
        InitAudioDevice();

        auto result = AudioState {
            .sounds{}
        };
        result.sounds.at(static_cast<size_t>(AudioType::MusicBackground)) = LoadSound("assets/audio/cozy.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::UIPageChange)) = LoadSound("assets/audio/temp/card-slide-6.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::UIButtonHover)) = LoadSound("assets/audio/temp/chips-handle-3.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::UIButtonClick)) = LoadSound("assets/audio/temp/die-throw-1.ogg");

        return result;
    }

    void audio_deinit(AudioState& audio_state)
    {
        for (auto& sound : audio_state.sounds)
        {
            UnloadSound(sound);
        }
        CloseAudioDevice();
    }

    void audio_play_sound(AudioState& audio_state, AudioType sound_type)
    {
        assert(sound_type < AudioType::MAX_AUDIO_TYPE);
        PlaySound(audio_state.sounds.at((size_t)sound_type));
    }
}
