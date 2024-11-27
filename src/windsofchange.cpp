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
        switch (game_state.current_level)
        {
            case 0:
            {
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { 0.f, 0.f },
                    .size = WALL_SIZE_DEFAULT,
                    .health = 3,
                    .rot = Radian { 0.f },
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
                    .size = WALL_SIZE_DEFAULT,
                    .health = 1,
                    .rot = Radian { 0.f },
                    .type = EnemyType::Normal,
                    .contributes_to_win = true
                });
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -200.f, -100.f },
                    .size = WALL_SIZE_DEFAULT,
                    .health = 1,
                    .rot = Radian { 0.f },
                    .type = EnemyType::Normal,
                    .contributes_to_win = true
                });
                game_state.player.balls_available = 1;
                break;
            }
            case 2:
            {
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -200.f, -300.f },
                    .size = WALL_SIZE_L,
                    .health = 1,
                    .rot = Radian { 0.f },
                    .type = EnemyType::Normal,
                    .contributes_to_win = true
                });
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { -200.f, 300.f },
                    .size = WALL_SIZE_XL,
                    .health = 0,
                    .rot = Radian { 0.f },
                    .type = EnemyType::Indestructible,
                    .contributes_to_win = false
                });
                game_state.player.balls_available = 1;
                game_state.player.wind_available = 1;
                break;
            }
            case 3:
            {
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { WORLD_MAX.x, -300.f },
                    .size = WALL_SIZE_DEFAULT,
                    .health = 1,
                    .rot = Radian { PI / 2.f },
                    .type = EnemyType::Normal,
                    .contributes_to_win = true
                });
                enemies.emplace_back(EnemyState {
                    .pos = Vector2 { 0.f, WORLD_MAX.y },
                    .size = WALL_SIZE_DEFAULT,
                    .health = 0,
                    .rot = Radian { 0.f },
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
    enum class CollisionResult
    {
        NoCollision = 0,
        Collision = 1,
    };
    woc_internal CollisionResult sphere_collides_rectangle(
        Vector2 sphere_pos, Vector2 sphere_dir, f32 sphere_radius, 
        Vector2 rectangle_pos, Vector2 rectangle_size, Radian rectangle_rotation,
        Vector2& out_normal)
    {
        out_normal = Vector2Zero();
        
        auto half_size = Vector2Scale(rectangle_size, 0.5f);
        auto s_pos2 = Vector2Subtract(sphere_pos, rectangle_pos);
        s_pos2 = Vector2Rotate(s_pos2, -rectangle_rotation.val);
        auto rotated_dir = Vector2Rotate(s_pos2, -rectangle_rotation.val);

        bool inside_x = true;
        bool inside_y = true;
        Vector2 test_point = s_pos2;
        
        if (s_pos2.x < -half_size.x) // LEFT
        {
            inside_x = false;
            test_point.x = -half_size.x;
            out_normal.x = -1.f;
        } else if (s_pos2.x > half_size.x) // RIGHT
        {
            inside_x = false;
            test_point.x = half_size.x;
            out_normal.x = 1.f;
        } 

        if (s_pos2.y > half_size.y) // ABOVE
        {
            inside_y = false;
            test_point.y = half_size.y;
            out_normal.y = 1.f;
        } else if (s_pos2.y < -half_size.y) // BELOW
        {
            inside_y = false;
            test_point.y = -half_size.y;
            out_normal.y = -1.f;
        } 

        // INSIDE
        if (inside_y && inside_x)
        {
            auto current_t = std::numeric_limits<f32>::max();

            auto back_dir = Vector2Scale(rotated_dir, -1.f);
            if (!FloatEquals(back_dir.x, 0.f))
            {
                auto hit_left_t = (-half_size.x - s_pos2.x) / back_dir.x;
                if (hit_left_t > 0.f && hit_left_t < current_t)
                {
                    out_normal = Vector2 { -1.f, 0.f };
                    current_t = hit_left_t;
                }
                auto hit_right_t = (half_size.x - s_pos2.x) / back_dir.x;
                if (hit_right_t > 0.f && hit_right_t < current_t)
                {
                    out_normal = Vector2 { 1.f, 0.f };
                    current_t = hit_right_t;
                }
            }
            
            if (!FloatEquals(back_dir.y, 0.f))
            {
                auto hit_up_t = (half_size.y - s_pos2.y) / back_dir.y;
                if (hit_up_t > 0.f && hit_up_t < current_t)
                {
                    out_normal = Vector2 { 0.f, 1.f };
                    current_t = hit_up_t;
                }
                auto hit_below_t = (-half_size.y - s_pos2.y) / back_dir.y;
                if (hit_below_t > 0.f && hit_below_t < current_t)
                {
                    out_normal = Vector2 { 0.f, -1.f };
                }
            }
            
            // For simplicity just return up vector now if we get inside the rectangle
            // Ideally this should find the intersection point using the velocity
            out_normal = Vector2Rotate(out_normal, rectangle_rotation.val);
            return CollisionResult::Collision;
        }

        auto normal_dot = Vector2DotProduct(out_normal, rotated_dir);
        if (normal_dot > 0.0f ||  Vector2DistanceSqr(s_pos2, test_point) > sphere_radius * sphere_radius)
        {
            return CollisionResult::NoCollision;
        }

        out_normal = Vector2Rotate(Vector2Normalize(out_normal), rectangle_rotation.val);
        return CollisionResult::Collision;
    }

    void game_update(GameState& game_state, InputState& input, AudioState& audio_state, f32 delta_seconds)
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

        std::erase_if(game_state.dead_projectile_effects, [delta_seconds, &vel = game_state.player.ball_velocity] (ProjectileDeadEffect& dead_projectile)
        {
            auto alpha = dead_projectile.timer / PROJECTILE_DEAD_EFFECT_DURATION;
            auto eased_alpha = ease_in_cubic(alpha);
            dead_projectile.pos = Vector2Add(dead_projectile.pos, Vector2Scale(dead_projectile.dir, vel * delta_seconds * eased_alpha));
            dead_projectile.timer -= delta_seconds;
            return dead_projectile.timer <= 0.f;
        });
        std::erase_if(game_state.player_projectiles, [&audio_state, &dbe = game_state.dead_projectile_effects] (Projectile& p)
        {
            if (p.pos.x < WORLD_MIN.x | p.pos.x > WORLD_MAX.x | p.pos.y < WORLD_MIN.y | p.pos.y > WORLD_MAX.y)
            {
                audio_play_sound_randomize_pitch(audio_state, AudioType::SFXBallDisappear);
                dbe.emplace_back(ProjectileDeadEffect {
                    .pos = p.pos,
                    .dir = p.dir,
                    .timer = PROJECTILE_DEAD_EFFECT_DURATION
                });
                return true;
            }
            return false;
        });

        // Without proper mixing, limit to 1 impact sound each frame
        bool collide_indestructible = false;
        bool collide_wall = false;
        for (auto& p : game_state.player_projectiles)
        {
            p.pos = Vector2Add(p.pos, Vector2Scale(p.dir, game_state.player.ball_velocity * delta_seconds));
            p.time_since_last_collision += delta_seconds;
            if (p.time_since_last_collision > MIN_TIME_BETWEEN_COLLISIONS)
            {
                for (auto& e : game_state.enemies)
                {
                    Vector2 collision_normal = Vector2Zero();
                    auto collision = sphere_collides_rectangle(p.pos, p.dir, BALL_DEFAULT_RADIUS, e.pos, e.size, e.rot, collision_normal);
                    if (collision == CollisionResult::Collision)
                    {
                        assert(!Vector2Equals(collision_normal, Vector2Zero()));
                        p.time_since_last_collision = 0.f;
                        p.dir = Vector2Reflect(p.dir, collision_normal);
                        e.health--;
                        collide_wall |= e.type != EnemyType::Indestructible;
                        collide_indestructible |= e.type == EnemyType::Indestructible;
                        break;
                    }
                }
                
                Vector2 collision_normal = Vector2Zero();
                auto collision  = sphere_collides_rectangle(p.pos, p.dir, BALL_DEFAULT_RADIUS, player_pos(game_state.player), player_size(), Radian { 0.0f }, collision_normal);
                if (collision == CollisionResult::Collision)
                {
                    assert(!Vector2Equals(collision_normal, Vector2Zero()));
                    p.time_since_last_collision = 0.f;
                    p.dir = Vector2Reflect(p.dir, collision_normal);
                    collide_indestructible = true;
                    break;
                }
            }
        }

        if (collide_indestructible)
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::SFXIndestructibleImpact);
        }
        if (collide_wall)
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::SFXWallImpact);
        }

        std::erase_if(game_state.dead_enemy_effects, [delta_seconds] (EnemyDeadEffect& dead_effect)
        {
            dead_effect.timer -= delta_seconds;
            return dead_effect.timer <= 0.f;
        });
        std::erase_if(game_state.enemies, [&dee = game_state.dead_enemy_effects, &audio_state] (EnemyState& e)
        {
            if (e.health <= 0 && e.type != EnemyType::Indestructible)
            {
                audio_play_sound_randomize_pitch(audio_state, AudioType::SFXWallDisappear);
                dee.emplace_back(EnemyDeadEffect {
                    .pos = e.pos,
                    .size = e.size,
                    .rot = e.rot,
                    .timer = ENEMY_DEAD_EFFECT_DURATION
                });
                return true;
            }
            return false;
        });

        if (input.send_ball & game_state.player.balls_available)
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::SFXSendBall);
            game_state.player_projectiles.emplace_back(Projectile {
                .pos = Vector2Add(player_pos(game_state.player), Vector2 { 0.f, -BALL_DEFAULT_Y_OFFSET }),
                .dir = Vector2 { 0, -1 },
                .time_since_last_collision = 0.f
            });
            game_state.player.balls_available--;
        }

        if (!game_state.player.active_wind_ability && game_state.player.wind_available)
        {
            if (input.wind_dir_x) {
                audio_play_sound_randomize_pitch(audio_state, AudioType::SFXWind);
                game_state.player.active_wind_ability = WindAbility {
                    .timer = WIND_DURATION,
                    .angle = Radian { .val = static_cast<f32>(input.wind_dir_x) * PI / 4 },
                    .ball_current_velocity = game_state.player.ball_velocity,
                    .ball_target_velocity = game_state.player.ball_velocity
                };
                game_state.player.wind_available--;
            } else if (input.wind_dir_y == 1) {
                audio_play_sound_randomize_pitch(audio_state, AudioType::SFXWind);
                game_state.player.active_wind_ability = WindAbility {
                    .timer = WIND_DURATION,
                    .angle = Radian { .val = 0 },
                    .ball_current_velocity = game_state.player.ball_velocity,
                    .ball_target_velocity = game_state.player.ball_velocity * 1.5f
                };
                game_state.player.wind_available--;
            } else if (input.wind_dir_y == -1) {
                audio_play_sound_randomize_pitch(audio_state, AudioType::SFXWind);
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
            audio_play_sound(audio_state, AudioType::SFXLevelWon);
        } else if (game_state.level_status == LevelStatus::InProgress
            && !game_state.player.balls_available
            && game_state.player_projectiles.empty()
            && game_state.dead_projectile_effects.empty())
        {
            audio_play_sound(audio_state, AudioType::SFXLevelLost);
            game_state.level_status = LevelStatus::Lost;
        }
    }
    
    woc_internal Texture2D& texture_from_type(Renderer& r, TextureType t)
    {
        return r.loaded_textures.at(static_cast<size_t>(t));
    }
    
    Renderer renderer_init()
    {
        auto result = Renderer {
            .loaded_textures{}
        };

        texture_from_type(result, TextureType::KeyA) = LoadTexture("assets/textures/a_key.png");
        texture_from_type(result, TextureType::KeyD) = LoadTexture("assets/textures/d_key.png");
        texture_from_type(result, TextureType::KeyEsc) = LoadTexture("assets/textures/esc_key.png");
        texture_from_type(result, TextureType::KeySpace) = LoadTexture("assets/textures/space_key.png");
        texture_from_type(result, TextureType::KeyUp) = LoadTexture("assets/textures/up_key.png");
        texture_from_type(result, TextureType::KeyDown) = LoadTexture("assets/textures/down_key.png");
        texture_from_type(result, TextureType::KeyLeft) = LoadTexture("assets/textures/left_key.png");
        texture_from_type(result, TextureType::KeyRight) = LoadTexture("assets/textures/right_key.png");
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, 30);

        return result;
    }

    void renderer_deinit(Renderer& renderer)
    {
        for (auto& tex : renderer.loaded_textures)
        {
            UnloadTexture(tex);
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
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIButtonHover);
        }
        
        if (GuiButton(bounds, text.data()))
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIButtonClick);
            return true;
        }

        return false;
    }
    
    woc_internal bool renderer_ui_toggle_button(
        Rectangle bounds, std::string_view text, bool already_hovered,
        AudioState& audio_state,
        bool& out_hover, bool& toggled)
    {
        out_hover = CheckCollisionPointRec(GetMousePosition(), bounds);
        if (!already_hovered && out_hover)
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIButtonHover);
        }

        bool prev_toggled = toggled;
        GuiToggle(bounds, text.data(), &toggled);
        if (prev_toggled != toggled)
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIButtonClick);
            return true;
        }

        return false;
    }

    void renderer_update_and_render_menu(Renderer& renderer, MenuState& menu_state, std::optional<GameState>& game_state, AudioState& audio_state, Vector2 framebuffer_size) {
        auto title_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5}, Vector2 { framebuffer_size.x, 150.f }, Vector2{0.5f, 0.0f});
        title_rect.y -= title_rect.height + 40;
        GuiSetStyle(DEFAULT, TEXT_SIZE, 125);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiLabel(title_rect, "Winds of Change");
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, 65);
        auto primary_buttons_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 400.f, 100.f }, Vector2{0.5f, 0.0f});
        auto secondary_buttons_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 300.f, 60.f }, Vector2{0.5f, 0.0f});
        auto& continue_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Continue));
        if (!game_state)
        {
            GuiSetState(STATE_DISABLED);
        }
        if (renderer_ui_button(primary_buttons_rect, "CONTINUE", continue_hover, audio_state, continue_hover))
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIPageChange);
            menu_change_page(menu_state, MenuPageType::Game);
        }
        GuiSetState(STATE_NORMAL);
        
        primary_buttons_rect.y += primary_buttons_rect.height + BUTTON_SPACING;
        secondary_buttons_rect.y += primary_buttons_rect.height + BUTTON_SPACING;
        
        auto& new_game_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::NewGame));
        if (renderer_ui_button(primary_buttons_rect, "NEW GAME", new_game_hover, audio_state, new_game_hover))
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIPageChange);
            menu_change_page(menu_state, MenuPageType::Game);
            game_state = game_init(START_LEVEL);
        }
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, 40);
        primary_buttons_rect.y += primary_buttons_rect.height + BUTTON_SPACING;
        secondary_buttons_rect.y += primary_buttons_rect.height + BUTTON_SPACING;
        auto& settings_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Settings));
        if (renderer_ui_button(secondary_buttons_rect, "SETTINGS", settings_hover, audio_state, settings_hover))
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIPageChange);
            menu_change_page(menu_state, MenuPageType::Settings);
        }
        
        secondary_buttons_rect.y += secondary_buttons_rect.height + BUTTON_SPACING;
        auto& credits_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Credits));

        if (renderer_ui_button(secondary_buttons_rect, "CREDITS", credits_hover, audio_state, credits_hover))
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIPageChange);
            menu_change_page(menu_state, MenuPageType::Credits);
        }
        
        secondary_buttons_rect.y += secondary_buttons_rect.height + BUTTON_SPACING;
        auto& quit_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(MainMenuButtonType::Quit));
        if (renderer_ui_button(secondary_buttons_rect, "QUIT", quit_hover, audio_state, quit_hover))
        {
            audio_play_sound_randomize_pitch(audio_state, AudioType::UIPageChange);
            menu_change_page(menu_state, MenuPageType::Quit);
        }
    }

    void renderer_update_and_render_settings(Renderer& renderer, MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size)
    {
        auto button_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.3f}, Vector2 { 300.f, 50.f }, Vector2{0.5f, 0.0f});
        auto& back_hover = menu_state.buttons_hover_state.at((size_t)SettingsButtonType::Back);
        GuiSetState(STATE_NORMAL);
        
        GuiLine(button_rect, "WINDOW");
        button_rect.y += button_rect.height + BUTTON_SPACING;
        if (renderer_ui_toggle_button(button_rect, "FULLSCREEN", back_hover, audio_state, back_hover, menu_state.is_fullscreen))
        {
        }
        button_rect.y += button_rect.height + BUTTON_SPACING;

        if (menu_state.is_fullscreen)
        {
            GuiSetState(STATE_DISABLED);
        }
        i32 current_res = static_cast<i32>(menu_state.resolution);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
        button_rect.width /= static_cast<f32>(static_cast<u32>(ResolutionPreset::MAX));
        constexpr auto RESOLUTION_TEXT = "1600x900;1920x1080";
        GuiToggleGroup(button_rect, RESOLUTION_TEXT, &current_res);
        menu_state.resolution = static_cast<ResolutionPreset>(current_res);
        assert(menu_state.resolution < ResolutionPreset::MAX);
        button_rect.y += button_rect.height + BUTTON_SPACING;
        button_rect.width *= static_cast<f32>(static_cast<u32>(ResolutionPreset::MAX));
        GuiSetStyle(DEFAULT, TEXT_SIZE, 40);
        GuiSetState(STATE_NORMAL);
        
        GuiLine(button_rect, "AUDIO");
        button_rect.y += button_rect.height + BUTTON_SPACING;

        GuiSlider(button_rect, "VOLUME:", "", &menu_state.volume, 0.0f, 1.0f);
        button_rect.y += button_rect.height + BUTTON_SPACING;

        GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

        constexpr f32 SMALL_BUTTON_SIZE = 150.f;
        button_rect.x += (button_rect.width - SMALL_BUTTON_SIZE) / 2.f;
        button_rect.width = SMALL_BUTTON_SIZE;
        if (renderer_ui_button(button_rect, "BACK", back_hover, audio_state, back_hover))
        {
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_change_page(menu_state, MenuPageType::MainMenu);
        }
        GuiSetStyle(DEFAULT, TEXT_SIZE, 40);
    }
    
    void renderer_update_and_render_credits(Renderer& renderer, MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size)
    {
        auto label_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.16f}, Vector2 { framebuffer_size.x, 50.f }, Vector2{0.5f, 0.0f});
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 40);
        GuiLabel(label_rect, "Developed by:");
        label_rect.y += label_rect.height + BUTTON_SPACING;
        label_rect.height = 100.f;
        GuiSetStyle(DEFAULT, TEXT_SIZE, 80);
        GuiLabel(label_rect, "Oliver Jorgensen");
        label_rect.y += label_rect.height + BUTTON_SPACING * 3;
        label_rect.height = 50.f;
        GuiSetStyle(DEFAULT, TEXT_SIZE, 40);
        
        GuiLabel(label_rect, "Background Music:");
        label_rect.y += label_rect.height + BUTTON_SPACING;
        GuiLabel(label_rect, "Cozy - Composed by One Man Symphony");
        label_rect.y += label_rect.height + BUTTON_SPACING;
        GuiLabel(label_rect, "https://onemansymphony.bandcamp.com");
        label_rect.y += label_rect.height + BUTTON_SPACING * 3;
        
        GuiLabel(label_rect, "Sound Effects & Icons");
        label_rect.y += label_rect.height + BUTTON_SPACING;
        GuiLabel(label_rect, "Created by Kenney");
        label_rect.y += label_rect.height + BUTTON_SPACING;
        GuiLabel(label_rect, "https://kenney.nl");
        label_rect.y += label_rect.height + BUTTON_SPACING * 3;
        
        auto button_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.75f}, Vector2 { 300.f, 50.f }, Vector2{0.5f, 0.0f});
        button_rect.y = label_rect.y;
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
        auto& back_hover = menu_state.buttons_hover_state.at((size_t)CreditsButtonType::Back);
        if (renderer_ui_button(button_rect, "BACK", back_hover, audio_state, back_hover))
        {
            audio_play_sound(audio_state, AudioType::UIPageChange);
            menu_change_page(menu_state, MenuPageType::MainMenu);
        }
        GuiSetStyle(DEFAULT, TEXT_SIZE, 40);
        button_rect.y += button_rect.height + BUTTON_SPACING;

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

        auto player_half_size = Vector2Scale(player_size(), 0.5f);
        auto player_rect = Rectangle { player.pos_x - player_half_size.x, PLAYER_WORLD_Y - player_half_size.y, PLAYER_DEFAULT_WIDTH, PLAYER_DEFAULT_HEIGHT };
        DrawRectanglePro(player_rect, Vector2Zero(), 0.f, PLAYER_COLOR);
        DrawRectangleLinesEx(player_rect, 1.0f, BLACK);
        
        for (auto& e : game_state.enemies)
        {
            auto half_size = Vector2Scale(e.size, 0.5f);
            auto disp = Vector2 { -half_size.x, -half_size.y };
            disp = Vector2Rotate(disp, e.rot.val);
            auto e_rect = Rectangle {e.pos.x + disp.x, e.pos.y + disp.y, e.size.x, e.size.y };
            
            auto border_disp = Vector2Rotate({ 3.f, 3.f}, e.rot.val);
            switch (e.type)
            {
                case EnemyType::Indestructible: {
                    auto border_rect = e_rect;
                    border_rect.height += 6;
                    border_rect.width += 6;
                    border_rect.x -= border_disp.x;
                    border_rect.y -= border_disp.y;
                    DrawRectanglePro(border_rect, Vector2Zero(), e.rot.val * RAD2DEG, BLACK);
                    DrawRectanglePro(e_rect, Vector2Zero(), e.rot.val * RAD2DEG, INDESTRUCTIBLE_WALL_COLOR);

                    break;
                }
                case EnemyType::Normal: {
                    auto health_rect = e_rect;
                    for (auto i = 0; i < e.health; i++)
                    {
                        health_rect.x -= border_disp.x;
                        health_rect.y -= border_disp.y;
                        health_rect.height += 6;
                        health_rect.width += 6;
                        DrawRectanglePro(health_rect, Vector2Zero(), e.rot.val * RAD2DEG, WHITE);
                    }
                    DrawRectanglePro(e_rect, Vector2Zero(), e.rot.val * RAD2DEG, WALL_COLOR);
                    break;
                }
            }
        }
        
        for (auto& e : game_state.dead_enemy_effects)
        {
            auto alpha = e.timer / ENEMY_DEAD_EFFECT_DURATION;
            auto alpha_eased = ease_in_back(alpha);
            auto size = Vector2Scale(e.size, alpha_eased);
            auto half_size = Vector2Scale(size, 0.5f);
            auto disp = Vector2 { -half_size.x, -half_size.y };
            disp = Vector2Rotate(disp, e.rot.val);
            auto e_rect = Rectangle {e.pos.x + disp.x, e.pos.y + disp.y, size.x, size.y };
            DrawRectanglePro(e_rect, Vector2Zero(), e.rot.val * RAD2DEG, WHITE);
        }
        
        if (game_state.player.balls_available)
        {
            DrawCircleLinesV(Vector2Add(player_pos(player), Vector2 { 0.f, -BALL_DEFAULT_Y_OFFSET}), BALL_DEFAULT_RADIUS, BALL_COLOR);
        }
        
        for (auto& projectile : game_state.player_projectiles)
        {
            DrawCircleV(projectile.pos, BALL_DEFAULT_RADIUS, BALL_COLOR);
        }
        for (auto& p : game_state.dead_projectile_effects)
        {
            auto alpha = p.timer / PROJECTILE_DEAD_EFFECT_DURATION;
            auto alpha_eased = ease_in_back(alpha);
            auto size = BALL_DEFAULT_RADIUS * alpha_eased;
            DrawCircleV(p.pos, size, WHITE);
        }

        EndMode2D();

        auto patch_info = NPatchInfo {
            .source = Rectangle { .x = 0, .y = 0, .width = 16, .height = 16  },
            .left = 0,
            .top = 0,
            .right = 0,
            .bottom = 0,
            .layout = 0
        };
        auto balls_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2 { 1.0, 1.0f }, Vector2 { ICON_SIZE, ICON_SIZE }, Vector2 { 1.0, 1.0f });
        auto wind_rect = balls_rect;
        wind_rect.x -= ICON_SPACING + ICON_SIZE;
        
        for (u32 i = 0; i < game_state.player.balls_available; i++)
        {
            DrawTextureNPatch(texture_from_type(renderer, TextureType::KeySpace), patch_info, balls_rect, Vector2Zero(), 0.f, BALL_COLOR);
            balls_rect.y += ICON_SPACING + ICON_SIZE;
        }
        
        //wind_rect.y += WIND_AVAILABLE_SPACING;
        for (u32 i = 0; i < game_state.player.wind_available; i++)
        {
            DrawTextureNPatch(texture_from_type(renderer, TextureType::KeyUp), patch_info, wind_rect, Vector2Zero(), 0.f, WHITE);
            wind_rect.y += ICON_SPACING + ICON_SIZE;
        }

        auto tutorial_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2 { 0.0, 1.0f }, Vector2 { ICON_SIZE, ICON_SIZE }, Vector2 { 0.0, 1.0f });
        constexpr i32 TUTORIAL_SMALL_MARGIN = 10;
        constexpr i32 TUTORIAL_BIG_MARGIN = 40;
        tutorial_rect.x += TUTORIAL_SMALL_MARGIN;
        tutorial_rect.y -= TUTORIAL_SMALL_MARGIN;
        
        DrawTextureNPatch(texture_from_type(renderer, TextureType::KeyA), patch_info, tutorial_rect, Vector2Zero(), 0.f, WHITE);
        tutorial_rect.x += tutorial_rect.width + TUTORIAL_SMALL_MARGIN;
        DrawTextureNPatch(texture_from_type(renderer, TextureType::KeyD), patch_info, tutorial_rect, Vector2Zero(), 0.f, WHITE);
        
        tutorial_rect.x += tutorial_rect.width + TUTORIAL_BIG_MARGIN;
        DrawTextureNPatch(texture_from_type(renderer, TextureType::KeySpace), patch_info, tutorial_rect, Vector2Zero(), 0.f, WHITE);
        
        tutorial_rect.x += tutorial_rect.width + TUTORIAL_BIG_MARGIN;
        DrawTextureNPatch(texture_from_type(renderer, TextureType::KeyLeft), patch_info, tutorial_rect, Vector2Zero(), 0.f, WHITE);
        tutorial_rect.x += tutorial_rect.width + TUTORIAL_SMALL_MARGIN;
        DrawTextureNPatch(texture_from_type(renderer, TextureType::KeyUp), patch_info, tutorial_rect, Vector2Zero(), 0.f, WHITE);
        tutorial_rect.x += tutorial_rect.width + TUTORIAL_SMALL_MARGIN;
        DrawTextureNPatch(texture_from_type(renderer, TextureType::KeyDown), patch_info, tutorial_rect, Vector2Zero(), 0.f, WHITE);
        tutorial_rect.x += tutorial_rect.width + TUTORIAL_SMALL_MARGIN;
        DrawTextureNPatch(texture_from_type(renderer, TextureType::KeyRight), patch_info, tutorial_rect, Vector2Zero(), 0.f, WHITE);
    }

    void renderer_render_level_complete(Renderer& renderer, GameState& game_state, MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size)
    {
        i32 fbx = static_cast<i32>(framebuffer_size.x);
        i32 fby = static_cast<i32>(framebuffer_size.y);
        Color overlay_color = BACKGROUND_COLOR;
        overlay_color.a = static_cast<u8>(Lerp(225.f, 0.f, game_state.time_scale * game_state.time_scale));
        DrawRectangle(0, 0, fbx, fby, overlay_color);
        
        auto title_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5}, Vector2 { framebuffer_size.x, 150.f }, Vector2{0.5f, 0.0f});
        title_rect.y -= title_rect.height + 40;
        GuiSetStyle(DEFAULT, TEXT_SIZE, 125);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiLabel(title_rect, "LEVEL COMPLETE");
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, 65);
        auto primary_buttons_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 400.f, 100.f }, Vector2{0.5f, 0.0f});
        auto& next_level_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(GameButtonType::NextLevel));
        if (renderer_ui_button(primary_buttons_rect, "NEXT LEVEL", next_level_hover, audio_state, next_level_hover))
        {
            game_state = game_init(game_state.current_level+1);
        }
    }
    
    void renderer_render_level_fail(Renderer& renderer, GameState& game_state, MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size)
    {
        i32 fbx = static_cast<i32>(framebuffer_size.x);
        i32 fby = static_cast<i32>(framebuffer_size.y);
        Color overlay_color = BACKGROUND_COLOR;
        overlay_color.a = static_cast<u8>(Lerp(225.f, 0.f, game_state.time_scale * game_state.time_scale));
        DrawRectangle(0, 0, fbx, fby, overlay_color);
        
        auto title_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5}, Vector2 { framebuffer_size.x, 150.f }, Vector2{0.5f, 0.0f});
        title_rect.y -= title_rect.height + 40;
        GuiSetStyle(DEFAULT, TEXT_SIZE, 125);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetState(STATE_FOCUSED);
        GuiLabel(title_rect, "NOT QUITE...");
        GuiSetState(STATE_NORMAL);
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, 65);
        auto primary_buttons_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 400.f, 100.f }, Vector2{0.5f, 0.0f});
        auto& try_again_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(GameButtonType::TryAgain));
        if (renderer_ui_button(primary_buttons_rect, "TRY AGAIN", try_again_hover, audio_state, try_again_hover))
        {
            game_state = game_init(game_state.current_level);
        }
    }
    
    void renderer_render_game_won(Renderer& renderer, std::optional<GameState>& game_state,  MenuState& menu_state, AudioState& audio_state, Vector2 framebuffer_size)
    {
        i32 fbx = static_cast<i32>(framebuffer_size.x);
        i32 fby = static_cast<i32>(framebuffer_size.y);
        Color overlay_color = BACKGROUND_COLOR;
        overlay_color.a = static_cast<u8>(Lerp(225.f, 0.f, game_state->time_scale * game_state->time_scale));
        DrawRectangle(0, 0, fbx, fby, overlay_color);
        
        auto title_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5}, Vector2 { framebuffer_size.x, 150.f }, Vector2{0.5f, 0.0f});
        title_rect.y -= title_rect.height + 2 * BUTTON_SPACING;
        GuiSetStyle(DEFAULT, TEXT_SIZE, 125);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiLabel(title_rect, "YOU WON!");
        title_rect.y -= title_rect.height + 2 * BUTTON_SPACING;
        GuiLabel(title_rect, "CONGRATULATIONS!");
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, 65);
        auto primary_buttons_rect = ui_rectangle_from_anchor(framebuffer_size, Vector2{0.5f, 0.5f}, Vector2 { 400.f, 100.f }, Vector2{0.5f, 0.0f});
        auto& back_to_main_menu = menu_state.buttons_hover_state.at(static_cast<size_t>(GameButtonType::BackToMenu));
        if (renderer_ui_button(primary_buttons_rect, "TO MENU", back_to_main_menu, audio_state, back_to_main_menu))
        {
            menu_change_page(menu_state, MenuPageType::MainMenu);
            game_state = std::nullopt;
        }
        primary_buttons_rect.y += primary_buttons_rect.height + BUTTON_SPACING;
        auto& credits_hover = menu_state.buttons_hover_state.at(static_cast<size_t>(GameButtonType::Credits));
        if (renderer_ui_button(primary_buttons_rect, "CREDITS", credits_hover, audio_state, credits_hover))
        {
            menu_change_page(menu_state, MenuPageType::Credits);
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

    MenuState menu_init(MenuPageType page, bool is_fullscreen, ResolutionPreset resolution)
    {
        auto result = MenuState {
            .current_page = page,
            .resolution = resolution,
            .is_fullscreen = is_fullscreen,
            .volume = 1.0f,
            .buttons_hover_state = {}
        };
        return result;
    }

    void menu_change_page(MenuState& menu_state, MenuPageType page)
    {
        if (page != menu_state.current_page)
        {
            menu_state.current_page = page;
            menu_state.buttons_hover_state.fill(false);
        }
    }

    Vector2 menu_resolution_to_size(ResolutionPreset resolution)
    {
        switch (resolution)
        {
        case ResolutionPreset::Resolution_1600x900:
            return Vector2 { 1600, 900 };
        case ResolutionPreset::Resolution_1920x1080:
            return Vector2 { 1920, 1080 };
        }

        // Unhandled resolution preset
        assert(false);
        return Vector2Zero();
    }

    const char* menu_resolution_title(ResolutionPreset resolution)
    {
        switch (resolution)
        {
        case ResolutionPreset::Resolution_1600x900:
            return "1600x900";
        case ResolutionPreset::Resolution_1920x1080:
            return "1920x1080";
        }

        // Unhandled resolution preset
        assert(false);
        return "INVALID";
    }

    f32 ease_in_back(f32 alpha)
    {
        constexpr f32 c1 = 1.70158f;
        constexpr f32 c3 = c1 + 1;
        return c3 * alpha * alpha * alpha - c1 * alpha * alpha;
    }

    f32 ease_in_cubic(f32 alpha)
    {
        return alpha * alpha * alpha;
    }

    AudioState audio_init()
    {
        InitAudioDevice();

        auto result = AudioState {
            .time_till_background_music = 0.f,
            .sounds{},
        };
        result.sounds.at(static_cast<size_t>(AudioType::MusicBackground)) = LoadSound("assets/audio/cozy.ogg");
        
        result.sounds.at(static_cast<size_t>(AudioType::SFXIndestructibleImpact)) = LoadSound("assets/audio/impactTin_medium_004.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::SFXWallImpact)) = LoadSound("assets/audio/impactGlass_medium_004.ogg");
        
        result.sounds.at(static_cast<size_t>(AudioType::SFXSendBall)) = LoadSound("assets/audio/phaserUp3.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::SFXWind)) = LoadSound("assets/audio/phaseJump3.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::SFXBallDisappear)) = LoadSound("assets/audio/phaserDown3.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::SFXWallDisappear)) = LoadSound("assets/audio/pepSound5.ogg");
        
        result.sounds.at(static_cast<size_t>(AudioType::SFXLevelLost)) = LoadSound("assets/audio/error_003.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::SFXLevelWon)) = LoadSound("assets/audio/confirmation_003.ogg");
        
        result.sounds.at(static_cast<size_t>(AudioType::UIPageChange)) = LoadSound("assets/audio/card-slide-6.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::UIButtonHover)) = LoadSound("assets/audio/chips-handle-3.ogg");
        result.sounds.at(static_cast<size_t>(AudioType::UIButtonClick)) = LoadSound("assets/audio/die-throw-1.ogg");

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
        auto& sound = audio_state.sounds.at(static_cast<size_t>(sound_type));
        SetSoundPitch(sound, 1.0f);
        PlaySound(sound);
    }

    void audio_play_sound_randomize_pitch(AudioState& audio_state, AudioType sound_type)
    {
        assert(sound_type < AudioType::MAX_AUDIO_TYPE);
        i32 rand = GetRandomValue(0, 10000);
        f32 frand = static_cast<f32>(rand) / (10000.f * 0.5f) - 1.f;
        constexpr f32 PITCH_VARIATION = 0.10f;
        f32 pitch = 1 + frand * PITCH_VARIATION;
        auto& sound = audio_state.sounds.at(static_cast<size_t>(sound_type));
        SetSoundPitch(sound, pitch);
        PlaySound(sound);
    }

    void audio_set_volume(AudioState& audio_state, f32 volume)
    {
        SetMasterVolume(volume);
    }
}
