#include "src/windsofchange.cpp"
#include "src/window.cpp"

// KNOWN ISSUES:
// - Input lag. Move to another thread?

int main()
{
    // Doesn't need smart pointers as when it goes out of scope, the app exits and OS cleans it up
    auto menu_state = woc::MenuState {
        .current_page = woc::MenuPageType::MainMenu,
    };
    auto window = woc::window_init();
    auto renderer = woc::Renderer{};
    auto game_state = std::optional<woc::GameState>{};
    auto audio_state = woc::audio_init();

    GuiLoadStyleBluish();

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
        
        app_input_state.send_ball += IsKeyPressed(KEY_SPACE);

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
        
        if (input.game_menu_swap % 2 == 1)
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
        
        auto delta_seconds = GetFrameTime();
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
                            woc::renderer_render_game_won(renderer, menu_state, game_state, *window_size);
                        } else
                        {
                            woc::renderer_render_level_complete(renderer, *game_state, *window_size);
                        }
                    } else if (game_state->level_status == woc::LevelStatus::Lost)
                    {
                        woc::renderer_render_level_fail(renderer, *game_state, *window_size);
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

    woc::audio_play_sound(audio_state, woc::AudioType::MusicBackground);
    while (keep_running_app)
    {
        update_app();
        update_game(app_input_state);
        app_input_state = woc::InputState{};
    }

    // Unnecessary before a program exit. OS cleans up.
    woc::audio_deinit(audio_state);
    woc::window_deinit(window);

    return 0;
}