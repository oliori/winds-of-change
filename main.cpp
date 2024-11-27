#include "src/windsofchange.cpp"
#include "src/window.cpp"

int main()
{
    auto menu_state = woc::menu_init(woc::MenuPageType::MainMenu, false, woc::ResolutionPreset::Resolution_1600x900);
    auto window = woc::window_init();
    auto renderer = woc::renderer_init();
    auto game_state = std::optional<woc::GameState>{};
    auto audio_state = woc::audio_init();

    GuiLoadStyleBluish();

    bool keep_running_app = true;
    bool is_window_visible = true;
    Vector2 window_size = woc::window_size(window);
    woc::InputState app_input_state{};
    auto update_app = [&window = window, &keep_running_app, &is_window_visible, &window_size, &app_input_state] ()
    {
        PollInputEvents();

        app_input_state.game_menu_swap += IsKeyPressed(KEY_ESCAPE);
        app_input_state.restart_level += IsKeyPressed(KEY_R);
        app_input_state.new_game += IsKeyPressed(KEY_Y);
        app_input_state.send_ball += IsKeyDown(KEY_SPACE);
        
        auto move_left = IsKeyDown(KEY_A);
        auto move_right = IsKeyDown(KEY_D);
        app_input_state.move_dir = static_cast<woc::i32>(move_right) - static_cast<woc::i32>(move_left);
        
        auto wind_up = IsKeyDown(KEY_UP);
        auto wind_down = IsKeyDown(KEY_DOWN);
        auto wind_left = IsKeyDown(KEY_LEFT);
        auto wind_right = IsKeyDown(KEY_RIGHT);
        app_input_state.wind_dir_x = static_cast<woc::i32>(wind_right) - static_cast<woc::i32>(wind_left);
        app_input_state.wind_dir_y = static_cast<woc::i32>(wind_up) - static_cast<woc::i32>(wind_down);

        is_window_visible = woc::window_is_visible(window);
        window_size = woc::window_size(window);

        if (!woc::window_is_running(window))
        {
            keep_running_app = false;
        }
    };

    auto update_game = [&audio_state, &menu_state, &keep_running_app, &game_state, visible = &is_window_visible, window_size = &window_size, &renderer] (woc::InputState input)
    {
        if (input.new_game)
        {
            game_state = woc::game_init(woc::START_LEVEL);
        }
        
        if (input.game_menu_swap)
        {
            if (menu_state.current_page == woc::MenuPageType::Game)
            {
                menu_state.current_page = woc::MenuPageType::MainMenu;
                woc::audio_play_sound_randomize_pitch(audio_state, woc::AudioType::UIPageChange);
            } else
            {
                menu_state.current_page = woc::MenuPageType::Game;
                woc::audio_play_sound_randomize_pitch(audio_state, woc::AudioType::UIPageChange);
            }
        }

        if (game_state && input.restart_level)
        {
            game_state = woc::game_init(game_state->current_level);
            woc::audio_play_sound(audio_state, woc::AudioType::SFXLevelLost);
        }
        
        auto delta_seconds = GetFrameTime();

        if (!IsSoundPlaying(audio_state.sounds.at((size_t)woc::AudioType::MusicBackground)))
        {
            audio_state.time_till_background_music -= delta_seconds;
            if (audio_state.time_till_background_music < 0.f)
            {
                woc::audio_play_sound(audio_state, woc::AudioType::MusicBackground);
                audio_state.time_till_background_music = static_cast<woc::f32>(GetRandomValue(10, 20));
            }
        }
        
        switch (menu_state.current_page)
        {
            case woc::MenuPageType::MainMenu:
            {
                if (*visible)
                {
                    woc::renderer_prepare_rendering(renderer);
                    woc::renderer_update_and_render_menu(renderer, menu_state, game_state, audio_state, *window_size);
                    woc::renderer_finalize_rendering(renderer);
                }
                break;
            }
            case woc::MenuPageType::Game:
            {
                woc::game_update(*game_state, input, audio_state, delta_seconds);
                    
                if (*visible)
                {
                    woc::renderer_prepare_rendering(renderer);
                    woc::renderer_render_world(renderer, *game_state, *window_size);
                    if (game_state->level_status == woc::LevelStatus::Won)
                    {
                        if (game_state->current_level == woc::END_LEVEL)
                        {
                            woc::renderer_render_game_won(renderer, game_state, menu_state, audio_state, *window_size);
                        } else
                        {
                            woc::renderer_render_level_complete(renderer, *game_state, menu_state, audio_state, *window_size);
                        }
                    } else if (game_state->level_status == woc::LevelStatus::Lost)
                    {
                        woc::renderer_render_level_fail(renderer, *game_state, menu_state, audio_state, *window_size);
                    }
                    DrawFPS(20, 20);
                    woc::renderer_finalize_rendering(renderer);
                }
                break;
            }
            case woc::MenuPageType::Settings:
            {
                if (*visible)
                {
                    woc::renderer_prepare_rendering(renderer);
                    woc::renderer_update_and_render_settings(renderer, menu_state, audio_state, *window_size);
                    woc::renderer_finalize_rendering(renderer);
                }
                break;
            }
            case woc::MenuPageType::Quit:
            {
                keep_running_app = false;
                break;
            }
            case woc::MenuPageType::Credits:
            {
                if (*visible)
                {
                    woc::renderer_prepare_rendering(renderer);
                    woc::renderer_update_and_render_credits(renderer, menu_state, audio_state, *window_size);
                    woc::renderer_finalize_rendering(renderer);
                }
                break; 
            }
        }
    };

    while (keep_running_app)
    {
        menu_state.is_fullscreen = woc::window_is_fullscreen(window);
        update_app();
        update_game(app_input_state);
        app_input_state = woc::InputState{};
        
        auto res_size = woc::menu_resolution_to_size(menu_state.resolution);
        if (!menu_state.is_fullscreen && !Vector2Equals(res_size, window_size)) 
        {
            woc::window_set_size(window, res_size);
        }
        woc::window_set_fullscreen(window, menu_state.is_fullscreen);
        woc::audio_set_volume(audio_state, menu_state.volume);
    }

    // Unnecessary before a program exit. OS cleans up.
    woc::renderer_deinit(renderer);
    woc::audio_deinit(audio_state);
    woc::window_deinit(window);

    return 0;
}