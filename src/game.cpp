
#define m_cpu_side 1
#if !defined(__EMSCRIPTEN__)
#define m_winhttp 1
#endif

#pragma comment(lib, "opengl32.lib")

#pragma warning(push, 0)
// #define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#pragma warning(pop)

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>
#endif // __EMSCRIPTEN__

#if defined(_WIN32)
#pragma warning(push, 0)
#define NOMINMAX
#if !defined(m_winhttp)
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#if defined(m_winhttp)
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif
#undef near
#undef far
#pragma warning(pop)
#endif // _WIN32


#include <stdlib.h>

#include <gl/GL.h>
#if !defined(__EMSCRIPTEN__)
#include "external/glcorearb.h"
#endif

#if defined(__EMSCRIPTEN__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT
#include "external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert
#include "external/stb_truetype.h"
#pragma warning(pop)

#if defined(__EMSCRIPTEN__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif

#if defined(__EMSCRIPTEN__)
#define m_gl_single_channel GL_LUMINANCE
#else // __EMSCRIPTEN__
#define m_gl_single_channel GL_RED
#endif // __EMSCRIPTEN__

#if defined(m_debug)
#define gl(...) __VA_ARGS__; {int error = glGetError(); if(error != 0) { on_gl_error(#__VA_ARGS__, __FILE__, __LINE__, error); }}
#else // m_debug
#define gl(...) __VA_ARGS__
#endif // m_debug

#include "tklib.h"
#include "shared.h"

#include "config.h"
#include "leaderboard.h"
#include "engine.h"
#include "game.h"
#include "shader_shared.h"

#if defined(__EMSCRIPTEN__)
global constexpr b8 c_on_web = true;
#else
global constexpr b8 c_on_web = false;
#endif


global s_platform_data* g_platform_data;
global s_game* game;
global s_v2 g_mouse;
global b8 g_left_click;
global b8 g_right_click;

#include "json.cpp"

#if defined(__EMSCRIPTEN__) || defined(m_winhttp)
#include "leaderboard.cpp"
#endif

#include "engine.cpp"
#include "aqtun.cpp"

m_dll_export void init(s_platform_data* platform_data)
{
	g_platform_data = platform_data;
	game = (s_game*)platform_data->memory;
	unpoison_memory(game, sizeof(s_game));
	game->speed_index = 5;
	game->rng = make_rng(SDL_GetPerformanceCounter());
	game->reload_shaders = true;
	game->speed = 0;
	game->music_speed = {1, 1};

	if(!c_on_web) {
		game->disable_music = true;
	}

	engine_init(platform_data);

	set_state(&game->state0, e_game_state0_main_menu);
	game->do_hard_reset = true;

	#if defined(m_winhttp)
	// @TODO(tkap, 08/08/2025): This might break with hot reloading
	game->session = WinHttpOpen(L"A WinHTTP POST Example/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	assert(game->session);
	#endif

	#if defined(__EMSCRIPTEN__) || defined(m_winhttp)
	load_or_create_leaderboard_id();
	#endif

	// play_sound(e_sound_music, {.loop = true, .speed = game->music_speed.curr});
}

m_dll_export void init_after_recompile(s_platform_data* platform_data)
{
	g_platform_data = platform_data;
	game = (s_game*)platform_data->memory;

	#define X(type, name) name = (type)SDL_GL_GetProcAddress(#name); assert(name);
		m_gl_funcs
	#undef X
}

#if defined(__EMSCRIPTEN__)
EM_JS(int, browser_get_width, (), {
	const { width, height } = canvas.getBoundingClientRect();
	return width;
});

EM_JS(int, browser_get_height, (), {
	const { width, height } = canvas.getBoundingClientRect();
	return height;
});
#endif // __EMSCRIPTEN__

m_dll_export void do_game(s_platform_data* platform_data)
{
	g_platform_data = platform_data;
	game = (s_game*)platform_data->memory;

	f64 delta64 = get_seconds() - game->time_before;
	delta64 = at_most(1.0/20.0, delta64);
	game->time_before = get_seconds();

	{
		#if defined(__EMSCRIPTEN__)
		int width = browser_get_width();
		int height = browser_get_height();
		g_platform_data->window_size.x = width;
		g_platform_data->window_size.y = height;
		if(g_platform_data->prev_window_size.x != width || g_platform_data->prev_window_size.y != height) {
			set_window_size(width, height);
			g_platform_data->window_resized = true;
		}
		g_platform_data->prev_window_size.x = width;
		g_platform_data->prev_window_size.y = height;
		#endif // __EMSCRIPTEN__
	}

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		handle state start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		b8 state_changed = handle_state(&game->state0, game->render_time);
		if(state_changed) {
			game->accumulator += c_update_delay;
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		handle state end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	input();

	float game_speed = c_game_speed_arr[game->speed_index] * game->speed;
	game->accumulator += delta64 * game_speed;
	f64 clamped_accumulator = at_most(c_update_delay * 20, game->accumulator);
	int update_count = 0;
	while(clamped_accumulator >= c_update_delay) {
		game->accumulator -= c_update_delay;
		clamped_accumulator -= c_update_delay;
		update();
		update_count += 1;
	}
	float interp_dt = (float)(clamped_accumulator / c_update_delay);
	render(interp_dt, (float)delta64);

	if(update_count > 0) {
		game->soft_data.hold_input = zero;
	}
	game->soft_data.press_input = zero;
}

func void input()
{
	game->char_events.count = 0;
	game->key_events.count = 0;

	// game->input.left = game->input.left || keyboard_state[SDL_SCANCODE_A];
	// game->input.right = game->input.right || keyboard_state[SDL_SCANCODE_D];

	for(int i = 0; i < c_max_keys; i += 1) {
		game->input_arr[i].half_transition_count = 0;
	}

	g_left_click = false;
	g_right_click = false;
	{
		int x;
		int y;
		SDL_GetMouseState(&x, &y);
		g_mouse = v2(x, y);
		s_rect letterbox = do_letterbox(v2(g_platform_data->window_size), c_world_size);
		g_mouse.x = range_lerp(g_mouse.x, letterbox.x, letterbox.x + letterbox.size.x, 0, c_world_size.x);
		g_mouse.y = range_lerp(g_mouse.y, letterbox.y, letterbox.y + letterbox.size.y, 0, c_world_size.y);
	}

	// s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;

	float soft_update_time = soft_data->update_count * (float)c_update_delay;

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		input start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		u8* keyboard_state = (u8*)SDL_GetKeyboardState(null);
		soft_data->hold_input.left = soft_data->hold_input.left || keyboard_state[SDL_SCANCODE_A];
		soft_data->hold_input.right = soft_data->hold_input.right || keyboard_state[SDL_SCANCODE_D];
		soft_data->hold_input.up = soft_data->hold_input.up || keyboard_state[SDL_SCANCODE_W];
		soft_data->hold_input.down = soft_data->hold_input.down || keyboard_state[SDL_SCANCODE_S];
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		input end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	e_game_state0 state0 = (e_game_state0)get_state(&game->state0);

	SDL_Event event;
	while(SDL_PollEvent(&event) != 0) {
		switch(event.type) {
			case SDL_QUIT: {
				g_platform_data->quit = true;
			} break;

			case SDL_WINDOWEVENT: {
				if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					int width = event.window.data1;
					int height = event.window.data2;
					g_platform_data->window_size.x = width;
					g_platform_data->window_size.y = height;
					g_platform_data->window_resized = true;
				}
			} break;

			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				b8 is_repeat = event.key.repeat != 0;
				int key = event.key.keysym.sym;
				SDL_Scancode scancode = event.key.keysym.scancode;
				if(key == SDLK_LSHIFT) {
					key = c_left_shift;
				}
				else if(key == SDLK_LCTRL) {
					key = c_left_ctrl;
				}
				b8 is_down = event.type == SDL_KEYDOWN;
				handle_key_event(key, is_down, event.key.repeat != 0);

				if(key < c_max_keys) {
					game->input_arr[key].is_down = is_down;
					game->input_arr[key].half_transition_count += 1;
				}
				if(event.type == SDL_KEYDOWN) {

					int entity_type_arr[9] = {
						e_editor_entity_wall, e_editor_entity_end, e_editor_entity_fence,
						e_editor_entity_spike, e_editor_entity_enemy, e_editor_entity_teleport,
						-1, -1, -1,
					};
					for(int i = 0; i < 9; i += 1) {
						int wanted_scancode = SDL_SCANCODE_1 + i;
						if(scancode == wanted_scancode) {
							if(game->in_editor) {
								if(entity_type_arr[i] != -1) {
									game->editor.to_place = maybe((e_editor_entity)entity_type_arr[i]);
								}
							}
						}
					}

					if(key == SDLK_r && !is_repeat) {
						if(event.key.keysym.mod & KMOD_LCTRL) {
							game->do_hard_reset = true;
						}
						else {
							game->do_soft_reset = true;
						}
					}
					else if(scancode == SDL_SCANCODE_L && !is_repeat) {
						#if defined(m_debug)
						game->saving_or_loading_map = maybe((int)e_load);
						#endif
					}
					else if(scancode == SDL_SCANCODE_SPACE && !is_repeat) {
						soft_data->want_to_jump_timestamp = maybe(soft_update_time);
					}
					else if(scancode == SDL_SCANCODE_W) {
						if(game->in_editor) {
							game->editor.cam_pos.y -= 20;
						}
						else if(!is_repeat) {
							soft_data->want_to_move_forward_timestamp = maybe(soft_update_time);
						}
					}
					else if(scancode == SDL_SCANCODE_S) {
						if(event.key.keysym.mod & KMOD_LCTRL) {
							game->saving_or_loading_map = maybe((int)e_save);
						}
						else if(game->in_editor) {
							game->editor.cam_pos.y += 20;
						}
					}
					else if(scancode == SDL_SCANCODE_A) {
						if(game->in_editor) {
							game->editor.cam_pos.x -= 20;
						}
						else if(!is_repeat) {
							soft_data->want_to_move_left_timestamp = maybe(soft_update_time);
						}
					}
					else if(scancode == SDL_SCANCODE_D) {
						if(game->in_editor) {
							game->editor.cam_pos.x += 20;
						}
						else if(!is_repeat) {
							soft_data->want_to_move_right_timestamp = maybe(soft_update_time);
						}
					}
					else if(scancode == SDL_SCANCODE_F) {
						if(!is_repeat) {
							soft_data->want_to_attack_timestamp = maybe(soft_update_time);
						}
					}
					else if(scancode == SDL_SCANCODE_LEFT && !is_repeat) {
					}
					else if(scancode == SDL_SCANCODE_RIGHT && !is_repeat) {
					}
					else if(scancode == SDL_SCANCODE_UP && !is_repeat) {
						if(game->saving_or_loading_map.valid) {
							game->map_to_save_or_load_index = circular_index(game->map_to_save_or_load_index - 1, c_map_count);
						}
					}
					else if(scancode == SDL_SCANCODE_DOWN && !is_repeat) {
						if(game->saving_or_loading_map.valid) {
							game->map_to_save_or_load_index = circular_index(game->map_to_save_or_load_index + 1, c_map_count);
						}
					}
					else if(scancode == SDL_SCANCODE_RETURN && !is_repeat) {
						if(game->saving_or_loading_map.valid) {
							if(game->saving_or_loading_map.value == e_save) {
								save_map(game->map_to_save_or_load_index);
							}
							else {
								load_map(game->map_to_save_or_load_index);
							}
							game->saving_or_loading_map = zero;
						}
					}
					else if(scancode == SDL_SCANCODE_Q && !is_repeat) {
						soft_data->press_input.q = true;
					}
					else if(scancode == SDL_SCANCODE_ESCAPE && !is_repeat) {
						breakable_block {
							if(game->saving_or_loading_map.valid) {
								game->saving_or_loading_map = zero;
								break;
							}
							if(state0 == e_game_state0_play) {
								if(game->in_editor) {
									game->editor.to_place = zero;
								}
								else {
									add_state(&game->state0, e_game_state0_pause);
								}
								break;
							}
							if(state0 == e_game_state0_pause) {
								pop_state_transition(&game->state0, game->render_time, c_transition_time);
								break;
							}
						}
					}
					else if((scancode == SDL_SCANCODE_O || scancode == SDL_SCANCODE_P) && !is_repeat) {
						if(state0 == e_game_state0_play) {
							add_state(&game->state0, e_game_state0_pause);
						}
						else if(state0 == e_game_state0_pause) {
							pop_state_transition(&game->state0, game->render_time, c_transition_time);
						}
					}

					#if defined(m_debug)
					else if(key == SDLK_s && !is_repeat && event.key.keysym.mod & KMOD_LCTRL) {
					}
					else if(key == SDLK_KP_PLUS) {
						game->speed_index = circular_index(game->speed_index + 1, array_count(c_game_speed_arr));
						printf("Game speed: %f\n", c_game_speed_arr[game->speed_index]);
					}
					else if(key == SDLK_KP_MINUS) {
						game->speed_index = circular_index(game->speed_index - 1, array_count(c_game_speed_arr));
						printf("Game speed: %f\n", c_game_speed_arr[game->speed_index]);
					}
					else if(key == SDLK_F1 && !is_repeat) {
						game->cheat_menu_enabled = !game->cheat_menu_enabled;
					}
					else if(key == SDLK_F2 && !is_repeat) {
						#if defined(m_debug)
						game->in_editor = !game->in_editor;
						#endif
					}
					else if(key == SDLK_j && !is_repeat) {
					}
					else if(key == SDLK_h && !is_repeat) {
					}
					#endif // m_debug

				}

				// @Note(tkap, 28/06/2025): Key up
				else {
					if(key == SDLK_SPACE) {
					}
				}
			} break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 1) {
					g_left_click = true;
				}
				if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 3) {
					g_right_click = true;
				}
				// int key = sdl_key_to_windows_key(event.button.button);
				// b8 is_down = event.type == SDL_MOUSEBUTTONDOWN;
				// handle_key_event(key, is_down, false);

				if(event.button.button == 1) {
					int key = c_left_button;
					game->input_arr[key].is_down = event.type == SDL_MOUSEBUTTONDOWN;
					game->input_arr[key].half_transition_count += 1;
				}

				else if(event.button.button == 3) {
					int key = c_right_button;
					game->input_arr[key].is_down = event.type == SDL_MOUSEBUTTONDOWN;
					game->input_arr[key].half_transition_count += 1;
				}

			} break;

			case SDL_TEXTINPUT: {
				char c = event.text.text[0];
				game->char_events.add((char)c);
			} break;

			case SDL_MOUSEWHEEL: {
				// float x = range_lerp((float)event.wheel.y, -1, 1, 0.9f, 1.1f);
				// if(state0 == e_game_state0_play) {
				// 	soft_data->wanted_zoom *= x;
				// 	soft_data->wanted_zoom = clamp(soft_data->wanted_zoom, 0.125f, 4.0f);
				// }
			} break;
		}
	}
}

func void update()
{
	reset_linear_arena(&game->update_frame_arena);

	e_game_state0 state0 = (e_game_state0)get_state(&game->state0);

	s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;
	auto entity_arr = &soft_data->entity_arr;

	float delta = (float)c_update_delay;

	if(game->do_hard_reset) {
		game->do_hard_reset = false;
		game->do_soft_reset = true;
		memset(hard_data, 0, sizeof(*hard_data));
		game->update_time = 0;
		reset_linear_arena(&game->arena);
		for_enum(type_i, e_entity) {
			entity_manager_reset(entity_arr, type_i);
		}

		game->music_speed.target = 1;
	}
	if(game->do_soft_reset) {
		game->do_soft_reset = false;
		memset(soft_data, 0, sizeof(*soft_data));
		reset_linear_arena(&game->arena);

		for_enum(type_i, e_entity) {
			entity_manager_reset(&soft_data->entity_arr, type_i);
		}

		{
			s_entity player = zero;
			reset_player_pos(&player);
			entity_manager_add(&soft_data->entity_arr, e_entity_player, player);
		}

		load_map(hard_data->current_map);
	}

	b8 should_do_game = false;
	switch(state0) {
		case e_game_state0_play: {
			should_do_game = true;
		} break;

		default: {}
	}
	if(game->saving_or_loading_map.valid) {
		should_do_game = false;
	}

	for(int i = 0; i < c_max_entities; i += 1) {
		if(entity_arr->active[i]) {
			entity_arr->data[i].prev_pos = entity_arr->data[i].pos;
		}
	}

	if(soft_data->frames_to_freeze > 0) {
		soft_data->frames_to_freeze -= 1;
		return;
	}

	float soft_update_time = soft_data->update_count * (float)c_update_delay;

	if(should_do_game) {
		s_entity* player = &entity_arr->data[0];

		if(game->in_editor) {
		}
		else {

			b8 do_a_turn = false;

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				b8 can_act = false;
				if(soft_update_time >= soft_data->next_action_time && soft_update_time <= soft_data->next_action_time + c_action_grace_period){
					can_act = true;
				}
				if(soft_data->num_free_actions > 0) {
					can_act = true;
				}

				b8 wanted_to_perform_action = false;

				if(can_act) {
					soft_data->draw_signal = true;
				}
				else {
					soft_data->draw_signal = false;
				}

				b8 advance_next_action_time = false;

				if(!advance_next_action_time && check_action_maybe(soft_update_time, soft_data->want_to_move_forward_timestamp, 0.0f)) {
					wanted_to_perform_action = true;
					if(can_act) {
						soft_data->want_to_move_forward_timestamp = zero;
						move_forward(player, true);
						advance_next_action_time = true;
						play_sound_at_speed(e_sound_walk, get_rand_sound_speed(1.1f, &game->rng));
					}
				}

				if(!advance_next_action_time && check_action_maybe(soft_update_time, soft_data->want_to_move_left_timestamp, 0.0f)) {
					wanted_to_perform_action = true;
					if(can_act) {
						play_sound_at_speed(e_sound_walk, get_rand_sound_speed(1.1f, &game->rng));
						s_v2i player_tile = tile_index_from_3d(player->target_pos - v3(1, 0, 0));
						s_entity* wall = get_wall_at_tile(player_tile);
						soft_data->want_to_move_left_timestamp = zero;
						advance_next_action_time = true;
						if(!wall) {
							s_v3 target_pos = player->target_pos;
							target_pos.x -= 1;
							player->target_pos = target_pos;
						}
					}
				}

				if(!advance_next_action_time && check_action_maybe(soft_update_time, soft_data->want_to_move_right_timestamp, 0.0f)) {
					wanted_to_perform_action = true;
					if(can_act) {
						play_sound_at_speed(e_sound_walk, get_rand_sound_speed(1.1f, &game->rng));
						s_v2i player_tile = tile_index_from_3d(player->target_pos + v3(1, 0, 0));
						s_entity* wall = get_wall_at_tile(player_tile);
						soft_data->want_to_move_right_timestamp = zero;
						advance_next_action_time = true;
						if(!wall) {
							s_v3 target_pos = player->target_pos;
							target_pos.x += 1;
							player->target_pos = target_pos;
						}
					}
				}

				if(!advance_next_action_time && check_action_maybe(soft_update_time, soft_data->want_to_jump_timestamp, 0.0f)) {
					wanted_to_perform_action = true;
					if(can_act) {
						soft_data->want_to_jump_timestamp = zero;
						play_sound_at_speed(e_sound_jump, get_rand_sound_speed(1.1f, &game->rng));
						advance_next_action_time = true;
						jump_forward(player);
						player->is_jumping = maybe(soft_update_time);
					}
				}

				if(!advance_next_action_time && check_action_maybe(soft_update_time, soft_data->want_to_attack_timestamp, 0.0f)) {
					wanted_to_perform_action = true;
					if(can_act) {
						soft_data->want_to_attack_timestamp = zero;
						advance_next_action_time = true;
						s_maybe<int> enemy = get_closest_enemy_in_attack_range(player->pos);
						soft_data->last_attack_timestamp = maybe(game->render_time);
						if(enemy.valid) {
							kill_enemy(enemy.value);
							game->soft_data.num_free_actions = 2;
							play_sound_at_speed(rand_bool(&game->rng) ? e_sound_kill1 : e_sound_kill2, get_rand_sound_speed(1.1f, &game->rng));
						}
						else {
							play_sound(e_sound_fail_attack);
						}
					}
				}

				if(advance_next_action_time) {
					float diff = soft_update_time - soft_data->next_action_time;
					soft_data->next_action_time += c_action_interval + diff;
					do_a_turn = true;
				}

				if(wanted_to_perform_action && !can_act) {
					play_sound_at_speed(e_sound_fail_action, get_rand_sound_speed(1.1f, &game->rng));
					{
						s_transition t = zero;
						t.active = true;
						t.timestamp = game->render_time;
						t.duration = 0.5f;
						soft_data->fail_action_effect = t;
					}
					soft_data->next_action_time = at_most(soft_update_time + 1.5f, soft_data->next_action_time + (float)c_update_delay * 10);
				}

				if(soft_update_time > soft_data->next_action_time + c_action_grace_period) {
					soft_data->next_action_time += c_action_interval + c_action_grace_period;
					do_a_turn = true;
					move_forward(player, true);
					play_sound_at_speed(e_sound_walk, get_rand_sound_speed(1.1f, &game->rng));
				}

				if(wanted_to_perform_action && soft_data->num_free_actions > 0) {
					soft_data->next_action_time = soft_update_time - (float)c_update_delay;
					soft_data->num_free_actions -= 1;
				}

				player->pos = lerp_v3(player->pos, player->target_pos, delta * 10);

				if(check_for_win(player->target_pos) && !will_win_soon()) {
					soft_data->win_timestamp = maybe(soft_update_time);
					play_sound(e_sound_beat_level);
					{
						s_transition t = zero;
						t.active = true;
						t.timestamp = game->render_time;
						t.duration = c_win_transition_duration * 2;
						hard_data->map_win_transition = t;
					}
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update pickups start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			for(int i = c_first_index[e_entity_pickup]; i < c_last_index_plus_one[e_entity_pickup]; i += 1) {
				if(!entity_arr->active[i]) {
					continue;
				}
				s_entity* entity = &entity_arr->data[i];
				if(entity->pickup_type == e_pickup_spike) {
					if(float_equals(entity->dir.x, 0.0f)) {
						entity->dir.x = 1;
					}
					if(do_a_turn) {
						s_v3 new_pos = entity->target_pos;
						new_pos.x += entity->dir.x;
						s_v2i index = tile_index_from_3d(new_pos);
						s_entity* wall = get_wall_at_tile(index);
						if(wall) {
							entity->dir.x *= -1;
						}
						entity->target_pos.x += entity->dir.x;
					}
					entity->pos = lerp_v3(entity->pos, entity->target_pos, delta * 10);
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update pickups end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
				if(!entity_arr->active[i]) {
					continue;
				}
				s_entity* entity = &entity_arr->data[i];
				s_v3 target_pos = player->pos;
				target_pos.y = 0;
				s_v3 source_pos = entity->pos;
				source_pos.y = 0;
				entity->pos = go_towards_v3(source_pos, target_pos, delta);

				float distance = v3_distance(source_pos, target_pos);
				if(distance <= c_enemy_range) {
					start_losing(0.0f);
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			{
				s_v2i player_tile = tile_index_from_3d(player->target_pos);
				{
					s_entity* spike = get_spike_at_tile(player_tile);
					if(spike && !will_win_soon()) {
						start_losing(0.25f);
					}
				}
				if(!will_teleport_soon()) {
					s_entity* teleport = get_teleport_at_tile(player_tile);
					if(teleport) {
						b8 on_cooldown = check_action_maybe(soft_update_time, teleport->last_teleport_timestamp, c_teleport_cooldown);
						if(!on_cooldown) {
							s_entity* other = find_closest_teleporter_to(teleport);
							assert(other);

							soft_data->teleport_timestamp = maybe(soft_update_time);
							soft_data->teleport_destination = v2(other->pos.x, other->pos.z);

							teleport->last_teleport_timestamp = maybe(soft_update_time);
							other->last_teleport_timestamp = maybe(soft_update_time);

							play_sound_at_speed(e_sound_teleport, get_rand_sound_speed(1.1f, &game->rng));

							{
								s_v3 pos_arr[] = {
									teleport->pos, other->pos
								};
								for(int i = 0; i < 2; i += 1) {
									s_entity emitter = make_teleporter_particles(pos_arr[i] + v3(0, 0.5f, 0));
									emitter.emitter_a.particle_duration *= 2;
									emitter.emitter_a.radius *= 2;
									emitter.emitter_a.speed *= 4;
									emitter.emitter_b.particle_count = 128;
									add_emitter(emitter);
								}
							}
						}
					}
				}
			}

			if(!will_win_soon() && !will_lose_soon()) {
				s_time_data data = get_time_data_maybe(soft_update_time, soft_data->teleport_timestamp, 0.25f);
				if(data.percent >= 1) {
					soft_data->teleport_timestamp = zero;
					s_v3 pos = v3(soft_data->teleport_destination.x, player->pos.y, soft_data->teleport_destination.y);
					player->target_pos = pos;
					teleport_entity(player, pos);
				}
			}

			if(!will_win_soon()) {
				s_time_data data = get_time_data_maybe(soft_update_time, soft_data->lose_timestamp, soft_data->lose_delay);
				if(data.percent >= 1) {
					game->do_soft_reset = true;
				}
			}

			{
				s_time_data data = get_time_data_maybe(soft_update_time, soft_data->win_timestamp, c_win_transition_duration);
				if(data.percent >= 1) {
					hard_data->current_map += 1;
					game->do_soft_reset = true;
				}
			}

		}

		game->update_time += (float)c_update_delay;
		hard_data->update_count += 1;
		soft_data->update_count += 1;
	}

	if(hard_data->current_map >= c_map_count) {
		hard_data->current_map = 0;
		if(game->leaderboard_nice_name.count <= 0 && c_on_web) {
			add_temporary_state_transition(&game->state0, e_game_state0_input_name, game->render_time, c_transition_time);
		}
		else if(!soft_data->tried_to_submit_score) {
			soft_data->tried_to_submit_score = true;
			add_state_transition(&game->state0, e_game_state0_win_leaderboard, game->render_time, c_transition_time);
			game->update_count_at_win_time = hard_data->update_count;

			#if defined(__EMSCRIPTEN__)
			submit_leaderboard_score(hard_data->update_count, c_leaderboard_id);
			#elif defined(m_winhttp)
			get_leaderboard(c_leaderboard_id);
			#endif
		}
	}
}

func void render(float interp_dt, float delta)
{
	pre_render(delta);

	s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;
	auto entity_arr = &soft_data->entity_arr;

	float soft_update_time = soft_data->update_count * (float)c_update_delay;

	s_entity player = entity_arr->data[0];
	s_v3 player_pos = lerp_v3(player.prev_pos, player.pos, interp_dt);

	s_m4 ortho = make_orthographic(0, c_world_size.x, c_world_size.y, 0, -100, 100);
	s_m4 view_3d = look_at(player_pos + v3(0, 2, 3), player_pos, c_y_axis);
	s_m4 perspective = make_perspective(45.0f, c_aspect_ratio, 0.01f, 100.0f);
	s_m4 view_2d = m4_translate(v3(-game->editor.cam_pos.x, -game->editor.cam_pos.y, 0.0f));
	s_m4 view_inv = m4_inverse(view_2d);
	s_m4 view_projection = m4_multiply(ortho, view_2d);

	s_v2 world_mouse = v2_multiply_m4(g_mouse, view_inv);

	clear_framebuffer_color(0, make_rrr(0.0f));

	clear_framebuffer_color(game->game_fbo.id, v4(0, 0, 0, 0));
	clear_framebuffer_depth(game->game_fbo.id);

	e_game_state0 state0 = (e_game_state0)get_state(&game->state0);

	b8 should_do_game = false;
	b8 do_defeat = false;

	float wanted_speed = get_wanted_game_speed(interp_dt);

	b8 should_do_main_menu = state0 == e_game_state0_main_menu;
	b8 should_do_pause_menu = state0 == e_game_state0_pause;
	b8 should_do_leaderboard_menu = state0 == e_game_state0_leaderboard || state0 == e_game_state0_win_leaderboard;
	b8 should_do_win_leaderboard_menu = state0 == e_game_state0_win_leaderboard;
	b8 should_do_options_menu = state0 == e_game_state0_options;
	b8 should_do_play = state0 == e_game_state0_play;
	b8 should_do_input_name_menu = state0 == e_game_state0_input_name;

	if(should_do_main_menu) {
		game->speed = 0;
		game->music_speed.target = 1;

		if(do_button(S("Play"), wxy(0.5f, 0.5f), true) == e_button_result_left_click || is_key_pressed(SDLK_RETURN, true)) {
			add_state_transition(&game->state0, e_game_state0_play, game->render_time, c_transition_time);
			game->do_hard_reset = true;
		}

		if(do_button(S("Leaderboard"), wxy(0.5f, 0.6f), true) == e_button_result_left_click) {
			#if defined(__EMSCRIPTEN__) || defined(m_winhttp)
			get_leaderboard(c_leaderboard_id);
			#endif
			add_state_transition(&game->state0, e_game_state0_leaderboard, game->render_time, c_transition_time);
		}

		if(do_button(S("Options"), wxy(0.5f, 0.7f), true) == e_button_result_left_click) {
			add_state_transition(&game->state0, e_game_state0_options, game->render_time, c_transition_time);
		}

		// draw_text(c_game_name, wxy(0.5f, 0.2f), 128, make_rrr(1), true, &game->font, zero, 0);
		draw_text(S("www.twitch.tv/Tkap1"), wxy(0.5f, 0.3f), 32, make_rrr(0.6f), true, &game->font, zero, 0);

		if(c_on_web) {
			s_v4 color = hsv_to_rgb(game->render_time * 360, 1, 1);
			draw_text(S("Go fullscreen!\n             V"), wxy(0.9f, 0.93f), sin_range(32, 40, game->render_time * 8), color, true, &game->font, zero, 0);
		}

		{
			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_yes_write;
			render_flush(data, true, 0);
		}
	}

	if(should_do_pause_menu) {
		game->speed = 0;
		game->music_speed.target = 1;

		if(do_button(S("Resume"), wxy(0.5f, 0.5f), true) == e_button_result_left_click || is_key_pressed(SDLK_RETURN, true)) {
			pop_state_transition(&game->state0, game->render_time, c_transition_time);
		}

		if(do_button(S("Leaderboard"), wxy(0.5f, 0.6f), true) == e_button_result_left_click) {
			#if defined(__EMSCRIPTEN__)
			get_leaderboard(c_leaderboard_id);
			#endif
			add_state_transition(&game->state0, e_game_state0_leaderboard, game->render_time, c_transition_time);
		}

		if(do_button(S("Options"), wxy(0.5f, 0.7f), true) == e_button_result_left_click) {
			add_state_transition(&game->state0, e_game_state0_options, game->render_time, c_transition_time);
		}

		// draw_text(c_game_name, wxy(0.5f, 0.2f), 128, make_rrr(1), true, &game->font, zero, 0);
		draw_text(S("www.twitch.tv/Tkap1"), wxy(0.5f, 0.3f), 32, make_rrr(0.6f), true, &game->font, zero, 0);

		{
			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_yes_write;
			render_flush(data, true, 0);
		}
	}

	if(should_do_leaderboard_menu) {
		game->speed = 0;
		game->music_speed.target = 1;
		do_leaderboard();
	}

	if(should_do_win_leaderboard_menu) {

		{
			s_time_format data = update_count_to_time_format(game->update_count_at_win_time, false);
			s_len_str text = format_text("%02i:%02i.%03i", data.minutes, data.seconds, data.milliseconds);
			draw_text(text, c_world_center * v2(1.0f, 0.2f), 64, make_rrr(1), true, &game->font, zero, 0);

			draw_text(S("Press R to restart..."), c_world_center * v2(1.0f, 0.4f), sin_range(48, 60, game->render_time * 8.0f), make_rrr(0.66f), true, &game->font, zero, 0);
		}

		b8 want_to_reset = is_key_pressed(SDLK_r, true);
		if(
			do_button(S("Restart"), c_world_size * v2(0.87f, 0.82f), true) == e_button_result_left_click
			|| is_key_pressed(SDLK_ESCAPE, true) || want_to_reset
		) {
			pop_state_transition(&game->state0, game->render_time, c_transition_time);
			game->do_hard_reset = true;
		}

		{
			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_yes_write;
			render_flush(data, true, 0);
		}
	}

	if(should_do_leaderboard_menu) {
		s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
		data.projection = ortho;
		data.blend_mode = e_blend_mode_normal;
		data.depth_mode = e_depth_mode_no_read_yes_write;
		render_flush(data, true, 0);
	}

	if(should_do_options_menu) {
		game->speed = 0;
		game->music_speed.target = 1;

		s_v2 button_size = v2(600, 48);
		s_container container = make_down_center_x_container(make_rect(wxy(0.0f, 0.05f), c_world_size), button_size, 32);

		do_basic_options(&container, button_size);

		{
			s_len_str text = format_text("Lights: %s", game->disable_lights ? "Off" : "On");
			do_bool_button_ex(text, container_get_pos_and_advance(&container), button_size, false, &game->disable_lights);
		}

		b8 escape = is_key_pressed(SDLK_ESCAPE, true);
		if(do_button(S("Back"), wxy(0.87f, 0.92f), true) == e_button_result_left_click || escape) {
			pop_state_transition(&game->state0, game->render_time, c_transition_time);
		}

		{
			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_yes_write;
			render_flush(data, true, 0);
		}
	}

	if(should_do_play) {
		game->speed = wanted_speed;
		game->music_speed.target = 1;
		should_do_game = true;
	}

	if(should_do_input_name_menu) {
		game->speed = 0;
		game->music_speed.target = 1;
		s_input_name_state* state = &game->input_name_state;
		float font_size = 36;
		s_v2 pos = c_world_size * v2(0.5f, 0.4f);

		int count_before = state->name.str.count;
		b8 submitted = handle_string_input(&state->name, game->render_time);
		int count_after = state->name.str.count;
		if(count_before != count_after) {
			play_sound(e_sound_key);
		}
		if(submitted) {
			b8 can_submit = true;
			if(state->name.str.count < 2) {
				can_submit = false;
				cstr_into_builder(&state->error_str, "Name must have at least 2 characters!");
			}
			if(can_submit) {
				state->error_str.count = 0;
				#if defined(__EMSCRIPTEN__)
				set_leaderboard_name(builder_to_len_str(&state->name.str));
				#endif
				{
					s_len_str temp = builder_to_len_str(&state->name.str);
					game->leaderboard_nice_name.count = 0;
					builder_add(&game->leaderboard_nice_name, "%.*s", expand_str(temp));
				}
			}
		}

		draw_text(S("Victory!"), c_world_size * v2(0.5f, 0.1f), font_size, make_rrr(1), true, &game->font, zero, 0);
		draw_text(S("Enter your name"), c_world_size * v2(0.5f, 0.2f), font_size, make_rrr(1), true, &game->font, zero, 0);
		if(state->error_str.count > 0) {
			draw_text(builder_to_len_str(&state->error_str), c_world_size * v2(0.5f, 0.3f), font_size, hex_to_rgb(0xD77870), true, &game->font, zero, 0);
		}

		if(state->name.str.count > 0) {
			draw_text(builder_to_len_str(&state->name.str), pos, font_size, make_rrr(1), true, &game->font, zero, 0);
		}

		s_v2 full_text_size = get_text_size(builder_to_len_str(&state->name.str), &game->font, font_size);
		s_v2 partial_text_size = get_text_size_with_count(builder_to_len_str(&state->name.str), &game->font, font_size, state->name.cursor.value, 0);
		s_v2 cursor_pos = v2(
			-full_text_size.x * 0.5f + pos.x + partial_text_size.x,
			pos.y - font_size * 0.5f
		);

		s_v2 cursor_size = v2(15.0f, font_size);
		float t = game->render_time - max(state->name.last_action_time, state->name.last_edit_time);
		b8 blink = false;
		constexpr float c_blink_rate = 0.75f;
		if(t > 0.75f && fmodf(t, c_blink_rate) >= c_blink_rate / 2) {
			blink = true;
		}
		float t2 = clamp(game->render_time - state->name.last_edit_time, 0.0f, 1.0f);
		s_v4 color = lerp_color(hex_to_rgb(0xffdddd), multiply_rgb_clamped(hex_to_rgb(0xABC28F), 0.8f), 1 - powf(1 - t2, 3));
		float extra_height = ease_out_elastic2_advanced(t2, 0, 0.75f, 20, 0);
		cursor_size.y += extra_height;

		if(!state->name.visual_pos_initialized) {
			state->name.visual_pos_initialized = true;
			state->name.cursor_visual_pos = cursor_pos;
		}
		else {
			state->name.cursor_visual_pos = lerp_snap(state->name.cursor_visual_pos, cursor_pos, delta * 20);
		}

		if(!blink) {
			draw_rect_topleft(state->name.cursor_visual_pos - v2(0.0f, extra_height / 2), cursor_size, color, 0);
		}

		s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
		data.projection = ortho;
		data.blend_mode = e_blend_mode_normal;
		data.depth_mode = e_depth_mode_no_read_no_write;
		render_flush(data, true, 0);
	}

	if(should_do_game) {

		b8 do_game_ui = true;

		if(game->saving_or_loading_map.valid) {

			if(game->saving_or_loading_map.value == e_save) {
				draw_text(S("SAVING"), wxy(0.5f, 0.05f), 64, make_rrr(1), true, &game->font, zero, 0);
			}
			else {
				draw_text(S("LOADING"), wxy(0.5f, 0.05f), 64, make_rrr(1), true, &game->font, zero, 0);
			}
			s_v2 pos = wxy(0.05f, 0.1f);
			for(int map_i = 0; map_i < c_map_count; map_i += 1) {
				s_v4 color = make_rrr(0.5f);
				if(game->map_to_save_or_load_index == map_i) {
					color = make_rrr(1);
				}
				float font_size = 32;
				s_len_str str = format_text("Map %i", map_i + 1);
				draw_text(str, pos, font_size, color, false, &game->font, zero, 0);
				pos.y += font_size;
			}

			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true, 0);
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		editor start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		if(!game->saving_or_loading_map.valid && game->in_editor) {
			s_editor* editor = &game->editor;

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		editor background start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				s_v2 topleft_v = v2_multiply_m4(v2(0), view_inv);
				s_v2 bottomright_v = v2_multiply_m4(c_world_size, view_inv);
				s_v2i topleft = v2i(floorfi(topleft_v.x / (float)c_tile_size), floorfi(topleft_v.y / (float)c_tile_size));
				s_v2i bottomright = v2i(ceilfi(bottomright_v.x / (float)c_tile_size), ceilfi(bottomright_v.y / (float)c_tile_size));

				for(int y = topleft.y; y < bottomright.y; y += 1) {
					for(int x = topleft.x; x < bottomright.x; x += 1) {
						s_v2 pos = v2(x * c_tile_size, y * c_tile_size);
						b8 inside_map = x >= 0 && x < c_map_width && y >= 0 && y < c_map_height;
						float white = inside_map ? 0.25f : 0.1f;
						if((x + y) % 2 == 0) {
							white = inside_map ? 0.15f : 0.01f;
						}
						draw_rect_topleft(pos, c_tile_size_v, make_rrr(white), 0);
					}
				}

				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.view = view_2d;
				data.projection = ortho;
				data.blend_mode = e_blend_mode_disabled;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true, 0);
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		editor background end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			s_v2i mouse_tile = v2i(floorfi(world_mouse.x / c_tile_size), floorfi(world_mouse.y / c_tile_size));
			s_v2 mouse_tile_v = v2(mouse_tile.x * c_tile_size, mouse_tile.y * c_tile_size);
			b8 valid_index = is_valid_2d_index(mouse_tile, c_map_width, c_map_height);
			if(editor->to_place.valid) {
				draw_rect_topleft(mouse_tile_v, c_tile_size_v, make_rgb(0, 1, 0), 0);
				if(is_key_down(c_left_button) && valid_index) {
					switch(editor->to_place.value) {
						xcase e_editor_entity_wall: {
							set_editor_entity(mouse_tile, e_entity_wall, 0);
						};

						xcase e_editor_entity_fence: {
							set_editor_entity(mouse_tile, e_entity_wall, 1);
						};

						xcase e_editor_entity_end: {
							set_editor_entity(mouse_tile, e_entity_pickup, e_pickup_end);
						}

						xcase e_editor_entity_spike: {
							set_editor_entity(mouse_tile, e_entity_pickup, e_pickup_spike);
						}

						xcase e_editor_entity_enemy: {
							set_editor_entity(mouse_tile, e_entity_enemy, 0);
						}

						xcase e_editor_entity_teleport: {
							set_editor_entity(mouse_tile, e_entity_pickup, e_pickup_teleport);
						}
					}
				}
			}
			if(is_key_down(c_right_button) && valid_index) {
				editor->map.active[mouse_tile.y][mouse_tile.x] = false;
			}

			for(int y = 0; y < c_map_height; y += 1) {
				for(int x = 0; x < c_map_width; x += 1) {
					if(editor->map.active[y][x]) {
						draw_entity_2d(editor->map.entity_arr[y][x], x, y);
					}
				}
			}

			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.view = view_2d;
			data.projection = ortho;
			data.blend_mode = e_blend_mode_disabled;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true, 0);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		editor end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		else if(!game->saving_or_loading_map.valid) {
			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw ground start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				draw_plane(
					v3(-100, 0, -100),
					v3(100, 0, -100),
					v3(-100, 0, 100),
					v3(100, 0, 100),
					make_rrr(0.25f),
					0
				);

				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.view = view_3d;
				data.fbo = game->game_fbo;
				data.projection = perspective;
				data.blend_mode = e_blend_mode_disabled;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true, 0);
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw ground end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			{
				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				{
					s_v3 pos = player_pos;
					if(player.is_jumping.valid) {
						float passed = update_time_to_render_time(soft_update_time, interp_dt) - player.is_jumping.value;
						pos.y += sinf(passed / c_action_interval * c_pi);
						if(passed >= c_action_interval) {
							get_player()->is_jumping = zero;
							play_sound_at_speed(e_sound_landing, get_rand_sound_speed(1.1f, &game->rng));
						}
					}
					s_v2 size = c_player_size * 1.5f;
					pos.y += size.y * 0.5f;
					s_v2i frame_arr[] = {
						v2i(0, 0), v2i(1, 0), v2i(2, 0), v2i(1, 0), v2i(3, 0), v2i(4, 0), v2i(3, 0),
					};
					static float animation_time = 0;
					animation_time += delta * 20;
					s_v2i frame = frame_arr[floorfi(animation_time) % array_count(frame_arr)];
					draw_billboard_ex(game->atlas2, pos, size, frame, make_rrr(1), c_pi, zero, 0);

					{
						s_time_data data = get_time_data_maybe(game->render_time, soft_data->last_attack_timestamp, 0.25f);
						if(soft_data->last_attack_timestamp.valid) {

							constexpr int c_count = 64;
							for(int i = 0; i < c_count; i += 1) {
								float t = data.percent * c_pi * 2 + c_half_pi;
								float it = i / (float)(c_count - 1);
								t -= (1.0f - it) * 2;
								s_v3 temp_pos = pos;
								float coss = cosf(t);
								float sinn = sinf(t);
								temp_pos.x += coss * 0.25f;
								temp_pos.z += sinn * 0.25f;
								s_m4 model = m4_translate(temp_pos);
								model *= m4_rotate(c_half_pi - t, c_z_axis);
								model *= m4_scale(v3(size, 1.0f));

								if(i == c_count - 1) {
									draw_atlas_model(game->atlas2, model, v2i(0, 4), make_rrr(1), zero, 0);
								}
								else {
									draw_atlas_model(game->atlas2, model, v2i(0, 4), make_ra(1.0f, 0.03f), zero, 1);
								}
								if(i >= c_count * 0.75f) {
									s_v3 emitter_pos = pos;
									temp_pos.x += coss * 0.5f;
									temp_pos.z += sinn * 0.5f;
									s_entity emitter = make_sword_particles(temp_pos);
									add_emitter(emitter);
								}
							}

							if(data.percent >= 1) {
								soft_data->last_attack_timestamp = zero;
							}
						}
					}
				}
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				for(int y = 0; y < c_map_height; y += 1) {
					b8 anything_active = false;
					for(int x = 0; x < c_map_width + 1; x += 1) {

						if(x < c_map_width) {
							if(game->map.active[y][x]) {
								anything_active = true;
							}
						}

						constexpr e_mesh mesh_arr[] = {
							e_mesh_aqtun_empty,
							e_mesh_aqtun_corner,
							e_mesh_aqtun_diagonal,
							e_mesh_aqtun_side,
							e_mesh_aqtun_lshape,
							e_mesh_aqtun_full,
						};

						s_v3 size = v3(1);
						s_v3 pos = v3(x, 0, -y);
						pos.z += 0.5f;
						pos.x -= 0.5f;

						s_tile_visual tile = get_tile_index(&game->map, x, y);

						if(tile.tile_index != 0xFF) {
							e_mesh mesh_id = mesh_arr[tile.tile_index];
							float rotation = c_tile_rotations[tile.rotation_index];
							s_m4 model = m4_translate(pos);
							model = m4_multiply(model, m4_rotate(rotation, c_y_axis));
							model = m4_multiply(model, m4_scale(size));
							draw_mesh(mesh_id, model, make_rrr(1), e_shader_mesh, 0);
						}
					}
					if(!anything_active) {
						break;
					}
				}

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw wall start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				for(int i = c_first_index[e_entity_wall]; i < c_last_index_plus_one[e_entity_wall]; i += 1) {
					if(!entity_arr->active[i]) {
						continue;
					}
					s_entity entity = entity_arr->data[i];
					if(entity.is_fence) {
						s_v3 size = v3(1, 1, 1);
						s_v4 color = make_rrr(1);
						color = make_rgb(1, 0.8f, 0.8f);
						s_v3 pos = entity.pos;
						draw_mesh(e_mesh_aqtun_spikes, pos, size, color, e_shader_mesh, 0);
					}
				}
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw wall end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw pickups start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				for(int i = c_first_index[e_entity_pickup]; i < c_last_index_plus_one[e_entity_pickup]; i += 1) {
					if(!entity_arr->active[i]) {
						continue;
					}
					s_entity entity = entity_arr->data[i];
					static float animation_time[g_entity_type_data[e_entity_pickup].max_count] = zero;
					switch(entity.pickup_type) {
						xcase e_pickup_end: {
							s_v3 pos = entity.pos;
							s_v2 size = v2(1);
							pos.y += size.y * 0.5f;

							s_v2i frame_arr[] = {
								v2i(0, 3), v2i(1, 3), v2i(2, 3)
							};
							animation_time[i] += delta * 10;
							s_v2i frame = frame_arr[floorfi(animation_time[i]) % array_count(frame_arr)];
							draw_billboard_ex(game->atlas2, pos, size, frame, make_rrr(1), -game->render_time * 2.5f, zero, 0);

							{
								s_entity emitter = make_end_particles(pos);
								add_emitter(emitter);
							}

						}
						xcase e_pickup_spike: {
							s_v3 pos = lerp_v3(entity.prev_pos, entity.pos, interp_dt);
							pos.y += 0.25f;
							float rotation = game->render_time;
							if(entity.dir.x > 0) {
								rotation = -game->render_time;
							}
							draw_billboard_ex(game->atlas, pos, v2(0.5f), v2i(5, 0), make_rrr(1), rotation * 4, zero, 0);
						}
						xcase e_pickup_teleport: {
							s_v3 pos = entity.pos;
							s_v2 size = v2(0.75f);
							pos.y += size.y * 0.5f;

							s_v2i frame_arr[] = {
								v2i(0, 2), v2i(1, 2), v2i(2, 2)
							};
							animation_time[i] += delta * 10;
							s_v2i frame = frame_arr[floorfi(animation_time[i]) % array_count(frame_arr)];

							draw_billboard_ex(game->atlas2, pos, size, frame, make_rrr(1), 0, zero, 0);

							{
								s_entity emitter = make_teleporter_particles(pos);
								add_emitter(emitter);
							}
						}
					}
				}
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw pickups end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
					if(!entity_arr->active[i]) {
						continue;
					}
					s_entity* entity = &entity_arr->data[i];
					s_v3 pos = lerp_v3(entity->prev_pos, entity->pos, interp_dt);
					pos.y += c_enemy_size.y * 0.5f;
					s_v4 color = make_rrr(0.7f);
					if(v3_distance(player.pos, entity->pos) <= c_player_range) {
						color = make_rrr(1.5f);
					}

					s_v2i frame = get_enemy_animation_frame(entity->animation_time);
					entity->animation_time += delta * 10;
					draw_billboard_ex(game->atlas2, pos, c_enemy_size, frame, color, c_pi, zero, 0);
				}
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				{
					s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
					data.fbo = game->game_fbo;
					data.view = view_3d;
					data.projection = perspective;
					data.blend_mode = e_blend_mode_disabled;
					data.depth_mode = e_depth_mode_read_and_write;
					render_flush(data, true, 0);
				}

				// @Note(tkap, 19/04/2026): Sword trail thing
				{
					s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
					data.fbo = game->game_fbo;
					data.view = view_3d;
					data.projection = perspective;
					data.blend_mode = e_blend_mode_normal;
					data.depth_mode = e_depth_mode_read_no_write;
					render_flush(data, true, 1);
				}

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw dying enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				for(int i = c_first_index[e_entity_dying_enemy]; i < c_last_index_plus_one[e_entity_dying_enemy]; i += 1) {
					if(!entity_arr->active[i]) {
						continue;
					}
					s_entity* entity = &entity_arr->data[i];
					s_v2 size = c_enemy_size * v2(1.0f, 0.5f);
					s_v3 pos = lerp_v3(entity->prev_pos, entity->pos, interp_dt);
					pos.y += c_enemy_size.y * 0.5f;
					s_v4 color = make_rrr(0.7f);

					b8 can_fall = true;
					if(pos.y <= 0.0f) {
						pos.y = 0;
						can_fall = false;
					}

					b8 should_remove = false;
					{
						s_time_data data = get_time_data(game->render_time, entity->spawn_timestamp, 10.0f);
						color.a = data.inv_percent;
						s_rng rng = make_rng(entity->seed);
						if(can_fall) {
							entity->rotation += randf_range(&rng, 0.1f, 2.0f) * rand_minus_1_or_1(&rng) * delta;
						}
						if(data.percent >= 1) {
							should_remove = true;
						}
					}

					{
						s_instance_data data = zero;
						data.model = m4_translate(pos);
						data.model = m4_multiply(data.model, m4_rotate(entity->rotation, c_z_axis));
						data.model = m4_multiply(data.model, m4_scale(v3(entity->size, 1)));
						data.color = color;
						data.uv_min = entity->uv_min;
						data.uv_max = entity->uv_max;
						add_to_render_group(data, e_shader_billboard, e_texture_atlas2, e_mesh_quad, 0);
					}

					if(can_fall) {
						entity->pos += entity->dir * delta;
						entity->dir.y -= entity->gravity* delta;
					}

					if(should_remove) {
						entity_manager_remove(entity_arr, e_entity_dying_enemy, i);
					}
				}
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw dying enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				{
					s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
					data.fbo = game->game_fbo;
					data.view = view_3d;
					data.projection = perspective;
					data.blend_mode = e_blend_mode_normal;
					data.depth_mode = e_depth_mode_read_no_write;
					render_flush(data, true, 0);
				}

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw fail action start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				{
					if(soft_data->fail_action_effect.active) {
						s_time_data data = get_time_data(game->render_time, soft_data->fail_action_effect.timestamp, soft_data->fail_action_effect.duration);
						draw_rect_ex(c_world_center, c_world_size, make_rgba(0.5f, 0.0f, 0.0f, data.inv_percent), e_shader_fail_action, 0);
						if(data.percent >= 1) {
							soft_data->fail_action_effect = zero;
						}
					}

					{
						s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
						data.fbo = game->game_fbo;
						data.projection = ortho;
						data.blend_mode = e_blend_mode_normal;
						data.depth_mode = e_depth_mode_no_read_no_write;
						render_flush(data, true, 0);
					}
				}
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw fail action end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			}

			{
				update_particles(delta, true, 0);
				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.fbo = game->game_fbo;
				data.view = view_3d;
				data.projection = perspective;
				data.blend_mode = e_blend_mode_additive;
				data.depth_mode = e_depth_mode_read_no_write;
				render_flush(data, true, 0);
			}

			{
				float transition_time = get_transition_percent(game->render_time, hard_data->map_win_transition);
				e_shader shader = e_shader_flat;
				if(hard_data->map_win_transition.active) {
					shader = e_shader_transition;
				}
				draw_texture_screen(c_world_center, c_world_size, make_rrr(1), e_texture_game_fbo, shader, v2(0, 1), v2(1, 0), zero, 0);
				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.transition_time = transition_time;
				data.projection = ortho;
				data.blend_mode = e_blend_mode_disabled;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true, 0);
			}

			if(!will_win_soon() && soft_data->draw_signal) {
				draw_text(S("NOW"), wxy(0.5f, 0.1f), 64, make_rrr(1), true, &game->font, zero, 0);
				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true, 0);
			}
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		lights start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		#if 0
		if(!game->disable_lights) {
			{
				clear_framebuffer_color(game->light_fbo.id, make_rrr(0.66f));
				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.projection = view_projection;
				data.blend_mode = e_blend_mode_additive;
				data.depth_mode = e_depth_mode_no_read_no_write;
				data.fbo = game->light_fbo;
				render_flush(data, true, 3);
			}

			{
				draw_texture_screen(c_world_center, c_world_size, make_rrr(1), e_texture_light, e_shader_flat, v2(0, 1), v2(1, 0), zero, 0);
				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_multiply;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true, 0);
			}
		}
		#endif
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		lights end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw fct start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			for(int fct_i = c_first_index[e_entity_fct]; fct_i < c_last_index_plus_one[e_entity_fct]; fct_i += 1) {
				if(!entity_arr->active[fct_i]) {
					continue;
				}
				s_entity* fct = &entity_arr->data[fct_i];
				float passed = game->render_time - fct->spawn_timestamp;
				float alpha = ease_in_expo_advanced(passed, 0, 1, 1, 0);
				draw_text(fct->text, fct->pos.xy, 24, make_ra(1, alpha), true, &game->font, zero, 0);
				fct->pos.y -= delta * 60;
				if(passed >= 1) {
					entity_manager_remove(entity_arr, e_entity_fct, fct_i);
				}
			}
			do_basic_render_flush(view_projection, 0, view_inv);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw fct end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		if(do_game_ui) {

			s_rect rect = {
				4, 4, 4, 4
			};
			s_v2 button_size = v2(1, 44);
			s_container container = make_down_center_x_container(rect, button_size, 10);

			{
				s_time_format data = update_count_to_time_format(game->hard_data.update_count, false);
				s_len_str text = format_text("%02i:%02i.%03i", data.minutes, data.seconds, data.milliseconds);
				if(!game->hide_timer) {
					draw_text(text, container_get_pos_and_advance(&container), 48, make_rrr(1), false, &game->font, zero, 0);
				}
			}

			{
				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true, 0);
			}

			// {
			// 	s_render_flush_data data = make_render_flush_data(zero, zero);
			// 	data.projection = ortho;
			// 	data.blend_mode = e_blend_mode_normal;
			// 	data.depth_mode = e_depth_mode_read_and_write;
			// 	render_flush(data, true, 0);
			// }
		}

		if(game->tooltip.count > 0) {
			float font_size = 32;
			s_v2 size = get_text_size(game->tooltip, &game->font, font_size);
			s_v2 rect_pos = zero;
			rect_pos -= v2(8);
			size += v2(16);
			rect_pos = topleft_to_bottomleft_mouse(rect_pos, size, g_mouse);
			rect_pos = prevent_offscreen(rect_pos, size);
			s_v2 text_pos = rect_pos + v2(8);
			draw_rect_topleft(rect_pos, size, make_ra(0.1f, 0.95f), 0);
			draw_text(game->tooltip, text_pos, font_size, make_rrr(1), false, &game->font, zero, 0);
			{
				s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true, 0);
			}
		}
		game->tooltip = zero;

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		cheat menu start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		#if defined(m_debug)
		if(game->cheat_menu_enabled) {
			s_rect rect = {
				10, 10, 350, c_world_size.y
			};
			s_v2 size = v2(320, 48);
			s_container container = make_down_center_x_container(rect, size, 10);

			do_basic_render_flush(ortho, 0, view_inv);
		}
		#endif
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		cheat menu end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	}

	if(do_defeat) {

		draw_rect_topleft(v2(0), c_world_size, make_ra(0, 0.75f), 0);
		draw_text(S("Defeat"), wxy(0.5f, 0.4f) + rand_v2_11(&game->rng) * 8, 128, make_rgb(1, 0.2f, 0.2f), true, &game->font, zero, 0);
		draw_text(S("Press R to try again"), wxy(0.5f, 0.55f), sin_range(48, 64, game->render_time * 8), make_rrr(1), true, &game->font, zero, 0);

		{
			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true, 0);
		}
	}

	s_state_transition transition = get_state_transition(&game->state0, game->render_time);
	if(transition.active) {
		{
			float alpha = 0;
			if(transition.time_data.percent <= 0.5f) {
				alpha = transition.time_data.percent * 2;
			}
			else {
				alpha = transition.time_data.inv_percent * 2;
			}
			s_instance_data data = zero;
			data.model = fullscreen_m4();
			data.color = make_rgba(0.0f, 0, 0, alpha);
			add_to_render_group(data, e_shader_flat, e_texture_white, e_mesh_quad, 0);
		}

		{
			s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true, 0);
		}
	}

	// {
	// 	do_lerpable_snap(&game->music_speed, delta * 2.0f, 0.01f);
	// 	s_active_sound* music = find_playing_sound(e_sound_music);
	// 	if(music) {
	// 		music->data.speed = game->music_speed.curr;
	// 	}
	// }

	SDL_GL_SwapWindow(g_platform_data->window);

	game->render_time += delta;
}

func e_button_result do_button(s_len_str text, s_v2 pos, b8 centered)
{
	s_v2 size = v2(256, 48);
	e_button_result result = do_button_ex(text, pos, size, centered, zero);
	return result;
}

func e_button_result do_button_ex(s_len_str text, s_v2 pos, s_v2 size, b8 centered, s_button_data optional)
{
	e_button_result result = e_button_result_none;
	if(!centered) {
		pos += size * 0.5f;
	}

	b8 do_tooltip = false;
	b8 hovered = mouse_vs_rect_center(g_mouse, pos, size);
	s_v4 color = optional.button_color;
	s_v4 text_color = make_rrr(1);
	if(optional.disabled) {
		color = multiply_rgb(color, 0.6f);
		text_color = make_rrr(0.7f);
	}
	if(hovered) {
		do_tooltip = true;
		if(!optional.disabled) {
			size += v2(8);
			if(!centered) {
				pos -= v2(4) * 0.5f;
			}
			color = multiply_rgb(color, 2);
			if(g_left_click) {
				result = e_button_result_left_click;
				if(!optional.mute_click_sound) {
					play_sound(e_sound_click);
				}
			}
		}
		if(g_right_click) {
			result = e_button_result_right_click;
		}
	}

	{
		s_instance_data data = zero;
		data.model = m4_translate(v3(pos, 0));
		data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
		data.color = color;
		add_to_render_group(data, e_shader_button, e_texture_white, e_mesh_quad, optional.render_pass_index);
	}

	draw_text(text, pos, optional.font_size, text_color, true, &game->font, {.z = 1}, optional.render_pass_index);

	if(do_tooltip && optional.tooltip.count > 0) {
		game->tooltip = optional.tooltip;
	}

	return result;
}

func b8 do_bool_button(s_len_str text, s_v2 pos, b8 centered, b8* out)
{
	s_v2 size = v2(256, 48);
	b8 result = do_bool_button_ex(text, pos, size, centered, out);
	return result;
}

func b8 do_bool_button_ex(s_len_str text, s_v2 pos, s_v2 size, b8 centered, b8* out)
{
	assert(out);
	b8 result = false;
	if(do_button_ex(text, pos, size, centered, zero) == e_button_result_left_click) {
		result = true;
		*out = !(*out);
	}
	return result;
}

template <int n>
func void cstr_into_builder(s_str_builder<n>* builder, char* str)
{
	assert(str);

	int len = (int)strlen(str);
	assert(len <= n);
	memcpy(builder->str, str, len);
	builder->count = len;
}

func void do_leaderboard()
{
	b8 escape = is_key_pressed(SDLK_ESCAPE, true);
	if(do_button(S("Back"), wxy(0.87f, 0.92f), true) == e_button_result_left_click || escape) {
		s_maybe<int> prev = get_previous_non_temporary_state(&game->state0);
		if(prev.valid && prev.value == e_game_state0_pause) {
			pop_state_transition(&game->state0, game->render_time, c_transition_time);
		}
		else {
			add_state_transition(&game->state0, e_game_state0_main_menu, game->render_time, c_transition_time);
			clear_state(&game->state0);
		}
	}

	{
		if(!game->leaderboard_received) {
			draw_text(S("Getting leaderboard..."), c_world_center, 48, make_rrr(0.66f), true, &game->font, zero, 0);
		}
		else if(game->leaderboard_arr.count <= 0) {
			draw_text(S("No scores yet :("), c_world_center, 48, make_rrr(0.66f), true, &game->font, zero, 0);
		}

		s_v2 pos = c_world_center * v2(1.0f, 0.7f);
		int num_pages = ceilfi(game->leaderboard_arr.count / (float)c_leaderboard_visible_entries_per_page);
		b8 is_last_page = game->curr_leaderboard_page >= num_pages - 1;
		int start = (c_leaderboard_visible_entries_per_page * game->curr_leaderboard_page);
		int end = start + c_leaderboard_visible_entries_per_page;
		if(end > game->leaderboard_arr.count) {
			end = game->leaderboard_arr.count;
		}
		for(int entry_i = start; entry_i < end; entry_i += 1) {
			s_leaderboard_entry entry = game->leaderboard_arr[entry_i];
			s_time_format data = update_count_to_time_format(entry.time, false);
			s_v4 color = make_rrr(0.8f);
			if(builder_equals(&game->leaderboard_public_uid, &entry.internal_name)) {
				color = hex_to_rgb(0xD3A861);
			}
			char* name = entry.internal_name.str;
			if(entry.nice_name.count > 0) {
				name = entry.nice_name.str;
			}
			constexpr float font_size = 28;
			draw_text(format_text("%2i %s", entry_i + 1, name), v2(c_world_size.x * 0.1f, pos.y - font_size * 0.75f), font_size, color, false, &game->font, zero, 0);
			s_len_str text = format_text("%02i:%02i.%03i", data.minutes, data.seconds, data.milliseconds);
			draw_text(text, v2(c_world_size.x * 0.5f, pos.y - font_size * 0.75f), font_size, color, false, &game->font, zero, 0);
			pos.y += font_size * 1.5f;
		}
		{
			s_v2 button_size = v2(256, 48);
			{
				s_button_data optional = zero;
				if(game->curr_leaderboard_page <= 0) {
					optional.disabled = true;
				}
				if(do_button_ex(S("Previous"), wxy(0.25f, 0.92f), button_size, true, optional) == e_button_result_left_click) {
					game->curr_leaderboard_page -= 1;
				}
			}
			{
				s_button_data optional = zero;
				if(is_last_page) {
					optional.disabled = true;
				}
				if(do_button_ex(S("Next"), wxy(0.45f, 0.92f), button_size, true, optional) == e_button_result_left_click) {
					game->curr_leaderboard_page += 1;
				}
			}
		}
	}
}

func b8 is_valid_2d_index(s_v2i index, int x_count, int y_count)
{
	b8 result = index.x >= 0 && index.x < x_count && index.y >= 0 && index.y < y_count;
	return result;
}

func b8 check_action(float curr_time, float timestamp, float grace)
{
	float passed = curr_time - timestamp;
	b8 result = passed <= grace && timestamp > 0;
	return result;
}

func b8 check_action_maybe(float curr_time, s_maybe<float> timestamp, float grace)
{
	b8 result = false;
	if(timestamp.valid) {
		result = check_action(curr_time, timestamp.value, grace);
	}
	return result;
}

func void do_screen_shake(float intensity)
{
	s_soft_game_data* soft_data = &game->soft_data;
	soft_data->start_screen_shake_timestamp = game->render_time;
	soft_data->shake_intensity = intensity;
}

func s_particle_emitter_a make_emitter_a()
{
	s_particle_emitter_a result = zero;
	result.particle_duration = 1;
	result.radius = 16;
	result.speed = 128;
	{
		s_particle_color color = zero;
		color.color = make_rrr(1);
		result.color_arr.add(color);
	}
	{
		s_particle_color color = zero;
		color.color = make_rrr(0.0f);
		color.percent = 1;
		result.color_arr.add(color);
	}
	return result;
}

func s_particle_emitter_b make_emitter_b()
{
	s_particle_emitter_b result = zero;
	result.particles_per_second = 1;
	result.particle_count = 1;
	return result;
}

func int add_emitter(s_entity emitter)
{
	s_soft_game_data* soft_data = &game->soft_data;
	emitter.emitter_b.creation_timestamp = game->render_time;
	emitter.emitter_b.last_emit_timestamp = game->render_time - 1.0f / emitter.emitter_b.particles_per_second;
	int index = entity_manager_add_if_not_full(&soft_data->entity_arr, e_entity_emitter, emitter);
	return index;
}

func void draw_keycap(char c, s_v2 pos, s_v2 size, float alpha, int render_pass_index)
{
	pos += size * 0.5f;
	s_v2 keycap_size = size * 1.1f;
	draw_atlas_ex(game->atlas, pos, keycap_size, v2i(2, 0), make_ra(1, alpha), 0, zero, render_pass_index);
	s_len_str str = format_text("%c", to_upper_case(c));
	pos.x -= keycap_size.x * 0.025f;
	pos.y -= keycap_size.x * 0.05f;
	s_v4 color = set_alpha(c_key_color, alpha);
	draw_text(str, pos, size.x, color, true, &game->font, zero, render_pass_index + 1);
}

// func void add_multiplicative_light(s_v2 pos, float radius, s_v4 color, float smoothness)
// {
// 	s_light light = zero;
// 	light.pos = pos;
// 	light.radius = radius;
// 	light.color = color;
// 	light.smoothness = smoothness;
// 	if(!game->multiplicative_light_arr.is_full()) {
// 		game->multiplicative_light_arr.add(light);
// 	}
// }

// func void add_additive_light(s_v2 pos, float radius, s_v4 color, float smoothness)
// {
// 	s_light light = zero;
// 	light.pos = pos;
// 	light.radius = radius;
// 	light.color = color;
// 	light.smoothness = smoothness;
// 	if(!game->additive_light_arr.is_full()) {
// 		game->additive_light_arr.add(light);
// 	}
// }

func s_entity make_entity()
{
	s_entity entity = zero;
	game->next_entity_id += 1;
	entity.id = game->next_entity_id;
	return entity;
}

func s_entity* get_entity(s_entity_ref ref)
{
	s_entity* result = null;
	if(ref.id > 0) {
		if(ref.index >= 0 && ref.index < c_max_entities) {
			if(game->soft_data.entity_arr.active[ref.index]) {
				if(game->soft_data.entity_arr.data[ref.index].id == ref.id) {
					result = &game->soft_data.entity_arr.data[ref.index];
				}
			}
		}
	}
	return result;
}

func float get_wanted_game_speed(float interp_dt)
{
	(void)interp_dt;
	float result = 1;
	// if(game->soft_data.hit_enemy_while_dashing_timestamp > 0) {
	// 	s_time_data time_data = get_time_data(update_time_to_render_time(game->update_time, interp_dt), game->soft_data.hit_enemy_while_dashing_timestamp, 0.04f);
	// 	time_data.percent = at_most(1.0f, time_data.percent);
	// 	result = ease_out_expo_advanced(time_data.percent, 0.75f, 1, 0.1f, 1);
	// 	if(time_data.percent >= 1) {
	// 		game->soft_data.hit_enemy_while_dashing_timestamp = 0;
	// 	}
	// }
	return result;
}

func s_rect get_camera_bounds(s_m4 view_inv)
{
	s_v2 topleft = v2(0, 0);
	s_v2 bottomright = v2(c_world_size.x, c_world_size.y);
	s_rect result;
	result.pos = v2_multiply_m4(topleft, view_inv);
	result.size = v2_multiply_m4(bottomright, view_inv) - result.pos;
	return result;
}

func s_v2i tile_index_from_pos(s_v2 pos)
{
	s_v2i result = v2i(
		floorfi(pos.x / c_tile_size),
		floorfi(pos.y / c_tile_size)
	);
	return result;
}

template <int n>
func void builder_add_separator(s_str_builder<n>* builder)
{
	builder_add(builder, "$$888888--------------------$.\n");
}

func void add_non_spammy_message(s_v2 pos, s_len_str text)
{
	float passed = game->render_time - game->soft_data.last_non_spammy_timestamp;
	if(passed >= 0.5f) {
		game->soft_data.last_non_spammy_timestamp = game->render_time;
		s_entity fct = zero;
		fct.prev_pos.xy = pos;
		fct.pos.xy = pos;
		fct.spawn_timestamp = game->render_time;
		fct.text = text;
		entity_manager_add_if_not_full(&game->soft_data.entity_arr, e_entity_fct, fct);
	}
}

func void teleport_entity(s_entity* entity, s_v3 pos)
{
	entity->pos = pos;
	entity->prev_pos = pos;
}

func void set_editor_entity(s_v2i index, e_entity type, int sub_type)
{
	s_editor* editor = &game->editor;
	s_editor_entity entity = zero;
	entity.type = type;
	entity.sub_type = sub_type;
	editor->map.entity_arr[index.y][index.x] = entity;
	editor->map.active[index.y][index.x] = true;
}

func void load_map(int index)
{
	s_len_str path = format_text("assets/%i.map", index);
	char* temp_path = to_cstr(path, &game->update_frame_arena);
	u8* data = read_file(temp_path, &game->update_frame_arena, null);
	if(!data) {
		return;
	}

	game->hard_data.current_map = index;


	for_enum(type_i, e_entity) {
		if(type_i != e_entity_player) {
			entity_manager_reset(&game->soft_data.entity_arr, type_i);
		}
	}

	s_map* map = (s_map*)data;
	assert(map->version == 1);

	game->map = *map;

	#if defined(m_debug)
	game->editor.map = *map;
	#endif

	for(int y = 0; y < c_map_height; y += 1) {
		for(int x = 0; x < c_map_width; x += 1) {
			if(map->active[y][x]) {
				s_editor_entity editor_entity = map->entity_arr[y][x];
				s_entity entity = zero;
				if(editor_entity.type == e_entity_pickup) {
					entity.pickup_type = (e_pickup)editor_entity.sub_type;
				}
				else if(editor_entity.type == e_entity_wall) {
					entity.is_fence = editor_entity.sub_type == 1;
				}
				s_v3 pos = v3(x, 0.0f, -y);
				entity.target_pos = pos;
				teleport_entity(&entity, pos);
				entity_manager_add(&game->soft_data.entity_arr, editor_entity.type, entity);
			}
		}
	}

	reset_player_pos(get_player());
}

func void save_map(int index)
{
	s_len_str path = format_text("assets/%i.map", index);
	char* temp_path = to_cstr(path, &game->update_frame_arena);
	game->editor.map.version = c_map_version;
	write_file(temp_path, &game->editor.map, sizeof(game->editor.map));
}

func char* to_cstr(s_len_str str, s_linear_arena* arena)
{
	char* result = (char*)arena_alloc(arena, str.count + 1);
	memcpy(result, str.str, str.count);
	result[str.count] = 0;
	return result;
}

func void draw_entity_2d(s_editor_entity entity, int x, int y)
{
	s_v2 base_pos = v2(x, y) * c_tile_size;
	switch(entity.type) {
		xcase e_entity_wall: {
			if(entity.sub_type == 0) {
				draw_atlas_topleft(game->atlas, base_pos, c_tile_size_v, v2i(3, 0), make_rrr(1), 0);
			}
			else {
				draw_atlas_topleft(game->atlas, base_pos, c_tile_size_v, v2i(4, 0), make_rrr(1), 0);
			}
		}
		xcase e_entity_pickup: {
			s_v2 pos = base_pos;
			switch(entity.sub_type) {
				xcase e_pickup_end: {
					draw_atlas_topleft(game->atlas, pos, c_tile_size_v, v2i(2, 0), make_rrr(1), 0);
				}
				xcase e_pickup_spike: {
					draw_atlas_topleft(game->atlas, pos, c_tile_size_v, v2i(5, 0), make_rrr(1), 0);
				}
				xcase e_pickup_teleport: {
					draw_atlas_topleft(game->atlas, pos, c_tile_size_v, v2i(7, 0), make_rrr(1), 0);
				}
			}
		}
		xcase e_entity_enemy: {
			s_v2 pos = base_pos;
			draw_atlas_topleft(game->atlas, pos, c_tile_size_v, v2i(6, 0), make_rrr(1), 0);
		}
	}
}

func s_v2i tile_index_from_3d(s_v3 pos)
{
	s_v2i result = v2i(floorfi(pos.x), floorfi(pos.z));
	return result;
}

func int tile_index_1d_from_3d(s_v3 pos)
{
	s_v2i temp = v2i(floorfi(pos.x), floorfi(pos.z));
	int result = tile_index_to_1d(temp);
	return result;
}

func int tile_index_to_1d(s_v2i index)
{
	int result = index.x + index.y * c_map_width;
	return result;
}

func s_entity* get_end()
{
	s_entity* result = null;
	for(int i = c_first_index[e_entity_pickup]; i < c_last_index_plus_one[e_entity_pickup]; i += 1) {
		if(!game->soft_data.entity_arr.active[i]) {
			continue;
		}
		s_entity* entity = &game->soft_data.entity_arr.data[i];
		if(entity->pickup_type == e_pickup_end) {
			result = entity;
			break;
		}
	}
	return result;
}

func void reset_player_pos(s_entity* player)
{
	s_v3 target_pos = v3(c_map_width / 2, 0, 0);
	player->target_pos = target_pos;
	teleport_entity(player, target_pos);
}

func s_entity* get_player()
{
	s_entity* result = &game->soft_data.entity_arr.data[c_first_index[e_entity_player]];
	return result;
}

func s_entity* get_wall_at_tile(s_v2i index)
{
	s_entity* result = null;
	for(int i = c_first_index[e_entity_wall]; i < c_last_index_plus_one[e_entity_wall]; i += 1) {
		if(!game->soft_data.entity_arr.active[i]) {
			continue;
		}
		s_entity* entity = &game->soft_data.entity_arr.data[i];
		s_v2i temp = tile_index_from_3d(entity->pos);
		if(index == temp) {
			result = entity;
			break;
		}
	}
	return result;
}

func s_entity* get_spike_at_tile(s_v2i index)
{
	s_entity* result = get_pickup_at_tile(index, e_pickup_spike);
	return result;
}

func s_entity* get_teleport_at_tile(s_v2i index)
{
	s_entity* result = get_pickup_at_tile(index, e_pickup_teleport);
	return result;
}

func s_entity* get_pickup_at_tile(s_v2i index, e_pickup pickup_type)
{
	s_entity* result = null;
	for(int i = c_first_index[e_entity_pickup]; i < c_last_index_plus_one[e_entity_pickup]; i += 1) {
		if(!game->soft_data.entity_arr.active[i]) {
			continue;
		}
		s_entity* entity = &game->soft_data.entity_arr.data[i];
		if(entity->pickup_type == pickup_type) {
			s_v2i temp = tile_index_from_3d(entity->target_pos);
			if(index == temp) {
				result = entity;
				break;
			}
		}
	}
	return result;
}

func void move_forward(s_entity* player, b8 does_walking_into_wall_kill_you)
{
	s_entity* wall = null;
	{
		s_v2i player_tile = tile_index_from_3d(player->target_pos - v3(0, 0, 1));
		wall = get_wall_at_tile(player_tile);
	}

	b8 is_fence = false;
	if(wall) {
		is_fence = wall->is_fence;
	}
	else {
		s_v3 target_pos = player->target_pos;
		target_pos.z -= 1;
		player->target_pos = target_pos;
	}

	if((does_walking_into_wall_kill_you || is_fence) && wall) {
		start_losing(c_action_interval * 1.1f);
	}
}

func void jump_forward(s_entity* player)
{
	s_v3 target_pos = player->target_pos;
	for(int i = 0; i < 2; i += 1) {
		target_pos.z -= 1;
		s_entity* wall = null;
		{
			s_v2i player_tile = tile_index_from_3d(target_pos);
			wall = get_wall_at_tile(player_tile);
		}

		b8 can_jump_over = true;
		if(wall) {
			if(wall->is_fence) {
				can_jump_over = i == 0;
			}
			else {
				can_jump_over = false;
			}
		}

		if(can_jump_over) {
			player->target_pos = target_pos;
		}
		else {
			start_losing(c_action_interval * 1.1f);
			break;
		}
	}
}

func b8 check_for_win(s_v3 pos)
{
	b8 result = false;
	int player_index = tile_index_1d_from_3d(pos);
	s_entity* end = get_end();
	if(end) {
		int end_index = tile_index_1d_from_3d(end->pos);
		if(player_index == end_index) {
			result = true;
		}
	}
	return result;
}

func b8 will_win_soon()
{
	b8 result = game->soft_data.win_timestamp.valid;
	return result;
}

func b8 will_lose_soon()
{
	b8 result = game->soft_data.lose_timestamp.valid;
	return result;
}

func void start_losing(float delay)
{
	if(!will_lose_soon() && !will_win_soon()) {
		float soft_update_time = game->soft_data.update_count * (float)c_update_delay;
		game->soft_data.lose_timestamp = maybe(soft_update_time);
		game->soft_data.lose_delay = delay;
		play_sound_at_speed(e_sound_death, get_rand_sound_speed(1.1f, &game->rng));

		{
			s_entity* player = get_player();
			s_v3 pos = player->target_pos;
			pos.y += c_player_size.y * 0.5f;
			s_entity emitter = make_player_death_particles(pos);
			add_emitter(emitter);
		}
	}
}

func void kill_enemy(int index)
{
	auto entity_arr = &game->soft_data.entity_arr;
	entity_arr->active[index] = false;
	s_entity enemy = entity_arr->data[index];

	{
		s_v3 pos = enemy.pos;
		s_entity emitter = make_enemy_death_particles(pos);
		add_emitter(emitter);
	}

	float cut_y = randf_range(&game->rng, 0.3f, 0.7f);
	for(int i = 0; i < 2; i += 1) {
		s_entity entity = zero;
		s_v3 pos = enemy.pos;
		if(i == 0) {
			float top_of_enemy = pos.y + c_enemy_size.y * 0.5f;
			float my_size = c_enemy_size.y * 0.5f * cut_y;
			pos.y = top_of_enemy - my_size;
			entity.size = v2(c_enemy_size.x, c_enemy_size.y * cut_y);
		}
		else {
			float bottom_of_enemy = pos.y - c_enemy_size.y * 0.5f;
			pos.y = bottom_of_enemy + c_enemy_size.y * 0.5f * (1.0f - cut_y);
			entity.size = v2(c_enemy_size.x, c_enemy_size.y * (1.0f - cut_y));
		}
		entity.pos = pos;
		entity.prev_pos = pos;
		entity.dir.z = randf_range(&game->rng, -3, 0);
		{
			float r = randf_range(&game->rng, 0.5f, 2.5f) * rand_minus_1_or_1(&game->rng);
			entity.dir.x = r;
		}
		{
			float r = randf_range(&game->rng, 0.5f, 2.5f);
			entity.dir.y = i == 0 ? r : r * 0.5f;
		}
		entity.gravity = 6;
		entity.spawn_timestamp = game->render_time;
		entity.seed = randu64(&game->rng);

		s_v2i animation_frame = get_enemy_animation_frame(enemy.animation_time);
		entity.animation_frame = animation_frame;

		float x = (float)(game->atlas2.sprite_size.x * animation_frame.x);
		float y = (float)(game->atlas2.sprite_size.y * animation_frame.y);
		float sx = (float)game->atlas2.sprite_size.x;
		float sy = (float)game->atlas2.sprite_size.y;

		float top = y / game->atlas2.texture_size.y;
		float top_cut = (y + sy * cut_y) / game->atlas2.texture_size.y;
		float bottom = (y + sy) / game->atlas2.texture_size.y;

		// @Note(tkap, 19/04/2026): uv.x has to be flipped, idk why. maybe something to do with billboard shader??
		if(i == 0) {
			entity.uv_min = v2(
				(x + sx) / game->atlas2.texture_size.x,
				top
			);
			entity.uv_max = v2(
				x / game->atlas2.texture_size.x,
				top_cut
			);
		}
		else {
			entity.uv_min = v2(
				(x + sx) / game->atlas2.texture_size.x,
				top_cut
			);
			entity.uv_max = v2(
				x / game->atlas2.texture_size.x,
				bottom
			);
		}
		swap(&entity.uv_min.y, &entity.uv_max.y);
		entity_manager_add(entity_arr, e_entity_dying_enemy, entity);
	}
}

func s_maybe<int> get_closest_enemy_in_attack_range(s_v3 pos)
{
	s_maybe<int> result = zero;
	float smallest_dist = 9999999;
	for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
		if(!game->soft_data.entity_arr.active[i]) {
			continue;
		}
		s_entity entity = game->soft_data.entity_arr.data[i];
		float dist = v3_distance(entity.pos, pos);
		if(dist < smallest_dist && dist < c_player_range) {
			smallest_dist = dist;
			result = maybe(i);
		}
	}
	return result;
}

func s_entity* find_closest_teleporter_to(s_entity* other)
{
	float soft_update_time = game->soft_data.update_count * (float)c_update_delay;
	s_entity* result = null;
	float smallest_dist = 9999999;
	for(int i = c_first_index[e_entity_pickup]; i < c_last_index_plus_one[e_entity_pickup]; i += 1) {
		if(!game->soft_data.entity_arr.active[i]) {
			continue;
		}
		s_entity* entity = &game->soft_data.entity_arr.data[i];
		b8 on_cooldown = check_action_maybe(soft_update_time, entity->last_teleport_timestamp, c_teleport_cooldown);
		if(!on_cooldown && entity != other && entity->pickup_type == e_pickup_teleport) {
			float dist = v3_distance(entity->pos, other->pos);
			if(dist < smallest_dist) {
				smallest_dist = dist;
				result = entity;
			}
		}
	}
	return result;
}

func b8 will_teleport_soon()
{
	b8 result = game->soft_data.teleport_timestamp.valid;
	return result;
}

func float get_transition_percent(float time_now, s_transition t)
{
	float result = 0;
	if(t.active) {
		result = (time_now - t.timestamp) / t.duration;
		result = at_most(1.0f, result);
	}
	return result;
}

func s_entity make_teleporter_particles(s_v3 pos)
{
	s_entity emitter = make_end_particles(pos);
	emitter.emitter_a.speed *= 1.2f;
	emitter.emitter_a.color_arr[0].color = hex_to_rgb(0xa23e8c);
	emitter.emitter_b.spawn_data.x *= 0.5f;
	return emitter;
}

func s_entity make_end_particles(s_v3 pos)
{
	s_particle_emitter_a a = make_emitter_a();
	a.radius = 0.02f;
	a.speed = 0.25f;
	a.particle_duration = 1.0f;
	a.radius_rand = 0.5f;
	a.speed_rand = 0.5f;
	a.particle_duration_rand = 0.5f;
	a.dir = v3(1, 1, 1);
	a.dir_rand = v3(1, 1, 1);
	a.color_arr[0].color = make_rgb(0.815f * 0.25f, 0.854f, 0.568f * 0.25f);
	a.pos = pos;
	s_particle_emitter_b b = make_emitter_b();
	b.spawn_type = e_emitter_spawn_type_circle;
	b.spawn_data.x = 0.5f;
	b.particle_count = 10;

	s_entity emitter = zero;
	emitter.emitter_a = a;
	emitter.emitter_b = b;
	return emitter;
}

func s_entity make_enemy_death_particles(s_v3 pos)
{
	s_particle_emitter_a a = make_emitter_a();
	a.radius = 0.03f;
	a.speed = 1.0f;
	a.particle_duration = 1.0f;
	a.radius_rand = 0.5f;
	a.speed_rand = 0.5f;
	a.particle_duration_rand = 0.5f;
	a.dir = v3(1, 1, 1);
	a.dir_rand = v3(1, 1, 1);
	a.color_arr[0].color = make_rgb(1.0f, 0.1f, 0.1f);
	a.pos = pos;
	s_particle_emitter_b b = make_emitter_b();
	b.spawn_type = e_emitter_spawn_type_circle;
	b.spawn_data.x = 0.25f;
	b.particle_count = 128;

	s_entity emitter = zero;
	emitter.emitter_a = a;
	emitter.emitter_b = b;
	return emitter;
}

func s_entity make_player_death_particles(s_v3 pos)
{
	s_particle_emitter_a a = make_emitter_a();
	a.radius = 0.03f;
	a.speed = 4.0f;
	a.particle_duration = 0.5f;
	a.radius_rand = 0.5f;
	a.speed_rand = 0.5f;
	a.particle_duration_rand = 0.5f;
	a.dir = v3(1, 1, 1);
	a.dir_rand = v3(1, 1, 1);
	a.color_arr[0].color = make_rgb(1.0f, 0.1f, 0.1f);
	a.pos = pos;
	s_particle_emitter_b b = make_emitter_b();
	b.spawn_type = e_emitter_spawn_type_circle;
	b.spawn_data.x = 0.25f;
	b.particle_count = 256;

	s_entity emitter = zero;
	emitter.emitter_a = a;
	emitter.emitter_b = b;
	return emitter;
}

func s_v2i get_enemy_animation_frame(float time)
{
	s_v2i c_frame_arr[] = {
		v2i(0, 1), v2i(1, 1), v2i(2, 1),
	};
	s_v2i result = c_frame_arr[floorfi(time) % array_count(c_frame_arr)];
	return result;
}

func s_entity make_sword_particles(s_v3 pos)
{
	s_particle_emitter_a a = make_emitter_a();
	a.radius = 0.02f;
	a.speed = 1.1f;
	a.particle_duration = 0.25f;
	a.radius_rand = 0.5f;
	a.speed_rand = 0.5f;
	a.particle_duration_rand = 0.5f;
	a.dir = v3(1, 1, 1);
	a.dir_rand = v3(1, 1, 1);
	a.color_arr[0].color = make_rgb(0.5f, 0.1f, 0.1f);
	a.color_arr[1].color = make_rgb(0.5f, 0.5f, 0.5f);
	a.color_arr[1].percent = 0.5f;
	a.shrink = 1;
	{
		s_particle_color color = zero;
		color.color = make_rrr(0);
		color.percent = 1;
		a.color_arr.add(color);
	}
	a.pos = pos;
	s_particle_emitter_b b = make_emitter_b();
	// b.spawn_type = e_emitter_spawn_type_circle;
	// b.spawn_data.x = 0.25f;
	b.particle_count = 1;

	s_entity emitter = zero;
	emitter.emitter_a = a;
	emitter.emitter_b = b;
	return emitter;
}