
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
	game->soft_data.zoom = 1;
	game->soft_data.wanted_zoom = 1;

	if(!c_on_web) {
		game->disable_music = true;
	}

	engine_init(platform_data);

	set_state(&game->state0, e_game_state0_main_menu);
	game->do_hard_reset = true;
	set_state(&game->hard_data.state1, e_game_state1_default);

	{
		game->superku.texture = e_texture_superku;
		game->superku.texture_size = v2i(4096, 4502);
		game->superku.sprite_size = v2i(512, 500);
	}

	#if defined(m_winhttp)
	// @TODO(tkap, 08/08/2025): This might break with hot reloading
	game->session = WinHttpOpen(L"A WinHTTP POST Example/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	assert(game->session);
	#endif

	#if defined(__EMSCRIPTEN__) || defined(m_winhttp)
	load_or_create_leaderboard_id();
	#endif

	play_sound(e_sound_music, {.loop = true, .speed = game->music_speed.curr});
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

	s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;

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
	e_game_state1 state1 = (e_game_state1)get_state(&hard_data->state1);

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
					if(key == SDLK_r && event.key.repeat == 0) {
						if(event.key.keysym.mod & KMOD_LCTRL) {
							game->do_hard_reset = true;
						}
						else if(state1 == e_game_state1_defeat) {
							game->do_hard_reset = true;
						}
					}
					else if(key == SDLK_SPACE && event.key.repeat == 0) {
					}
					else if(scancode == SDL_SCANCODE_A) {
					}
					else if(scancode == SDL_SCANCODE_S && event.key.repeat == 0) {
					}
					else if(scancode == SDL_SCANCODE_E && event.key.repeat == 0) {
						if(state0 == e_game_state0_play && state1 == e_game_state1_default) {
							toggle_maybe(&soft_data->open_inventory_timestamp, game->render_time);
							if(soft_data->open_inventory_timestamp.valid) {
								set_maybe_if_invalid(&game->tutorial.opened_inventory, game->render_time);
							}
						}
					}
					else if(scancode == SDL_SCANCODE_Q && event.key.repeat == 0) {
						soft_data->press_input.q = true;
					}
					else if((key == SDLK_ESCAPE && event.key.repeat == 0) || (key == SDLK_o && event.key.repeat == 0) || (key == SDLK_p && event.key.repeat == 0)) {
						if(state0 == e_game_state0_play && state1 == e_game_state1_default) {
							if(soft_data->machine_to_place.valid) {
								soft_data->machine_to_place = zero;
							}
							else if(soft_data->open_inventory_timestamp.valid) {
								soft_data->open_inventory_timestamp = zero;
							}
							else {
								add_state(&game->state0, e_game_state0_pause);
							}
						}
						else if(state0 == e_game_state0_pause) {
							pop_state_transition(&game->state0, game->render_time, c_transition_time);
						}
					}
					#if defined(m_debug)
					else if(key == SDLK_s && event.key.repeat == 0 && event.key.keysym.mod & KMOD_LCTRL) {
					}
					else if(key == SDLK_l && event.key.repeat == 0 && event.key.keysym.mod & KMOD_LCTRL) {
					}
					else if(key == SDLK_KP_PLUS) {
						game->speed_index = circular_index(game->speed_index + 1, array_count(c_game_speed_arr));
						printf("Game speed: %f\n", c_game_speed_arr[game->speed_index]);
					}
					else if(key == SDLK_KP_MINUS) {
						game->speed_index = circular_index(game->speed_index - 1, array_count(c_game_speed_arr));
						printf("Game speed: %f\n", c_game_speed_arr[game->speed_index]);
					}
					else if(key == SDLK_F1) {
						game->cheat_menu_enabled = !game->cheat_menu_enabled;
					}
					else if(key == SDLK_j && event.key.repeat == 0) {
					}
					else if(key == SDLK_h && event.key.repeat == 0) {
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
				float x = range_lerp((float)event.wheel.y, -1, 1, 0.9f, 1.1f);
				if(state0 == e_game_state0_play && state1 == e_game_state1_default) {
					soft_data->wanted_zoom *= x;
					soft_data->wanted_zoom = clamp(soft_data->wanted_zoom, 0.125f, 4.0f);
					set_maybe_if_invalid(&game->tutorial.zoomed, game->render_time);
				}
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
		memset(hard_data, 0, sizeof(*hard_data));
		memset(soft_data, 0, sizeof(*soft_data));
		game->update_time = 0;
		set_state(&game->hard_data.state1, e_game_state1_default);
		reset_linear_arena(&game->arena);
		for_enum(type_i, e_entity) {
			entity_manager_reset(entity_arr, type_i);
		}
		soft_data->zoom = 1;
		soft_data->wanted_zoom = 1;

		soft_data->currency = 60;

		{
			s_v2 pos = v2(
				c_starting_chunk * c_chunk_size * c_tile_size + c_chunk_size * 0.5f * c_tile_size
			);
			unlock_chunk_v2i(v2i(c_starting_chunk, c_starting_chunk));
			s_entity player = zero;
			teleport_entity(&player, pos);
			entity_manager_add(entity_arr, e_entity_player, player);
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		spawn resources start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			for(int chunk_y = 0; chunk_y < c_chunk_count; chunk_y += 1) {
				for(int chunk_x = 0; chunk_x < c_chunk_count; chunk_x += 1) {
					int resource_count = rand_range_ii(&game->rng, 1, 4);
					if(v2i(chunk_x, chunk_y) == v2i(c_starting_chunk, c_starting_chunk)) {
						resource_count = 4;
					}
					for(int i = 0; i < resource_count; i += 1) {
						s_list<e_tile, e_tile_count> possible_resource_arr = zero;
						possible_resource_arr.add(e_tile_resource_1);
						int distance = abs(chunk_x - c_starting_chunk) + abs(chunk_y - c_starting_chunk);
						if(distance >= 2) {
							possible_resource_arr.add(e_tile_resource_2);
						}
						if(distance >= 5) {
							possible_resource_arr.add(e_tile_resource_3);
						}
						int rand_index = rand_range_ie(&game->rng, 0, possible_resource_arr.count);
						e_tile chosen_resource = possible_resource_arr[rand_index];
						int resource_size = rand_range_ii(&game->rng, 10, 60);
						int min_x = chunk_x * c_chunk_size;
						int max_x = min_x + c_chunk_size;
						int min_y = chunk_y * c_chunk_size;
						int max_y = min_y + c_chunk_size;
						int placed = 0;
						s_v2i pos = v2i(
							rand_range_ie(&game->rng, min_x, max_x),
							rand_range_ie(&game->rng, min_y, max_y)
						);
						while(placed < resource_size) {
							if(!is_2d_index_out_of_bounds(pos, v2i(c_max_tiles, c_max_tiles))) {
								s_v2i chunk_index = chunk_index_from_tile_index(pos);
								if(soft_data->natural_terrain_arr[pos.y][pos.x] == e_tile_none && chunk_index == v2i(chunk_x, chunk_y)) {
									soft_data->natural_terrain_arr[pos.y][pos.x] = chosen_resource;
									placed += 1;
								}
							}
							if(rand_bool(&game->rng)) {
								pos.x += rand_bool(&game->rng) ? 1 : -1;
								pos.x = clamp(pos.x, min_x, max_x);
							}
							else {
								pos.y += rand_bool(&game->rng) ? 1 : -1;
								pos.y = clamp(pos.y, min_y, max_y);
							}

						}
					}
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		spawn resources end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		game->music_speed.target = 1;
	}
	if(game->do_soft_reset) {
	}

	b8 should_do_game = false;
	switch(state0) {
		case e_game_state0_play: {
			e_game_state1 state1 = (e_game_state1)get_state(&hard_data->state1);
			switch(state1) {
				xcase e_game_state1_default: {
					should_do_game = true;
				}
				xcase e_game_state1_defeat: {
				}
			}
		} break;

		default: {}
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

	if(should_do_game) {
		game->update_time += (float)c_update_delay;
		hard_data->update_count += 1;
		soft_data->update_count += 1;

		s_stats stats = get_stats();
		soft_data->collector_timer += delta * (1.0f + stats.arr[e_stat_collector_speed] / 100.0f);
		while(soft_data->collector_timer >= 1.33f) {
			soft_data->collector_timer -= 1.33f;

			{
				int to_add = soft_data->machine_count_arr[e_machine_collector_1];
				to_add += soft_data->machine_count_arr[e_machine_collector_2] * 5;
				to_add += soft_data->machine_count_arr[e_machine_collector_3] * 5 * 5;
				add_raw_currency(to_add);
			}
		}

		soft_data->pure_collector_timer += delta * (1.0f + stats.arr[e_stat_collector_speed] / 100.0f);
		while(soft_data->pure_collector_timer >= 1.33f * 5) {
			soft_data->pure_collector_timer -= 1.33f * 5;

			int to_add = soft_data->machine_count_arr[e_machine_pure_collector_1];
			to_add += soft_data->machine_count_arr[e_machine_pure_collector_2] * 5;
			to_add += soft_data->machine_count_arr[e_machine_pure_collector_3] * 5 * 5;
			add_currency(to_add);
		}

		soft_data->processor_timer += delta * (1.0f + stats.arr[e_stat_processor_speed] / 100.0f);
		while(soft_data->processor_timer >= 1.33f) {
			soft_data->processor_timer -= 1.33f;

			{
				int could_process = soft_data->machine_count_arr[e_machine_processor_1] * 2;
				could_process += soft_data->machine_count_arr[e_machine_processor_2] * 2 * 5;
				could_process += soft_data->machine_count_arr[e_machine_processor_3] * 2 * 5 * 5;
				int will_process = min(could_process, soft_data->raw_currency);
				add_raw_currency(-will_process);
				add_currency(will_process);
			}
		}

		if(soft_data->current_research.valid) {
			soft_data->research_timer += delta * (1.0f + stats.arr[e_stat_research_speed] / 100.0f);

			while(soft_data->research_timer >= 0.45f) {
				soft_data->research_timer -= 0.45f;

				int could_process = soft_data->machine_count_arr[e_machine_research_1];
				could_process += soft_data->machine_count_arr[e_machine_research_2] * 5;
				could_process += soft_data->machine_count_arr[e_machine_research_3] * 5 * 5;
				int will_process = min(could_process, (int)soft_data->currency);
				if(game->free_research) {
					will_process = 10000000;
				}
				int research_left = g_research_data[soft_data->current_research.value].cost - soft_data->spent_on_research_arr[soft_data->current_research.value];
				will_process = min(will_process, research_left);
				if(!game->free_research) {
					add_currency(-will_process);
				}
				soft_data->spent_on_research_arr[soft_data->current_research.value] += will_process;
				if(will_process >= research_left) {
					soft_data->research_completed_timestamp_arr[soft_data->current_research.value] = maybe(game->update_time);
					if(soft_data->current_research.value == e_research_win) {
						play_sound(e_sound_win, zero);
					}
					else {
						play_sound(e_sound_upgrade, zero);
					}
					soft_data->current_research = zero;
				}
			}
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			s_entity* player = &entity_arr->data[c_first_index[e_entity_player]];
			player->walking = false;
			s_v2 movement = zero;
			if(soft_data->hold_input.left) {
				movement.x -= 1;
			}
			if(soft_data->hold_input.right) {
				movement.x += 1;
			}
			if(soft_data->hold_input.up) {
				movement.y -= 1;
			}
			if(soft_data->hold_input.down) {
				movement.y += 1;
			}
			if(v2_length(movement) > 0) {
				set_maybe_if_invalid(&game->tutorial.moved, game->render_time);
				player->last_dir = movement;
				player->walking = true;

				float passed = game->render_time - soft_data->last_step_sound_timestamp;
				float p = get_player_speed() / c_base_player_speed;
				if(passed >= 0.25f / p) {
					soft_data->last_step_sound_timestamp = game->render_time;
					if(!game->disable_walk_sound) {
						play_sound(e_sound_step, {.speed = get_rand_sound_speed(1.2f, &game->rng)});
					}
				}
			}
			movement = v2_normalized(movement) * get_player_speed() * delta;

			s_v2 temp_movement = movement;
			while(fabsf(temp_movement.x) > 0) {
				float to_move = at_most(1.0f, fabsf(temp_movement.x));
				float s = sign_as_float(temp_movement.x);
				player->pos.x += to_move * s;
				temp_movement.x -= to_move * s;
				s_v2i chunk_index = chunk_index_from_pos(player->pos);
				if(!is_chunk_unlocked_v2i(chunk_index)) {
					player->pos.x -= to_move * s;
					break;
				}
			}
			while(fabsf(temp_movement.y) > 0) {
				float to_move = at_most(1.0f, fabsf(temp_movement.y));
				float s = sign_as_float(temp_movement.y);
				player->pos.y += to_move * s;
				temp_movement.y -= to_move * s;
				s_v2i chunk_index = chunk_index_from_pos(player->pos);
				if(!is_chunk_unlocked_v2i(chunk_index)) {
					player->pos.y -= to_move * s;
					break;
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	}

	s_maybe<float>* temp = &soft_data->research_completed_timestamp_arr[e_research_win];
	if(temp->valid && game->update_time - temp->value >= 3.0f) {
		*temp = zero;
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
	memset(&soft_data->frame_data, 0, sizeof(soft_data->frame_data));
}

func void render(float interp_dt, float delta)
{
	pre_render(delta);

	s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;
	auto entity_arr = &soft_data->entity_arr;
	s_entity player = entity_arr->data[c_first_index[e_entity_player]];
	s_v2 player_pos = lerp_v2(player.prev_pos, player.pos, interp_dt);

	soft_data->zoom = lerp_snap(soft_data->zoom, soft_data->wanted_zoom, delta * 10, 0.001f);

	s_m4 ortho = make_orthographic(0, c_world_size.x, c_world_size.y, 0, -100, 100);
	s_m4 view;
	s_m4 view_inv;
	{
		float cam_x = (player_pos.x * soft_data->zoom) - c_world_center.x;
		float cam_y = (player_pos.y * soft_data->zoom) - c_world_center.y;
		view = m4_translate(v3(-cam_x, -cam_y, 0.0f));
		view = view * m4_scale(v3(soft_data->zoom, soft_data->zoom, 1.0f));
		view_inv = m4_inverse(view);
	}
	s_m4 view_projection = m4_multiply(ortho, view);

	s_v2 world_mouse = v2_multiply_m4(g_mouse, view_inv);

	clear_framebuffer_color(0, make_rrr(0.0f));

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

		draw_menu_background(ortho, view_inv);
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

		draw_game_name();
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

		draw_menu_background(ortho, view_inv);

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

		draw_game_name();
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
		draw_ground(ortho, view_inv);
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

		draw_ground(ortho, view_inv);

		s_v2 button_size = v2(600, 48);
		s_container container = make_down_center_x_container(make_rect(wxy(0.0f, 0.05f), c_world_size), button_size, 32);

		do_basic_options(&container, button_size);

		{
			s_len_str text = format_text("Lights: %s", game->disable_lights ? "Off" : "On");
			do_bool_button_ex(text, container_get_pos_and_advance(&container), button_size, false, &game->disable_lights);
		}

		{
			s_len_str text = format_text("Walk sound: %s", game->disable_walk_sound ? "Off" : "On");
			do_bool_button_ex(text, container_get_pos_and_advance(&container), button_size, false, &game->disable_walk_sound);
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

		handle_state(&hard_data->state1, game->render_time);

		e_game_state1 state1 = (e_game_state1)get_state(&hard_data->state1);
		switch(state1) {
			xcase e_game_state1_default: {
				should_do_game = true;
			}
			xcase e_game_state1_defeat: {
				should_do_game = true;
				do_defeat = true;
				game->music_speed.target = 0.5f;
			}
		}
	}

	if(should_do_input_name_menu) {
		game->speed = 0;
		game->music_speed.target = 1;
		s_input_name_state* state = &game->input_name_state;
		float font_size = 36;
		s_v2 pos = c_world_size * v2(0.5f, 0.4f);

		draw_ground(ortho, view_inv);

		int count_before = state->name.str.count;
		b8 submitted = handle_string_input(&state->name, game->render_time);
		int count_after = state->name.str.count;
		if(count_before != count_after) {
			play_sound(e_sound_key, zero);
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

		draw_ground(ortho, view_inv);

		if(soft_data->press_input.q && soft_data->machine_to_place.valid) {
			soft_data->machine_to_place = zero;
			soft_data->press_input.q = false;
		}

		s_v2i topleft_index;
		s_v2i bottomright_index;
		{
			int size = c_chunk_size * c_tile_size;
			s_v2 topleft = v2_multiply_m4(v2(0, 0), view_inv);
			s_v2 bottomright = v2_multiply_m4(c_world_size, view_inv);
			topleft_index = v2i(
				floorfi(topleft.x / size),
				floorfi(topleft.y / size)
			);
			topleft_index.x = at_least(0, topleft_index.x - 1);
			topleft_index.y = at_least(0, topleft_index.y - 1);

			bottomright_index = v2i(
				ceilfi(bottomright.x / size),
				ceilfi(bottomright.y / size)
			);
			bottomright_index.x = at_most(c_chunk_count - 1, bottomright_index.x + 1);
			bottomright_index.y = at_most(c_chunk_count - 1, bottomright_index.y + 1);
		}

		{
			s_rect camera_bounds = get_camera_bounds(view_inv);

			soft_data->machine_animation_time += delta * 8;
			if(soft_data->current_research.valid) {
				soft_data->researcher_animation_time += delta * 8;
			}

			for(int chunk_y = topleft_index.y; chunk_y <= bottomright_index.y; chunk_y += 1) {
				for(int chunk_x = topleft_index.x; chunk_x <= bottomright_index.x; chunk_x += 1) {
					s_rng chunk_rng = make_rng(chunk_x * chunk_y);
					// s_v4 tile_color = rand_color(&chunk_rng);
					s_v2i chunk_index = v2i(chunk_x, chunk_y);
					b8 is_unlocked = is_chunk_unlocked_v2i(chunk_index);
					b8 is_unlockable = !is_unlocked && are_any_adjacent_chunks_unlocked(chunk_index);
					if(is_unlocked) {
						for(int y = chunk_y * c_chunk_size; y < chunk_y * c_chunk_size + c_chunk_size; y += 1) {
							for(int x = chunk_x * c_chunk_size; x < chunk_x * c_chunk_size + c_chunk_size; x += 1) {
								// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw terrain start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
								if(soft_data->natural_terrain_arr[y][x] != e_tile_none) {
									s_v2 pos = v2(x, y) * c_tile_size + c_tile_size_v * 0.5f;
									s_v2i atlas_index = c_tile_atlas_index[soft_data->natural_terrain_arr[y][x]];
									s_rng rng = make_rng(x * y);
									float rotation = randf32_11(&rng) * 0.2f;
									s_v4 tile_color = make_rrr(randf_range(&rng, 0.8f, 1.0f) * 0.75f);
									draw_atlas_ex(game->superku, pos, c_tile_size_v * 2.0f, atlas_index, tile_color, rotation, zero, 0);
								}
								// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw terrain end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

								// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw machines start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
								{
									e_machine machine = soft_data->machine_arr[y][x];
									if(machine != e_machine_none) {
										s_v2i tile_index = v2i(x, y);
										s_v2 pos = v2(x, y) * c_tile_size;
										s_v2 size = c_tile_size_v * (float)g_machine_data[machine].size;
										b8 hovered = mouse_vs_rect_topleft(world_mouse, pos, size);
										s_v4 color = make_rrr(1);
										s_v2 tile_center = pos + c_tile_size_v * 0.5f * (float)g_machine_data[machine].size;
										if(hovered && !soft_data->machine_to_place.valid) {
											draw_selector_center(tile_center, size * 1.15f * sin_range(0.95f, 1.05f, game->render_time * 20), 1.0f, 2);
											if(soft_data->press_input.q) {
												soft_data->press_input.q = false;
												soft_data->machine_to_place = maybe(machine);
												set_maybe_if_invalid(&game->tutorial.used_q, game->render_time);
											}
										}
										int frame_index = roundfi(soft_data->machine_animation_time) % g_machine_data[machine].frame_count;
										if(machine == e_machine_research_1 || machine == e_machine_research_2 || machine == e_machine_research_3) {
											frame_index = roundfi(soft_data->researcher_animation_time) % g_machine_data[machine].frame_count;
										}
										s_v2i atlas_index = g_machine_data[machine].frame_arr[frame_index];
										if(machine == e_machine_collector_1) {
											draw_light(tile_center, 128, make_rgb(0.2f, 0.1f, 0.1f), 0.1f, 3);
										}
										else if(machine == e_machine_collector_2) {
											draw_light(tile_center, 128, make_rgb(0.1f, 0.2f, 0.1f), 0.1f, 3);
										}
										else if(machine == e_machine_collector_3) {
											draw_light(tile_center, 128, make_rgb(0.1f, 0.1f, 0.2f), 0.1f, 3);
										}
										draw_atlas(game->superku, tile_center, size * 1.5f, atlas_index, color, 1);
										int dist = get_tile_distance_from_player_to_machine(player.pos, tile_index, machine);
										b8 in_range = dist <= get_player_tile_reach();
										if(hovered && !soft_data->open_inventory_timestamp.valid && is_key_down(c_right_button)) {
											if(in_range) {
												sell_machine(tile_index, machine);
											}
											else {
												s_len_str str = str_from_place_result(e_place_result_out_of_reach);
												add_non_spammy_message(world_mouse, str);
											}
										}
									}
								}
								// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw machines end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
							}
						}
					}
					else {

						s_rect chunk_rect = get_chunk_rect(chunk_index);
						draw_atlas_topleft(game->atlas, chunk_rect.pos, chunk_rect.size, v2i(0, 0), make_rrr(0.0f), 0);

						if(is_unlockable) {
							int chunk_cost = get_chunk_unlock_cost(chunk_index);
							b8 can_afford_chunk = can_afford(soft_data->currency, chunk_cost);
							{
								constexpr float font_size = 128;
								s_len_str text = format_text("$$FFFFFF%i$.", chunk_cost);
								if(!can_afford_chunk) {
									text = format_text("$$FF0000%i$.", chunk_cost);
								}
								s_v2 text_pos = get_chunk_center(chunk_index);

								// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		try to keep the chunk cost on screen in a very dumb way start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
								{
									s_v2 dir = v2_dir_from_to(text_pos, player_pos);
									constexpr float movement_per_step = 2;
									float distance_moved = 0;
									s_v2 text_size = get_text_size(format_text("%i", chunk_cost), &game->font, font_size);
									s_rect temp_bounds = expand_rect_sides_from_center(camera_bounds, -256);
									while(true) {
										b8 is_on_screen = rect_vs_rect_topleft(temp_bounds.pos, temp_bounds.size, text_pos - text_size * 0.5f, text_size);
										if(is_on_screen) {
											break;
										}
										text_pos += dir * movement_per_step;
										distance_moved += movement_per_step;
										if(distance_moved >= 650) {
											break;
										}
									}
								}
								// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		try to keep the chunk cost on screen in a very dumb way end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

								draw_text(text, text_pos, font_size, make_rrr(1), true, &game->font, zero, 2);
							}

							if(!soft_data->open_inventory_timestamp.valid && mouse_vs_rect_topleft(world_mouse, chunk_rect.pos, chunk_rect.size)) {
								draw_atlas(game->atlas, chunk_rect.pos + chunk_rect.size * 0.5f, chunk_rect.size, v2i(0, 0), make_rrr(0.2f), 0);
								if(!soft_data->open_inventory_timestamp.valid && is_key_pressed(c_left_button, true) && can_afford_chunk) {
									unlock_chunk_v2i(chunk_index);
									add_currency(-chunk_cost);
									play_sound(e_sound_upgrade, zero);
								}
							}
						}
					}
				}
			}

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		placing start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			if(soft_data->machine_to_place.valid) {
				e_machine machine = soft_data->machine_to_place.value;
				s_v2i tile_index = tile_index_from_pos(world_mouse);
				if(!is_key_down(c_left_shift)) {
					soft_data->shift_start = tile_index;
				}
				s_v2i topleft = soft_data->shift_start;
				s_v2i bottomright = tile_index;
				if(topleft.x > bottomright.x) {
					int m = (topleft.x - bottomright.x) % g_machine_data[machine].size;
					swap(&topleft.x, &bottomright.x);
					topleft.x += m;
				}
				if(topleft.y > bottomright.y) {
					int m = (topleft.y - bottomright.y) % g_machine_data[machine].size;
					swap(&topleft.y, &bottomright.y);
					topleft.y += m;
				}
				topleft.x = clamp(topleft.x, 0, c_max_tiles - 1);
				topleft.y = clamp(topleft.y, 0, c_max_tiles - 1);
				bottomright.x = clamp(bottomright.x, 0, c_max_tiles - 1);
				bottomright.y = clamp(bottomright.y, 0, c_max_tiles - 1);
				if(bottomright.x - topleft.x >= g_machine_data[machine].size || bottomright.y - topleft.y >= g_machine_data[machine].size) {
					set_maybe_if_invalid(&game->tutorial.used_shift, game->render_time);
				}

				{
					s_v2i curr_index = topleft;
					b8 placed_something = false;
					while(true) {
						s_v2i chunk_index = chunk_index_from_tile_index(curr_index);
						s_v2 pos = c_tile_size_v * curr_index;
						float machine_size = (float)g_machine_data[machine].size;
						s_v2 size = c_tile_size_v * machine_size;
						s_v2i atlas_index = g_machine_data[machine].frame_arr[0];
						e_place_result place_result = can_we_place_machine(player.pos, chunk_index, curr_index, machine, soft_data->currency);
						s_v4 color = make_rgb(0, 1, 0);
						if(place_result == e_place_result_out_of_reach) {
							color = make_rgb(0, 0, 1);
						}
						else if(place_result != e_place_result_success) {
							color = make_rgb(1, 0, 0);
						}
						s_v2 tile_center = pos + c_tile_size_v * 0.5f * machine_size;
						draw_atlas(game->superku, tile_center, size * 1.5f, atlas_index, set_alpha(color, 0.5f), 2);

						if(!game->soft_data.open_inventory_timestamp.valid && is_key_down(c_left_button)) {
							if(place_result == e_place_result_success) {
								place_machine(curr_index, machine);
								placed_something = true;
								add_currency(-get_machine_cost(machine));
							}
							else {
								s_len_str str = str_from_place_result(place_result);
								if(str.count > 0) {
									add_non_spammy_message(world_mouse, str);
								}
							}
						}

						curr_index.x += g_machine_data[machine].size;
						if(curr_index.x > bottomright.x) {
							curr_index.y += g_machine_data[machine].size;
							curr_index.x = topleft.x;
							if(curr_index.y > bottomright.y) { break; }
						}
					}
					if(placed_something) {
						play_sound(e_sound_key, zero);
					}
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		placing end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		}

		do_basic_render_flush(view_projection, 0, view_inv);
		do_basic_render_flush(view_projection, 1, view_inv);
		do_basic_render_flush(view_projection, 2, view_inv);

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			float wanted_rotation = v2_angle(player.last_dir);
			s_entity* temp_player = &entity_arr->data[c_first_index[e_entity_player]];
			if(temp_player->walking) {
				float player_speed = get_player_speed();
				float p = player_speed / c_base_player_speed;
				temp_player->animation_time += delta * p;
			}
			if(temp_player->animation_time >= 0.1f) {
				temp_player->animation_time -= 0.1f;
				temp_player->animation_index = 4 + circular_index(temp_player->animation_index + 1, 4);
			}
			temp_player->rotation = lerp_angle(temp_player->rotation, wanted_rotation + c_pi * 0.5f, delta * 10);
			draw_atlas_ex(game->superku, player_pos, c_player_size_v * 2, v2i(temp_player->animation_index, 7), make_rrr(1), temp_player->rotation, zero, 0);

			draw_light(player_pos, 512, make_rrr(1), 0.1f, 3);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		do_basic_render_flush(view_projection, 0, view_inv);

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		lights start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
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
				draw_text(fct->text, fct->pos, 24, make_ra(1, alpha), true, &game->font, zero, 0);
				fct->pos.y -= delta * 60;
				if(passed >= 1) {
					entity_manager_remove(entity_arr, e_entity_fct, fct_i);
				}
			}
			do_basic_render_flush(view_projection, 0, view_inv);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw fct end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		tutorial start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			breakable_block {
				constexpr float font_size = 48;
				constexpr float advance = font_size + 4;
				float time_passed = 0;
				if(game->tutorial.moved.valid) {
					time_passed = game->render_time - game->tutorial.moved.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.4f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_keycap(scancode_to_char(SDL_SCANCODE_W), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4 + 0)), v2(font_size), alpha, 0);
					draw_keycap(scancode_to_char(SDL_SCANCODE_A), pos + v2(advance * 1, sin_range(-10, 10, game->render_time * 4 + 1)), v2(font_size), alpha, 0);
					draw_keycap(scancode_to_char(SDL_SCANCODE_S), pos + v2(advance * 2, sin_range(-10, 10, game->render_time * 4 + 2)), v2(font_size), alpha, 0);
					draw_keycap(scancode_to_char(SDL_SCANCODE_D), pos + v2(advance * 3, sin_range(-10, 10, game->render_time * 4 + 3)), v2(font_size), alpha, 0);
					draw_text(S("to move"), pos + v2(advance * 4, 0), font_size, make_ra(1, alpha), false, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.zoomed.valid) {
					time_passed = game->render_time - game->tutorial.zoomed.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("Mouse wheel to zoom"), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.opened_inventory.valid) {
					time_passed = game->render_time - game->tutorial.opened_inventory.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.4f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_keycap(scancode_to_char(SDL_SCANCODE_E), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4 + 0)), v2(font_size), alpha, 0);
					draw_text(S("to open menu"), pos + v2(advance * 1, 0), font_size, make_ra(1, alpha), false, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.placed_collector.valid) {
					time_passed = game->render_time - game->tutorial.placed_collector.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("Build a collector (it costs pure energy)\n     It will collect raw energy"), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.placed_processor.valid) {
					time_passed = game->render_time - game->tutorial.placed_processor.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("            Build a processor\nIt will convert raw energy into pure energy"), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.started_a_research.valid) {
					time_passed = game->render_time - game->tutorial.started_a_research.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("   Open the menu and start a research\n(You can pause it by clicking on it again)"), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.placed_researcher.valid) {
					time_passed = game->render_time - game->tutorial.placed_researcher.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.89f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("                    Build a researcher\nIt will consume pure energy until the research completes\n      (Build more researchers to research faster)"), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.deleted_machine.valid) {
					time_passed = game->render_time - game->tutorial.deleted_machine.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("Right click a machine to delete it\n (you get your pure energy back)"), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.used_q.valid) {
					time_passed = game->render_time - game->tutorial.used_q.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.1f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_keycap(scancode_to_char(SDL_SCANCODE_Q), pos + v2(advance * 0, sin_range(-10, 10, game->render_time * 4 + 0)), v2(font_size), alpha, 0);
					draw_text(S("on a machine to begin placing that type of machine"), pos + v2(advance * 1, 0), font_size, make_ra(1, alpha), false, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.used_shift.valid) {
					time_passed = game->render_time - game->tutorial.used_shift.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("When placing machines, hold shift\n to place in a rectangle pattern"), pos + v2(advance * 1, sin_range(-10, 10, game->render_time * 4 + 0)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				time_passed = 0;
				if(game->tutorial.unlocked_chunk.valid) {
					time_passed = game->render_time - game->tutorial.unlocked_chunk.value;
				}
				if(time_passed <= 1.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = 1.0f - powf(time_passed, 2.0f);
					draw_text(S("Click on an unrevealed area to reveal it\n        (It costs pure energy)"), pos + v2(advance * 1, sin_range(-10, 10, game->render_time * 4 + 0)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------

				set_maybe_if_invalid(&game->tutorial.tutorial_end, game->render_time);

				time_passed = 0;
				if(game->tutorial.tutorial_end.valid) {
					time_passed = game->render_time - game->tutorial.tutorial_end.value;
				}
				if(time_passed <= 10.0f) {
					s_v2 pos = wxy(0.5f, 0.9f);
					float alpha = ease_in_expo_advanced(time_passed, 0, 10, 1, 0);
					draw_text(S("Your goal is to research a specific technology\n                Good luck!"), pos + v2(advance * 1, sin_range(-10, 10, game->render_time * 4 + 0)), font_size, make_ra(1, alpha), true, &game->font, zero, 0);
					break;
				}
				// ------------------------------------------------------------------------
			}
			do_basic_render_flush(ortho, 0, view_inv);
			do_basic_render_flush(ortho, 1, view_inv);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		tutorial end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// {
		// 	s_render_flush_data data = make_render_flush_data(zero, zero);
		// 	data.projection = ortho;
		// 	data.blend_mode = e_blend_mode_normal;
		// 	data.depth_mode = e_depth_mode_no_read_no_write;
		// 	render_flush(data, true, 0);
		// }


		// update_particles(delta, true, 0);

		// {
		// 	s_render_flush_data data = make_render_flush_data(zero, zero);
		// 	data.projection = ortho;
		// 	data.blend_mode = e_blend_mode_additive;
		// 	data.depth_mode = e_depth_mode_no_read_no_write;
		// 	render_flush(data, true, 0);
		// }

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
				s_v2 pos = container_get_pos_and_advance(&container);
				s_v2 size = v2(48);
				s_v2 icon_pos = pos + size * 0.5f;
				s_v2 icon_size = size * 1.75f;
				draw_atlas(game->superku, icon_pos, icon_size, v2i(7, 6), make_rrr(1), 0);
				pos.x += size.x + 4;
				s_len_str text = format_text("%i", soft_data->currency);
				draw_text(text, pos, 48, make_rrr(1), false, &game->font, zero, 0);

				if(!soft_data->open_inventory_timestamp.valid && mouse_vs_rect_center(g_mouse, icon_pos, size)) {
					game->tooltip = S("Pure energy");
				}
			}

			{
				s_v2 pos = container_get_pos_and_advance(&container);
				s_v2 size = v2(48);
				s_v2 icon_pos = pos + size * 0.5f;
				s_v2 icon_size = size * 1.75f;
				draw_atlas(game->superku, icon_pos, icon_size, v2i(6, 6), make_rrr(1), 0);
				pos.x += size.x + 4;
				s_len_str text = format_text("%i", soft_data->raw_currency);
				draw_text(text, pos, 48, make_rrr(1), false, &game->font, zero, 0);

				if(!soft_data->open_inventory_timestamp.valid && mouse_vs_rect_center(g_mouse, icon_pos, size)) {
					game->tooltip = S("Raw energy");
				}
			}

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		research progress bar start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			if(soft_data->current_research.valid) {
				e_research r = soft_data->current_research.value;
				s_v2 pos = wxy(0.78f, 0.03f);
				s_v2 under_size = wxy(0.2f, 0.03f);
				float research_percentage = soft_data->spent_on_research_arr[r] / (float)g_research_data[r].cost;
				s_v2 over_size = v2(under_size.x * research_percentage, under_size.y);
				draw_rect_topleft(pos, under_size, make_rrr(0.5f), 0);
				draw_rect_topleft(pos, over_size, make_rrr(1), 0);
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		research progress bar end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		inventory start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			if(soft_data->open_inventory_timestamp.valid) {
				float p = at_most(1.0f, game->render_time - soft_data->open_inventory_timestamp.value);
				p = ease_out_back_advanced(p, 0, 0.33f, 0, 1);
				draw_rect(c_world_center, c_world_center * 1.75f * p, make_ra(0.0f, 0.75f), 0);

				{
					draw_atlas(game->superku, wxy(0.45f, 0.2f), v2(220 * p), v2i(6, 0), make_rrr(1), 0);
					draw_atlas(game->superku, wxy(0.45f, 0.5f), v2(220 * p), v2i(6, 1), make_rrr(1), 0);
					draw_atlas(game->superku, wxy(0.45f, 0.8f), v2(220 * p), v2i(6, 2), make_rrr(1), 0);
				}


				s_v2 rect_size = v2(64) * p;

				{
					e_machine arr[] = {
						e_machine_collector_1, e_machine_collector_2, e_machine_collector_3,
						e_machine_processor_1, e_machine_processor_2, e_machine_processor_3,
						e_machine_research_1, e_machine_research_2, e_machine_research_3,
						e_machine_pure_collector_1, e_machine_pure_collector_2, e_machine_pure_collector_3,
					};
					draw_text(S("Machines"), wxy(0.1f, 0.13f), 48 * p, make_rrr(1), false, &game->font, zero, 1);
					draw_text(S("Research"), wxy(0.6f, 0.13f), 48 * p, make_rrr(1), false, &game->font, zero, 1);
					s_v2 base_pos = wxy(0.1f, 0.2f);
					s_v2 pos = base_pos;
					for(int i = 0; i < array_count(arr); i += 1) {
						e_machine machine = arr[i];
						b8 unlocked = is_machine_unlocked(machine);
						b8 hovered = mouse_vs_rect_topleft(g_mouse, pos, rect_size);
						float flash = 0.7f;
						if(hovered) {
							if(unlocked) {
								game->tooltip = get_machine_tooltip(machine);
							}
							else {
								game->tooltip = S("Research to unlock!");
							}
							flash = 1;
						}
						s_v2i atlas_index = g_machine_data[machine].frame_arr[0];
						if(unlocked) {
							if(hovered && is_key_pressed(c_left_button, true)) {
								soft_data->machine_to_place = maybe(machine);
								soft_data->open_inventory_timestamp = zero;
							}
							s_v2 rect_center = pos + rect_size * 0.5f;
							draw_atlas(game->superku, rect_center - v2(0, 4), rect_size * 1.5f, atlas_index, make_rrr(flash), 1);
							draw_atlas_topleft(game->atlas, pos, rect_size, v2i(0, 0), make_rrr(flash * 0.5f), 1);
						}
						else {
							draw_undiscovered_slot(pos, rect_size, flash);
						}
						pos.x += 80;
						if((i + 1) % 3 == 0) {
							pos.x = base_pos.x;
							pos.y += 80;
						}
					}
				}

				{
					s_list<e_research, e_research_count> satisfied_arr = zero;
					s_list<e_research, e_research_count> unsatisfied_arr = zero;
					for_enum(research_i, e_research) {
						if(soft_data->research_completed_timestamp_arr[research_i].valid) {
							continue;
						}
						b8 requirements_satisfied = are_research_requirements_satisfied(research_i);
						if(requirements_satisfied) {
							satisfied_arr.add(research_i);
						}
						else {
							unsatisfied_arr.add(research_i);
						}
					}

					s_v2 base_pos = wxy(0.6f, 0.2f);
					int column = 0;
					int row = 0;
					foreach_val(research_i, research, satisfied_arr) {
						s_v2 pos = base_pos + v2(column * 80, row * 80);
						float flash = 0.7f;
						b8 hovered = mouse_vs_rect_topleft(g_mouse, pos, rect_size);
						if(hovered) {
							flash = 1;
							game->tooltip = get_research_tooltip(research);
						}
						if(soft_data->current_research.valid && soft_data->current_research.value == research) {
							flash = sin_range(0.5f, 1.0f, game->render_time * 15);
						}
						s_v4 color = make_rrr(flash);
						draw_atlas_topleft(game->atlas, pos, rect_size, v2i(0, 0), make_rrr(flash * 0.5f), 0);
						draw_atlas_topleft(game->atlas, pos, rect_size, v2i(6, 0), color, 1);

						if(hovered && is_key_pressed(c_left_button, true)) {
							if(soft_data->current_research.valid && soft_data->current_research.value == research) {
								soft_data->current_research = zero;
							}
							else {
								soft_data->current_research = maybe(research);
								set_maybe_if_invalid(&game->tutorial.started_a_research, game->render_time);
							}
							soft_data->open_inventory_timestamp = zero;
						}
						column += 1;
						if(column >= 5) {
							column = 0;
							row += 1;
						}
					}
					foreach_val(research_i, research, unsatisfied_arr) {
						s_v2 pos = base_pos + v2(column * 80, row * 80);
						float flash = 0.7f;
						b8 hovered = mouse_vs_rect_topleft(g_mouse, pos, rect_size);
						if(hovered) {
							flash = 1;
							game->tooltip = S("Research to unlock!");
						}
						draw_undiscovered_slot(pos, rect_size, flash);
						column += 1;
						if(column >= 5) {
							column = 0;
							row += 1;
						}
					}
				}

				do_basic_render_flush(ortho, 0, view_inv);
				do_basic_render_flush(ortho, 1, view_inv);
				do_basic_render_flush(ortho, 2, view_inv);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		inventory end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

			{
				s_len_str text = format_text("10x movement speed: %s", game->fast_player_speed ? "On" : "Off");
				do_bool_button_ex(text, container_get_pos_and_advance(&container), size, false, &game->fast_player_speed);
			}

			{
				s_len_str text = format_text("Super reach: %s", game->player_super_reach ? "On" : "Off");
				do_bool_button_ex(text, container_get_pos_and_advance(&container), size, false, &game->player_super_reach);
			}

			{
				s_len_str text = format_text("Free research: %s", game->free_research ? "On" : "Off");
				do_bool_button_ex(text, container_get_pos_and_advance(&container), size, false, &game->free_research);
			}

			{
				s_len_str text = format_text("Reset zoom");
				if(do_button_ex(text, container_get_pos_and_advance(&container), size, false, zero) == e_button_result_left_click) {
					soft_data->zoom = 1;
				}
			}

			{
				s_len_str text = format_text("+10000000 currency");
				if(do_button_ex(text, container_get_pos_and_advance(&container), size, false, zero) == e_button_result_left_click) {
					soft_data->currency += 10000000;
				}
			}

			do_basic_render_flush(ortho, 0, view_inv);
		}
		#endif
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		cheat menu end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	}
	else {
		update_particles(delta, false, 0);
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

	{
		do_lerpable_snap(&game->music_speed, delta * 2.0f, 0.01f);
		s_active_sound* music = find_playing_sound(e_sound_music);
		if(music) {
			music->data.speed = game->music_speed.curr;
		}
	}

	SDL_GL_SwapWindow(g_platform_data->window);

	game->render_time += delta;
}

func f64 get_seconds()
{
	u64 now =	SDL_GetPerformanceCounter();
	return (now - g_platform_data->start_cycles) / (f64)g_platform_data->cycle_frequency;
}

func void on_gl_error(const char* expr, char* file, int line, int error)
{
	#define m_gl_errors \
	X(GL_INVALID_ENUM, "GL_INVALID_ENUM") \
	X(GL_INVALID_VALUE, "GL_INVALID_VALUE") \
	X(GL_INVALID_OPERATION, "GL_INVALID_OPERATION") \
	X(GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW") \
	X(GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW") \
	X(GL_OUT_OF_MEMORY, "GL_STACK_OUT_OF_MEMORY") \
	X(GL_INVALID_FRAMEBUFFER_OPERATION, "GL_STACK_INVALID_FRAME_BUFFER_OPERATION")

	const char* error_str;
	#define X(a, b) case a: { error_str = b; } break;
	switch(error) {
		m_gl_errors
		default: {
			error_str = "unknown error";
		} break;
	}
	#undef X
	#undef m_gl_errors

	printf("GL ERROR: %s - %i (%s)\n", expr, error, error_str);
	printf("  %s(%i)\n", file, line);
	printf("--------\n");

	#if defined(_WIN32)
	__debugbreak();
	#else
	__builtin_trap();
	#endif
}

func void draw_rect(s_v2 pos, s_v2 size, s_v4 color, int render_pass_index)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
	data.color = color;

	add_to_render_group(data, e_shader_flat, e_texture_white, e_mesh_quad, render_pass_index);
}

func void draw_rect_3d(s_v3 pos, s_v2 size, s_v4 color, int render_pass_index)
{
	s_instance_data data = zero;
	data.model = m4_translate(pos);
	data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
	data.color = color;

	add_to_render_group(data, e_shader_flat, e_texture_white, e_mesh_quad, render_pass_index);
}

func void draw_plane(s_v3 topleft, s_v3 topright, s_v3 bottomleft, s_v3 bottomright, s_v4 color, int render_pass_index)
{
	s_plane_instance data = zero;
	data.topleft = topleft;
	data.topright = topright;
	data.bottomleft = bottomleft;
	data.bottomright = bottomright;
	data.color = color;
	add_to_render_group(data, e_shader_plane, e_texture_white, e_mesh_plane, render_pass_index);
}

func void draw_rect_topleft(s_v2 pos, s_v2 size, s_v4 color, int render_pass_index)
{
	pos += size * 0.5f;
	draw_rect(pos, size, color, render_pass_index);
}

func void draw_texture_screen(s_v2 pos, s_v2 size, s_v4 color, e_texture texture_id, e_shader shader_id, s_v2 uv_min, s_v2 uv_max, s_draw_data draw_data, int render_pass_index)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, draw_data.z));
	data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
	data.color = color;
	data.uv_min = uv_min;
	data.uv_max = uv_max;

	add_to_render_group(data, shader_id, texture_id, e_mesh_quad, render_pass_index);
}

func void draw_mesh(e_mesh mesh_id, s_m4 model, s_v4 color, e_shader shader_id, int render_pass_index)
{
	s_instance_data data = zero;
	data.model = model;
	data.color = color;
	add_to_render_group(data, shader_id, e_texture_white, mesh_id, render_pass_index);
}

func void draw_mesh(e_mesh mesh_id, s_v3 pos, s_v3 size, s_v4 color, e_shader shader_id, int render_pass_index)
{
	s_m4 model = m4_translate(pos);
	model = m4_multiply(model, m4_scale(size));
	draw_mesh(mesh_id, model, color, shader_id, render_pass_index);
}

func void bind_framebuffer(u32 fbo)
{
	if(game->curr_fbo != fbo) {
		gl(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
		game->curr_fbo = fbo;
	}
}

func void clear_framebuffer_color(u32 fbo, s_v4 color)
{
	bind_framebuffer(fbo);
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

func void clear_framebuffer_depth(u32 fbo)
{
	bind_framebuffer(fbo);
	set_depth_mode(e_depth_mode_read_and_write);
	glClear(GL_DEPTH_BUFFER_BIT);
}

func void render_flush(s_render_flush_data data, b8 reset_render_count, int render_pass_index)
{
	bind_framebuffer(data.fbo.id);

	assert(render_pass_index >= 0);
	assert(render_pass_index < c_render_pass_count);
	s_render_pass* render_pass = &game->render_pass_arr[render_pass_index];

	if(data.fbo.id == 0) {
		s_rect letterbox = do_letterbox(v2(g_platform_data->window_size), c_world_size);
		glViewport((int)letterbox.x, (int)letterbox.y, (int)letterbox.w, (int)letterbox.h);
	}
	else {
		glViewport(0, 0, data.fbo.size.x, data.fbo.size.y);
	}

	set_cull_mode(data.cull_mode);
	set_depth_mode(data.depth_mode);
	set_blend_mode(data.blend_mode);

	{
		gl(glBindBuffer(GL_UNIFORM_BUFFER, game->ubo));
		s_uniform_data uniform_data = zero;
		uniform_data.view = data.view;
		uniform_data.projection = data.projection;
		uniform_data.light_view = data.light_view;
		uniform_data.light_projection = data.light_projection;
		uniform_data.render_time = game->render_time;
		uniform_data.cam_pos = data.cam_pos;
		uniform_data.mouse = g_mouse;
		uniform_data.player_pos = data.player_pos;
		uniform_data.window_size.x = (float)g_platform_data->window_size.x;
		uniform_data.window_size.y = (float)g_platform_data->window_size.y;

		{
			s_rect r = get_camera_bounds(data.view_inv);
			uniform_data.camera_topleft = r.pos;
			uniform_data.camera_bottomright = r.pos + r.size;
		}

		gl(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(s_uniform_data), &uniform_data));
	}

	foreach_val(group_i, group, render_pass->render_group_arr) {
		s_mesh* mesh = &game->mesh_arr[group.mesh_id];
		int* instance_count = &render_pass->render_instance_count[group.shader_id][group.texture_id][group.mesh_id];
		assert(*instance_count > 0);
		void* instance_data = render_pass->render_instance_arr[group.shader_id][group.texture_id][group.mesh_id];

		gl(glUseProgram(game->shader_arr[group.shader_id].id));

		int in_texture_loc = glGetUniformLocation(game->shader_arr[group.shader_id].id, "in_texture");
		int noise_loc = glGetUniformLocation(game->shader_arr[group.shader_id].id, "noise");
		if(in_texture_loc >= 0) {
			glUniform1i(in_texture_loc, 0);
			glActiveTexture(GL_TEXTURE0);
			gl(glBindTexture(GL_TEXTURE_2D, game->texture_arr[group.texture_id].id));
		}
		if(noise_loc >= 0) {
			glUniform1i(noise_loc, 2);
			glActiveTexture(GL_TEXTURE2);
			gl(glBindTexture(GL_TEXTURE_2D, game->texture_arr[e_texture_noise].id));
		}

		gl(glBindVertexArray(mesh->vao));
		gl(glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo.id));
		gl(glBufferSubData(GL_ARRAY_BUFFER, 0, group.element_size * *instance_count, instance_data));
		gl(glDrawArraysInstanced(GL_TRIANGLES, 0, mesh->vertex_count, *instance_count));
		if(reset_render_count) {
			render_pass->render_group_arr.remove_and_swap(group_i);
			render_pass->render_group_index_arr[group.shader_id][group.texture_id][group.mesh_id] = -1;
			group_i -= 1;
			*instance_count = 0;
		}
	}
}

template <typename t>
func void add_to_render_group(t data, e_shader shader_id, e_texture texture_id, e_mesh mesh_id, int render_pass_index)
{
	assert(render_pass_index >= 0);
	assert(render_pass_index < c_render_pass_count);
	s_render_pass* render_pass = &game->render_pass_arr[render_pass_index];

	s_render_group render_group = zero;
	render_group.shader_id = shader_id;
	render_group.texture_id = texture_id;
	render_group.mesh_id = mesh_id;
	render_group.element_size = sizeof(t);

	s_mesh* mesh = &game->mesh_arr[render_group.mesh_id];

	int* render_group_index = &render_pass->render_group_index_arr[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	if(*render_group_index < 0) {
		render_pass->render_group_arr.add(render_group);
		*render_group_index = render_pass->render_group_arr.count - 1;
	}
	int* count = &render_pass->render_instance_count[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	int* max_elements = &render_pass->render_instance_max_elements[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	t* ptr = (t*)render_pass->render_instance_arr[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	b8 expand = *max_elements <= *count;
	b8 get_new_ptr = *count <= 0 || expand;
	int new_max_elements = *max_elements;
	if(expand) {
		if(new_max_elements <= 0) {
			new_max_elements = 64;
		}
		else {
			new_max_elements *= 2;
		}
		if(new_max_elements > mesh->instance_vbo.max_elements) {
			gl(glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo.id));
			gl(glBufferData(GL_ARRAY_BUFFER, sizeof(t) * new_max_elements, null, GL_DYNAMIC_DRAW));
			mesh->instance_vbo.max_elements = new_max_elements;
		}
	}
	if(get_new_ptr) {
		t* temp = (t*)arena_alloc(&game->render_frame_arena, sizeof(t) * new_max_elements);
		if(*count > 0) {
			memcpy(temp, ptr, *count * sizeof(t));
		}
		render_pass->render_instance_arr[render_group.shader_id][render_group.texture_id][render_group.mesh_id] = (void*)temp;
		ptr = temp;
	}
	*max_elements = new_max_elements;
	*(ptr + *count) = data;
	*count += 1;
}

func s_shader load_shader_from_file(char* file, s_linear_arena* arena)
{
	b8 delete_shaders[2] = {true, true};
	char* src = (char*)read_file(file, arena, null);
	assert(src);

	u32 shader_arr[] = {glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER)};

	#if defined(__EMSCRIPTEN__)
	const char* header = "#version 300 es\nprecision highp float;\n";
	#else
	const char* header = "#version 330 core\n";
	#endif

	char* shared_src = (char*)try_really_hard_to_read_file("src/shader_shared.h", arena);
	assert(shared_src);

	for(int i = 0; i < 2; i += 1) {
		const char* src_arr[] = {header, "", "", shared_src, src};
		if(i == 0) {
			src_arr[1] = "#define m_vertex 1\n";
			src_arr[2] = "#define shared_var out\n";
		}
		else {
			src_arr[1] = "#define m_fragment 1\n";
			src_arr[2] = "#define shared_var in\n";
		}
		gl(glShaderSource(shader_arr[i], array_count(src_arr), (const GLchar * const *)src_arr, null));
		gl(glCompileShader(shader_arr[i]));

		int compile_success;
		char info_log[1024];
		gl(glGetShaderiv(shader_arr[i], GL_COMPILE_STATUS, &compile_success));

		if(!compile_success) {
			gl(glGetShaderInfoLog(shader_arr[i], sizeof(info_log), null, info_log));
			printf("Failed to compile shader: %s\n%s", file, info_log);
			delete_shaders[i] = false;
		}
	}

	b8 detach_shaders = delete_shaders[0] && delete_shaders[1];

	u32 program = 0;
	if(delete_shaders[0] && delete_shaders[1]) {
		program = gl(glCreateProgram());
		gl(glAttachShader(program, shader_arr[0]));
		gl(glAttachShader(program, shader_arr[1]));
		gl(glLinkProgram(program));

		int linked = 0;
		gl(glGetProgramiv(program, GL_LINK_STATUS, &linked));
		if(!linked) {
			char info_log[1024] = zero;
			gl(glGetProgramInfoLog(program, sizeof(info_log), null, info_log));
			printf("FAILED TO LINK: %s\n", info_log);
		}
	}

	if(detach_shaders) {
		gl(glDetachShader(program, shader_arr[0]));
		gl(glDetachShader(program, shader_arr[1]));
	}

	if(delete_shaders[0]) {
		gl(glDeleteShader(shader_arr[0]));
	}
	if(delete_shaders[1]) {
		gl(glDeleteShader(shader_arr[1]));
	}

	s_shader result = zero;
	result.id = program;
	return result;
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
					play_sound(e_sound_click, zero);
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

func b8 is_key_pressed(int key, b8 consume)
{
	b8 result = false;
	if(key < c_max_keys) {
		result = game->input_arr[key].half_transition_count > 1 || (game->input_arr[key].half_transition_count > 0 && game->input_arr[key].is_down);
	}
	if(result && consume) {
		game->input_arr[key].half_transition_count = 0;
		game->input_arr[key].is_down = false;
	}
	return result;
}

func b8 is_key_down(int key)
{
	b8 result = false;
	if(key < c_max_keys) {
		result = game->input_arr[key].half_transition_count > 1 || game->input_arr[key].is_down;
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

template <int n>
func b8 handle_string_input(s_input_str<n>* str, float time)
{
	b8 result = false;
	if(!str->cursor.valid) {
		str->cursor = maybe(0);
	}
	foreach_val(c_i, c, game->char_events) {
		if(is_alpha_numeric(c) || c == '_') {
			if(!is_builder_full(&str->str)) {
				builder_insert_char(&str->str, str->cursor.value, c);
				str->cursor.value += 1;
				str->last_edit_time = time;
				str->last_action_time = str->last_edit_time;
			}
		}
	}

	foreach_val(event_i, event, game->key_events) {
		if(!event.went_down) { continue; }
		if(event.key == SDLK_RETURN) {
			result = true;
			str->last_action_time = time;
		}
		else if(event.key == SDLK_ESCAPE) {
			str->cursor.value = 0;
			str->str.count = 0;
			str->str.str[0] = 0;
			str->last_edit_time = time;
			str->last_action_time = str->last_edit_time;
		}
		else if(event.key == SDLK_BACKSPACE) {
			if(str->cursor.value > 0) {
				str->cursor.value -= 1;
				builder_remove_char_at(&str->str, str->cursor.value);
				str->last_edit_time = time;
				str->last_action_time = str->last_edit_time;
			}
		}
	}
	return result;
}

func void handle_key_event(int key, b8 is_down, b8 is_repeat)
{
	if(is_down) {
		game->any_key_pressed = true;
	}
	if(key < c_max_keys) {
		if(!is_repeat) {
			game->any_key_pressed = true;
		}

		{
			s_key_event key_event = {};
			key_event.went_down = is_down;
			key_event.key = key;
			// key_event.modifiers |= e_input_modifier_ctrl * is_key_down(&g_platform_data.input, c_key_left_ctrl);
			game->key_events.add(key_event);
		}
	}
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

func s_v2 get_rect_normal_of_closest_edge(s_v2 p, s_v2 center, s_v2 size)
{
	s_v2 edge_arr[] = {
		v2(center.x - size.x * 0.5f, center.y),
		v2(center.x + size.x * 0.5f, center.y),
		v2(center.x, center.y - size.y * 0.5f),
		v2(center.x, center.y + size.y * 0.5f),
	};
	s_v2 normal_arr[] = {
		v2(-1, 0),
		v2(1, 0),
		v2(0, -1),
		v2(0, 1),
	};
	float closest_dist[2] = {9999999, 9999999};
	int closest_index[2] = {0, 0};

	for(int i = 0; i < 4; i += 1) {
		float dist;
		if(i <= 1) {
			dist = fabsf(p.x - edge_arr[i].x);
		}
		else {
			dist = fabsf(p.y - edge_arr[i].y);
		}
		if(dist < closest_dist[0]) {
			closest_dist[0] = dist;
			closest_index[0] = i;
		}
		else if(dist < closest_dist[1]) {
			closest_dist[1] = dist;
			closest_index[1] = i;
		}
	}
	s_v2 result = normal_arr[closest_index[0]];

	// @Note(tkap, 19/04/2025): Probably breaks for very thin rectangles
	if(fabsf(closest_dist[0] - closest_dist[1]) <= 0.01f) {
		result += normal_arr[closest_index[1]];
		result = v2_normalized(result);
	}
	return result;
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

func void draw_atlas(s_atlas atlas, s_v2 pos, s_v2 size, s_v2i index, s_v4 color, int render_pass_index)
{
	draw_atlas_ex(atlas, pos, size, index, color, 0, zero, render_pass_index);
}

func void draw_atlas_ex(s_atlas atlas, s_v2 pos, s_v2 size, s_v2i index, s_v4 color, float rotation, s_draw_data draw_data, int render_pass_index)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, draw_data.z));
	if(rotation != 0) {
		data.model *= m4_rotate(rotation, v3(0, 0, 1));
	}
	data.model *= m4_scale(v3(size, 1));
	data.color = color;
	int x = index.x * atlas.sprite_size.x + atlas.padding;
	data.uv_min.x = x / (float)atlas.texture_size.x;
	data.uv_max.x = data.uv_min.x + (atlas.sprite_size.x - atlas.padding) / (float)atlas.texture_size.x;
	int y = index.y * atlas.sprite_size.y + atlas.padding;
	data.uv_min.y = y / (float)(atlas.texture_size.y);
	data.uv_max.y = data.uv_min.y + (atlas.sprite_size.y - atlas.padding) / (float)atlas.texture_size.y;
	data.mix_weight = draw_data.mix_weight;
	data.mix_color = draw_data.mix_color;

	if(draw_data.flip_x) {
		swap(&data.uv_min.x, &data.uv_max.x);
	}

	add_to_render_group(data, e_shader_flat, atlas.texture, e_mesh_quad, render_pass_index);
}

func void draw_atlas_topleft(s_atlas atlas, s_v2 pos, s_v2 size, s_v2i index, s_v4 color, int render_pass_index)
{
	pos += size * 0.5f;
	draw_atlas(atlas, pos, size, index, color, render_pass_index);
}

func void draw_circle(s_v2 pos, float radius, s_v4 color, int render_pass_index)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(radius * 2, radius * 2, 1)));
	data.color = color;

	add_to_render_group(data, e_shader_circle, e_texture_white, e_mesh_quad, render_pass_index);
}

func void draw_light(s_v2 pos, float radius, s_v4 color, float smoothness, int render_pass_index)
{
	if(game->disable_lights) {
		return;
	}

	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(radius * 2, radius * 2, 1)));
	data.color = color;
	data.mix_weight = smoothness;

	add_to_render_group(data, e_shader_light, e_texture_white, e_mesh_quad, render_pass_index);
}

func void do_screen_shake(float intensity)
{
	s_soft_game_data* soft_data = &game->soft_data;
	soft_data->start_screen_shake_timestamp = game->render_time;
	soft_data->shake_intensity = intensity;
}

func void teleport_entity(s_entity* entity, s_v2 pos)
{
	entity->pos = pos;
	entity->prev_pos = pos;
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

func s_v2 gxy(float x, float y)
{
	s_v2 result = {x * c_game_area.x, y * c_game_area.y};
	return result;
}

func s_v2 gxy(float x)
{
	s_v2 result = gxy(x, x);
	return result;
}


func s_container make_center_x_container(s_rect rect, s_v2 element_size, float padding, int element_count)
{
	assert(element_count > 0);

	s_container result = zero;
	float space_used = (element_size.x * element_count) + (padding * (element_count - 1));
	float space_left = rect.size.x - space_used;
	result.advance.x = element_size.x + padding;
	result.curr_pos.x = rect.pos.x + space_left * 0.5f;
	result.curr_pos.y = rect.pos.y + rect.size.y * 0.5f - element_size.y * 0.5f;
	return result;
}

func s_container make_forward_container(s_rect rect, s_v2 element_size, float padding)
{
	s_container result = zero;
	result.advance.x = element_size.x + padding;
	result.curr_pos.x = rect.pos.x + padding;
	result.curr_pos.y = rect.pos.y + rect.size.y * 0.5f - element_size.y * 0.5f;
	return result;
}

func s_container make_down_center_x_container(s_rect rect, s_v2 element_size, float padding)
{
	s_container result = zero;
	result.advance.y = element_size.y + padding;
	result.curr_pos.x = rect.pos.x + rect.size.x * 0.5f - element_size.x * 0.5f;
	result.curr_pos.y = rect.pos.y + padding;
	return result;
}

func s_container make_forward_align_right_container(s_rect rect, s_v2 element_size, float padding, float edge_padding, int element_count)
{
	s_container result = zero;
	float space_used = (element_size.x * element_count) + (padding * (element_count - 1));
	result.advance.x = element_size.x + padding;
	result.curr_pos.x = rect.size.x - space_used - edge_padding;
	result.curr_pos.y = rect.pos.y + rect.size.y * 0.5f - element_size.y * 0.5f;
	return result;
}

func s_v2 container_get_pos_and_advance(s_container* c)
{
	s_v2 result = c->curr_pos;
	c->curr_pos += c->advance;
	return result;
}

func s_v2 topleft_to_bottomleft_mouse(s_v2 pos, s_v2 size, s_v2 mouse)
{
	s_v2 result = pos;
	result += mouse;
	result.x -= pos.x;
	result.y -= pos.y;
	result.y -= size.y;
	return result;
}

func s_v2 prevent_offscreen(s_v2 pos, s_v2 size)
{
	s_v2 result = pos;
	float* ptr[4] = {
		&result.x, &result.x, &result.y, &result.y
	};
	// left, right, up, down
	float diff[4] = {
		pos.x - 0,
		c_world_size.x - (pos.x + size.x),
		pos.y - 0,
		c_world_size.y - (pos.y + size.y),
	};
	for(int i = 0; i < 4; i += 1) {
		if(diff[i] < 0) {
			float x = i % 2 == 0 ? -1.0f : 1.0f;
			*ptr[i] += diff[i] * x;
		}
	}
	return result;
}

func char* handle_plural(float x)
{
	char* result = "s";
	if(fabsf(x) == 1.0f) {
		result = "";
	}
	return result;
}

template <typename t, typename F>
func void radix_sort_32(t* source, u32 count, F get_radix, s_linear_arena* arena)
{
	if(count == 0) { return; }

	t* destination = (t*)arena_alloc(arena, sizeof(t) * count);

	for(u32 index_i = 0; index_i < 32; index_i += 8) {
		u32 offsets[256] = zero;

		// @Note(tkap, 02/07/2024): Count how many of each value between 0-255 we have
		// [0, 1, 1, 1, 2, 2] == [1, 3, 2]
		for(u32 i = 0; i < count; i += 1) {
			u32 val = get_radix(source[i]);
			u32 mask = 0xff << index_i;
			u32 radix_val = (val & mask) >> index_i;
			offsets[radix_val] += 1;
		}

		// @Note(tkap, 02/07/2024): Turn the offsets array from an array of counts into an array of offsets
		u32 total = 0;
		for(u32 i = 0; i < 256; i += 1) {
			u32 temp_count = offsets[i];
			offsets[i] = total;
			total += temp_count;
		}

		// @Note(tkap, 02/07/2024): Modify the new list
		for(u32 i = 0; i < count; i += 1) {
			u32 val = get_radix(source[i]);
			u32 mask = 0xff << index_i;
			u32 radix_val = (val & mask) >> index_i;

			u32 destination_index = offsets[radix_val];
			offsets[radix_val] += 1;
			destination[destination_index] = source[i];
		}

		t* swap_temp = destination;
		destination = source;
		source = swap_temp;
	}
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

func s_active_sound* find_playing_sound(e_sound id)
{
	foreach_ptr(sound_i, sound, g_platform_data->active_sound_arr) {
		if(sound->loaded_sound_id == id) {
			return sound;
		}
	}
	return null;
}

func void do_lerpable_snap(s_lerpable* lerpable, float dt, float max_diff)
{
	lerpable->curr = lerp_snap(lerpable->curr, lerpable->target, dt, max_diff);
}

func float get_player_speed()
{
	s_stats stats = get_stats();
	float result = c_base_player_speed;
	if(game->fast_player_speed) {
		result *= 10;
	}
	result *= 1.0f + stats.arr[e_stat_player_movement_speed] / 100.0f;
	return result;
}

func void do_basic_render_flush(s_m4 ortho, int render_pass_index, s_m4 view_inv)
{
	s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
	data.projection = ortho;
	data.blend_mode = e_blend_mode_normal;
	data.depth_mode = e_depth_mode_no_read_no_write;
	render_flush(data, true, render_pass_index);
}

func s_rect get_chunk_rect(s_v2i index)
{
	assert(index.x >= 0);
	assert(index.x < c_chunk_count);
	assert(index.y >= 0);
	assert(index.y < c_chunk_count);

	s_rect result = zero;
	result.pos = v2(
		index.x * c_chunk_size * c_tile_size,
		index.y * c_chunk_size * c_tile_size
	);
	result.size = v2(c_chunk_size * c_tile_size);
	return result;
}

func b8 is_chunk_unlocked(int x, int y)
{
	if(x < 0 || x >= c_chunk_count || y < 0 || y >= c_chunk_count) {
		return false;
	}
	b8 result = game->soft_data.purchased_chunk_arr[y][x];
	return result;
}

func b8 is_chunk_unlocked_v2i(s_v2i index)
{
	b8 result = is_chunk_unlocked(index.x, index.y);
	return result;
}

func b8 are_any_adjacent_chunks_unlocked(s_v2i index)
{
	s_v2i offset_arr[] = {
		v2i(0, -1), v2i(-1, 0), v2i(1, 0), v2i(0, 1)
	};
	b8 result = false;
	for(int i = 0; i < 4; i += 1) {
		if(is_chunk_unlocked_v2i(index + offset_arr[i])) {
			result = true;
			break;
		}
	}
	return result;
}

func void unlock_chunk_v2i(s_v2i index)
{
	assert(index.x >= 0);
	assert(index.x < c_chunk_count);
	assert(index.y >= 0);
	assert(index.y < c_chunk_count);
	assert(!is_chunk_unlocked_v2i(index));
	game->soft_data.purchased_chunk_arr[index.y][index.x] = true;
	if(index != v2i(c_starting_chunk, c_starting_chunk)) {
		set_maybe_if_invalid(&game->tutorial.unlocked_chunk, game->render_time);
	}
}

func int get_chunk_unlock_cost(s_v2i index)
{
	assert(index.x >= 0);
	assert(index.x < c_chunk_count);
	assert(index.y >= 0);
	assert(index.y < c_chunk_count);
	assert(!is_chunk_unlocked_v2i(index));
	int distance = abs(index.x - c_starting_chunk) + abs(index.y - c_starting_chunk);
	int result = distance * 50;
	return result;
}

func s_v2 get_chunk_center(s_v2i index)
{
	s_rect rect = get_chunk_rect(index);
	s_v2 result = rect.pos + rect.size * 0.5f;
	return result;
}

func b8 can_afford(int currency, int cost)
{
	b8 result = currency >= cost;
	return result;
}

func void add_currency(int currency)
{
	game->soft_data.currency += currency;
	assert(game->soft_data.currency >= 0);
}

func void add_raw_currency(int currency)
{
	game->soft_data.raw_currency += currency;
	assert(game->soft_data.raw_currency >= 0);
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

func s_v2i chunk_index_from_pos(s_v2 pos)
{
	int size = c_chunk_size * c_tile_size;
	s_v2i result = v2i(
		floorfi(pos.x / size),
		floorfi(pos.y / size)
	);
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

func void place_machine(s_v2i tile_index, e_machine machine)
{
	assert(game->soft_data.machine_arr[tile_index.y][tile_index.x] == e_machine_none);
	game->soft_data.machine_arr[tile_index.y][tile_index.x] = machine;
	game->soft_data.machine_count_arr[machine] += 1;
	if(machine == e_machine_collector_1) {
		set_maybe_if_invalid(&game->tutorial.placed_collector, game->render_time);
	}
	else if(machine == e_machine_processor_1) {
		set_maybe_if_invalid(&game->tutorial.placed_processor, game->render_time);
	}
	else if(machine == e_machine_research_1) {
		set_maybe_if_invalid(&game->tutorial.placed_researcher, game->render_time);
	}
}

func e_place_result can_we_place_machine(s_v2 player_pos, s_v2i chunk_index, s_v2i tile_index, e_machine machine, int currency)
{
	if(!is_chunk_unlocked_v2i(chunk_index)) {
		return e_place_result_chunk_locked;
	}

	{
		int dist = get_tile_distance_from_player_to_machine(player_pos, tile_index, machine);
		b8 in_range = dist <= get_player_tile_reach();
		if(!in_range) {
			return e_place_result_out_of_reach;
		}
	}

	e_place_result result = e_place_result_success;

	int x_offset = tile_index.x - (c_biggest_machine_tile_size - 1);
	int y_offset = tile_index.y - (c_biggest_machine_tile_size - 1);

	s_array2d<b8, c_biggest_machine_tile_size * 3, c_biggest_machine_tile_size * 3> mask = zero;
	s_soft_game_data* soft_data = &game->soft_data;

	{
		int min_x = at_least(0, tile_index.x - (c_biggest_machine_tile_size - 1));
		int min_y = at_least(0, tile_index.y - (c_biggest_machine_tile_size - 1));
		int max_x = at_most(c_max_tiles - 1, tile_index.x + (c_biggest_machine_tile_size - 1));
		int max_y = at_most(c_max_tiles - 1, tile_index.y + (c_biggest_machine_tile_size - 1));
		for(int y = min_y; y <= max_y; y += 1) {
			for(int x = min_x; x <= max_x; x += 1) {
				e_machine placed_machine = soft_data->machine_arr[y][x];
				int placed_machine_size = g_machine_data[placed_machine].size;
				for(int yy = 0; yy < placed_machine_size; yy += 1) {
					for(int xx = 0; xx < placed_machine_size; xx += 1) {
						mask[y + yy - y_offset][x + xx - x_offset] = true;
					}
				}
			}
		}
	}
	int to_place_size = g_machine_data[machine].size;
	int max_x = at_most(c_max_tiles - 1, tile_index.x + to_place_size - 1);
	int max_y = at_most(c_max_tiles - 1, tile_index.y + to_place_size - 1);
	b8 collision = false;
	b8 touches_resource = false;
	b8 touches_unlocked_chunk = false;
	for(int y = tile_index.y; y <= max_y; y += 1) {
		for(int x = tile_index.x; x <= max_x; x += 1) {
			s_v2i temp_chunk_index = chunk_index_from_tile_index(v2i(x, y));
			if(mask[y - y_offset][x - x_offset]) {
				collision = true;
			}
			if(!is_chunk_unlocked_v2i(temp_chunk_index)) {
				touches_unlocked_chunk = true;
			}

			if(machine == e_machine_collector_1) {
				if(soft_data->natural_terrain_arr[y][x] == e_tile_resource_1) {
					touches_resource = true;
				}
			}
			else if(machine == e_machine_collector_2) {
				if(soft_data->natural_terrain_arr[y][x] == e_tile_resource_2) {
					touches_resource = true;
				}
			}
			else if(machine == e_machine_collector_3) {
				if(soft_data->natural_terrain_arr[y][x] == e_tile_resource_3) {
					touches_resource = true;
				}
			}
		}
	}

	if(g_machine_data[machine].requires_resource && !touches_resource) {
		result = e_place_result_requires_resource;
	}
	else if(touches_unlocked_chunk) {
		result = e_place_result_chunk_locked;
	}
	else if(collision) {
		result = e_place_result_occupied;
	}
	else if(!can_afford(currency, get_machine_cost(machine))) {
		return e_place_result_currency;
	}

	return result;
}

func s_v2i chunk_index_from_tile_index(s_v2i tile_index)
{
	s_v2i result = v2i(
		tile_index.x / c_chunk_size,
		tile_index.y / c_chunk_size
	);
	return result;
}

func b8 is_machine_unlocked(e_machine machine)
{
	s_soft_game_data* soft_data = &game->soft_data;
	b8 result = false;
	switch(machine) {

		xcase e_machine_collector_1: { result = true; }
		xcase e_machine_collector_2: {
			if(soft_data->research_completed_timestamp_arr[e_research_collector_2].valid) {
				result = true;
			}
		}
		xcase e_machine_collector_3: {
			if(soft_data->research_completed_timestamp_arr[e_research_collector_3].valid) {
				result = true;
			}
		}

		xcase e_machine_pure_collector_1: {
			if(soft_data->research_completed_timestamp_arr[e_research_pure_collector_1].valid) {
				result = true;
			}
		}
		xcase e_machine_pure_collector_2: {
			if(soft_data->research_completed_timestamp_arr[e_research_pure_collector_2].valid) {
				result = true;
			}
		}
		xcase e_machine_pure_collector_3: {
			if(soft_data->research_completed_timestamp_arr[e_research_pure_collector_3].valid) {
				result = true;
			}
		}

		xcase e_machine_processor_1: { result = true; }
		xcase e_machine_processor_2: {
			if(soft_data->research_completed_timestamp_arr[e_research_processor_2].valid) {
				result = true;
			}
		}

		xcase e_machine_processor_3: {
			if(soft_data->research_completed_timestamp_arr[e_research_processor_3].valid) {
				result = true;
			}
		}

		xcase e_machine_research_1: { result = true; }

		xcase e_machine_research_2: {
			if(soft_data->research_completed_timestamp_arr[e_research_research_2].valid) {
				result = true;
			}
		}
		xcase e_machine_research_3: {
			if(soft_data->research_completed_timestamp_arr[e_research_research_3].valid) {
				result = true;
			}
		}

		break; invalid_default_case;
	}
	return result;
}

func b8 is_resource_tile(e_tile tile)
{
	b8 result = tile == e_tile_resource_1 || tile == e_tile_resource_2 || tile == e_tile_resource_3;
	return result;
}

func int get_machine_cost(e_machine machine)
{
	int result = g_machine_data[machine].cost;
	return result;
}

func void remove_machine(s_v2i tile_index)
{
	e_machine machine = game->soft_data.machine_arr[tile_index.y][tile_index.x];
	assert(machine != e_machine_none);
	game->soft_data.machine_count_arr[machine] -= 1;
	game->soft_data.machine_arr[tile_index.y][tile_index.x] = e_machine_none;
	assert(game->soft_data.machine_count_arr[machine] >= 0);
	set_maybe_if_invalid(&game->tutorial.deleted_machine, game->render_time);
}

func void sell_machine(s_v2i tile_index, e_machine machine)
{
	remove_machine(tile_index);
	// @Fixme(tkap, 05/10/2025): should be currency spent, not current cost
	add_currency(get_machine_cost(machine));
	play_sound(e_sound_key, zero);
}

func s_len_str get_research_tooltip(e_research research)
{
	s_len_str result = zero;
	s_research_data data = g_research_data[research];
	switch(research) {

		case e_research_player_speed_1:
		case e_research_player_speed_2:
		case e_research_player_speed_3:
		case e_research_player_speed_4: {
			result = format_text("+%.0f%% player movement speed", data.value);
		} break;

		case e_research_player_tile_reach_1:
		case e_research_player_tile_reach_2:
		case e_research_player_tile_reach_3:
		case e_research_player_tile_reach_4: {
			result = format_text("+%.0f player tile reach", data.value);
		} break;

		xcase e_research_collector_2: {
			result = format_text("Unlocks Collector Mk2");
		}
		xcase e_research_collector_3: {
			result = format_text("Unlocks Collector Mk3");
		}

		xcase e_research_pure_collector_1: {
			result = format_text("Unlocks Pure collector Mk1");
		}
		xcase e_research_pure_collector_2: {
			result = format_text("Unlocks Pure collector Mk2");
		}
		xcase e_research_pure_collector_3: {
			result = format_text("Unlocks Pure collector Mk3");
		}

		xcase e_research_processor_2: {
			result = format_text("Unlocks Processor Mk2");
		}
		xcase e_research_processor_3: {
			result = format_text("Unlocks Processor Mk3");
		}

		xcase e_research_research_2: {
			result = format_text("Unlocks Researcher Mk2");
		}
		xcase e_research_research_3: {
			result = format_text("Unlocks Researcher Mk3");
		} break;

		case e_research_collector_speed_1:
		case e_research_collector_speed_2:
		case e_research_collector_speed_3: {
			result = format_text("+%.0f%% collector speed", data.value);
		} break;

		case e_research_processor_speed_1:
		case e_research_processor_speed_2:
		case e_research_processor_speed_3: {
			result = format_text("+%.0f%% processor speed", data.value);
		} break;

		case e_research_research_speed_1:
		case e_research_research_speed_2:
		case e_research_research_speed_3: {
			result = format_text("+%.0f%% researcher speed", data.value);
		} break;

		xcase e_research_win: {
			result = format_text("This is it. YOU ARE THE COLLECTOR!");
		}

		break; invalid_default_case;
	}
	result = format_text("%.*s\nCost: %i", expand_str(result), data.cost);
	return result;
}

func s_len_str get_machine_tooltip(e_machine machine)
{
	s_machine_data data = g_machine_data[machine];
	s_str_builder<2048> builder;
	builder.count = 0;
	builder_add(&builder, "%.*s\n", expand_str(data.name));
	builder_add_separator(&builder);
	switch(machine) {
		xcase e_machine_collector_1: {
			builder_add(&builder, "Place on rockum to collect raw energy");
		}
		xcase e_machine_collector_2: {
			builder_add(&builder, "Place on warum to collect a lot of raw energy\n(5x faster)");
		}
		xcase e_machine_collector_3: {
			builder_add(&builder, "Place on starum to collect a huge amount raw energy\n(25x faster)");
		}
		xcase e_machine_processor_1: {
			builder_add(&builder, "Convert raw energy into pure energy");
		}
		xcase e_machine_processor_2: {
			builder_add(&builder, "Convert raw energy into pure energy, but faster\n(5x faster)");
		}
		xcase e_machine_processor_3: {
			builder_add(&builder, "Convert raw energy into pure energy, but even faster\n(25x faster)");
		}
		xcase e_machine_research_1: {
			builder_add(&builder, "Research new technology using pure energy");
		}
		xcase e_machine_research_2: {
			builder_add(&builder, "Research new technology, but faster\n(5x faster)");
		}
		xcase e_machine_research_3: {
			builder_add(&builder, "Research new technology, but even faster\n(25x faster)");
		}
		xcase e_machine_pure_collector_1: {
			builder_add(&builder, "Turn air into pure energy, very slowly");
		}
		xcase e_machine_pure_collector_2: {
			builder_add(&builder, "Turn air into pure energy, not as slow\n(5x faster)");
		}
		xcase e_machine_pure_collector_3: {
			builder_add(&builder, "Turn air into pure energy\n(25x faster)");
		}
		break; invalid_default_case;
	}
	builder_add(&builder, "\n");
	builder_add_separator(&builder);
	builder_add(&builder, "Cost: %i", data.cost);
	s_len_str result = builder_to_len_str_alloc(&builder, &game->render_frame_arena);
	return result;
}

template <int n>
func void builder_add_separator(s_str_builder<n>* builder)
{
	builder_add(builder, "$$888888--------------------$.\n");
}

func void draw_selector_center(s_v2 pos, s_v2 size, float brightness, int render_pass_index)
{
	s_v4 color = make_rrr(brightness);
	float thick0 = 0.15f;
	float thick1 = thick0 * 0.5f;
	draw_rect_topleft(pos + size * v2(-0.5f, -0.5f), size * v2(thick0, thick1), color, render_pass_index);
	draw_rect_topleft(pos + size * v2(-0.5f, -0.5f), size * v2(thick1, thick0), color, render_pass_index);

	draw_rect_topleft(pos + size * v2(0.5f, -0.5f), size * v2(-thick0, thick1), color, render_pass_index);
	draw_rect_topleft(pos + size * v2(0.5f, -0.5f), size * v2(-thick1, thick0), color, render_pass_index);

	draw_rect_topleft(pos + size * v2(-0.5f, 0.5f), size * v2(thick0, -thick1), color, render_pass_index);
	draw_rect_topleft(pos + size * v2(-0.5f, 0.5f), size * v2(thick1, -thick0), color, render_pass_index);

	draw_rect_topleft(pos + size * v2(0.5f, 0.5f), size * v2(-thick0, -thick1), color, render_pass_index);
	draw_rect_topleft(pos + size * v2(0.5f, 0.5f), size * v2(-thick1, -thick0), color, render_pass_index);
}

func int get_tile_distance_from_player_to_machine(s_v2 player_pos, s_v2i machine_tile_index, e_machine machine)
{
	int machine_size = g_machine_data[machine].size;
	s_v2i player_tile_index = tile_index_from_pos(player_pos);
	s_v2i arr[] = {
		machine_tile_index, machine_tile_index + v2i(machine_size - 1, 0),
		machine_tile_index + v2i(0, machine_size - 1), machine_tile_index + v2i(machine_size - 1, machine_size - 1),
	};
	int smallest_dist = 9999999;
	for(int i = 0; i < array_count(arr); i += 1) {
		int dist = abs(player_tile_index.x - arr[i].x) + abs(player_tile_index.y - arr[i].y);
		if(dist < smallest_dist) {
			smallest_dist = dist;
		}
	}
	return smallest_dist;
}

func int get_player_tile_reach()
{
	s_stats stats = get_stats();
	int result = 6;
	result += roundfi(stats.arr[e_stat_player_tile_reach]);
	if(game->player_super_reach) {
		result += 100000;
	}
	return result;
}

func void add_non_spammy_message(s_v2 pos, s_len_str text)
{
	float passed = game->render_time - game->soft_data.last_non_spammy_timestamp;
	if(passed >= 0.5f) {
		game->soft_data.last_non_spammy_timestamp = game->render_time;
		s_entity fct = zero;
		fct.prev_pos = pos;
		fct.pos = pos;
		fct.spawn_timestamp = game->render_time;
		fct.text = text;
		entity_manager_add_if_not_full(&game->soft_data.entity_arr, e_entity_fct, fct);
	}
}

func s_len_str str_from_place_result(e_place_result place_result)
{
	s_len_str result = zero;
	switch(place_result) {
		xcase e_place_result_currency: {
			result = format_text("Not enough pure energy");
		}
		xcase e_place_result_requires_resource: {
			result = format_text("Must be built on the appropriate resource");
		}
		xcase e_place_result_out_of_reach: {
			result = format_text("Out of reach");
		}
		// xcase e_place_result_occupied: {
		// 	result = format_text("Occupied");
		// }
		xcase e_place_result_chunk_locked: {
			result = format_text("This area hasn't been revealed");
		}
		break; default: {}
	}
	return result;
}

func s_stats get_stats()
{
	s_stats result = zero;
	for_enum(research_i, e_research) {
		s_research_data data = g_research_data[research_i];
		if(data.target_stat.valid && game->soft_data.research_completed_timestamp_arr[research_i].valid) {
			result.arr[data.target_stat.value] += data.value;
		}
	}
	return result;
}

func b8 are_research_requirements_satisfied(e_research research)
{
	s_research_data data = g_research_data[research];
	b8 result = true;
	for(int i = 0; i < data.requirement_count; i += 1) {
		e_research requirement = data.requirement_arr[i];
		if(!game->soft_data.research_completed_timestamp_arr[requirement].valid) {
			result = false;
			break;
		}
	}
	return result;
}

func void draw_undiscovered_slot(s_v2 pos, s_v2 rect_size, float flash)
{
	draw_atlas_topleft(game->atlas, pos, rect_size, v2i(0, 0), make_rrr(0.33f * flash), 1);
	draw_atlas_topleft(game->atlas, pos, rect_size, v2i(1, 0), make_rrr(1.0f * flash), 2);
}

func void draw_game_name()
{
	s_v2 topleft = v2(84, 3108);
	s_v2 bottomright = v2(1464, 3423);
	s_v2 size = bottomright - topleft;
	s_v2 aspect_ratio = v2(1, size.y / size.x);
	s_instance_data data = zero;
	data.model = m4_translate(v3(wxy(0.5f, 0.2f), 0.0f));
	data.model = m4_multiply(data.model, m4_scale(v3(v2(512) * aspect_ratio, 1)));
	data.color = make_rrr(1);
	data.uv_min = v2(topleft.x / game->superku.texture_size.x, topleft.y / game->superku.texture_size.y);
	data.uv_max = v2(bottomright.x / game->superku.texture_size.x, bottomright.y / game->superku.texture_size.y);
	add_to_render_group(data, e_shader_flat, e_texture_superku, e_mesh_quad, 0);
}

func void draw_menu_background(s_m4 ortho, s_m4 view_inv)
{
	draw_ground(ortho, view_inv);

	e_machine machine = e_machine_collector_1;
	int frame_index = roundfi(game->render_time * 8) % g_machine_data[machine].frame_count;
	s_v2i atlas_index = g_machine_data[machine].frame_arr[frame_index];
	s_v2 size = v2(400);
	draw_atlas(game->superku, wxy(0.1f, 0.15f), size, atlas_index, make_rrr(1), 0);
	draw_atlas(game->superku, wxy(0.9f, 0.15f), size, atlas_index, make_rrr(1), 0);
	do_basic_render_flush(ortho, 0, view_inv);
}

func void draw_ground(s_m4 ortho, s_m4 view_inv)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(c_world_center, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(c_world_size, 1)));
	data.color = make_rrr(1);
	add_to_render_group(data, e_shader_ground, e_texture_white, e_mesh_quad, 0);
	do_basic_render_flush(ortho, 0, view_inv);
}
