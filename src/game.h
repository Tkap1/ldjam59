
#include "gen_meta/game.h.enums"

#if defined(__EMSCRIPTEN__)

#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
X(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced) \
X(PFNGLUNIFORM1FVPROC, glUniform1fv) \
X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
X(PFNGLUNIFORM3FVPROC, glUniform3fv) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
X(PFNGLDELETESHADERPROC, glDeleteShader) \
X(PFNGLUNIFORM1IPROC, glUniform1i) \
X(PFNGLUNIFORM1FPROC, glUniform1f) \
X(PFNGLDETACHSHADERPROC, glDetachShader) \
X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers) \
X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate) \
X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \

#else
#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
X(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced) \
X(PFNGLUNIFORM1FVPROC, glUniform1fv) \
X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
X(PFNGLUNIFORM3FVPROC, glUniform3fv) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
X(PFNGLDELETESHADERPROC, glDeleteShader) \
X(PFNGLUNIFORM1IPROC, glUniform1i) \
X(PFNGLUNIFORM1FPROC, glUniform1f) \
X(PFNGLDETACHSHADERPROC, glDetachShader) \
X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers) \
X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate) \
X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \

#endif

#define X(type, name) static type name = NULL;
m_gl_funcs
#undef X

#define invalid_default_case default: { assert(!"Invalid default case"); }
#define invalid_else else { assert(!"Invalid else"); }

#if defined(_WIN32) && defined(m_debug)
#define m_dll_export extern "C" __declspec(dllexport)
#else // _WIN32
#define m_dll_export
#endif

enum e_game_state1
{
	e_game_state1_default,
	e_game_state1_defeat,
};

struct s_lerpable
{
	float curr;
	float target;
};

enum e_game_state0
{
	e_game_state0_main_menu,
	e_game_state0_leaderboard,
	e_game_state0_win_leaderboard,
	e_game_state0_options,
	e_game_state0_pause,
	e_game_state0_play,
	e_game_state0_input_name,
};

struct s_key_event
{
	b8 went_down;
	int modifiers;
	int key;
};

struct s_key_state
{
	b8 is_down;
	int half_transition_count;
};

template <int n>
struct s_input_str
{
	b8 visual_pos_initialized;
	s_v2 cursor_visual_pos;
	float last_edit_time;
	float last_action_time;
	s_maybe<int> cursor;
	s_str_builder<n> str;
};

struct s_input_name_state
{
	s_input_str<16> name;
	s_str_builder<64> error_str;
};

struct s_timed_msg
{
	float spawn_timestamp;
	s_v2 pos;
	s_str_builder<64> builder;
};

struct s_button_data
{
	b8 disabled;
	b8 mute_click_sound;
	int render_pass_index;
	float font_size = 32;
	s_len_str tooltip;
	s_v4 button_color = {0.25f, 0.25f, 0.25f, 1.0f};
};

enum e_button_result
{
	e_button_result_none,
	e_button_result_left_click,
	e_button_result_right_click,
};

enum e_place_result
{
	e_place_result_success,
	e_place_result_currency,
	e_place_result_requires_resource,
	e_place_result_out_of_reach,
	e_place_result_occupied,
	e_place_result_chunk_locked,
};

struct s_entity
{
	int id;
	float timer;
	s_v2 pos;
	s_v2 prev_pos;
	float spawn_timestamp;
	float duration;
	union {

		// @Note(tkap, 04/10/2025): Player
		struct {
			b8 walking;
			int animation_index;
			float animation_time;
			float rotation;
			s_v2 last_dir;
		};

		// @Note(tkap, 31/07/2025): Emitter
		struct {
			s_particle_emitter_a emitter_a;
			s_particle_emitter_b emitter_b;
		};

		// @Note(tkap, 31/07/2025): FCT
		struct {
			s_len_str text;
		};
	};
};

struct s_frame_data
{
	int lives_to_lose;
};

struct s_hold_input
{
	b8 left;
	b8 right;
	b8 up;
	b8 down;
};

struct s_press_input
{
	b8 q;
};

enum e_tile : u8
{
	e_tile_none,
	e_tile_resource_1,
	e_tile_resource_2,
	e_tile_resource_3,
	e_tile_count,
};

global constexpr s_v2i c_tile_atlas_index[] = {
	zero,
	{5, 6},
	{3, 6},
	{4, 6},
};

data_enum(e_machine,
	s_machine_data
	g_machine_data

	none {

	}

	collector_1 {
		.name = S("Collector Mk1"),
		.requires_resource = true,
		.size = 2,
		.cost = 10,
		.frame_count = 4,
		.frame_arr = {
			{0, 0}, {1, 0}, {2, 0}, {3, 0},
		},
	}

	collector_2 {
		.name = S("Collector Mk2"),
		.requires_resource = true,
		.size = 2,
		.cost = 10 * 10,
		.frame_count = 4,
		.frame_arr = {
			{0, 1}, {1, 1}, {2, 1}, {3, 1},
		},
	}

	collector_3 {
		.name = S("Collector Mk3"),
		.requires_resource = true,
		.size = 2,
		.cost = 10 * 10 * 10,
		.frame_count = 4,
		.frame_arr = {
			{0, 2}, {1, 2}, {2, 2}, {3, 2},
		},
	}

	processor_1 {
		.name = S("Processor Mk1"),
		.size = 3,
		.cost = 20,
		.frame_count = 1,
		.frame_arr = {
			{5, 0},
		},
	}

	processor_2 {
		.name = S("Processor Mk2"),
		.size = 3,
		.cost = 20 * 5,
		.frame_count = 1,
		.frame_arr = {
			{5, 1},
		},
	}

	processor_3 {
		.name = S("Processor Mk3"),
		.size = 3,
		.cost = 20 * 5 * 5,
		.frame_count = 1,
		.frame_arr = {
			{5, 2},
		},
	}

	research_1 {
		.name = S("Researcher Mk1"),
		.size = 3,
		.cost = 30,
		.frame_count = 8,
		.frame_arr = {
			{0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {6, 3}, {7, 3},
		},
	}

	research_2 {
		.name = S("Researcher Mk2"),
		.size = 3,
		.cost = 30 * 5,
		.frame_count = 8,
		.frame_arr = {
			{0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4}, {7, 4},
		},
	}

	research_3 {
		.name = S("Researcher Mk3"),
		.size = 3,
		.cost = 30 * 5 * 5,
		.frame_count = 8,
		.frame_arr = {
			{0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5}, {6, 5}, {7, 5},
		},
	}

	pure_collector_1 {
		.name = S("Pure collector Mk1"),
		.size = 1,
		.cost = 20,
		.frame_count = 1,
		.frame_arr = {
			{7, 0},
		},
	}

	pure_collector_2 {
		.name = S("Pure collector Mk2"),
		.size = 1,
		.cost = 20 * 5,
		.frame_count = 1,
		.frame_arr = {
			{7, 1},
		},
	}

	pure_collector_3 {
		.name = S("Pure collector Mk3"),
		.size = 1,
		.cost = 20 * 5 * 5,
		.frame_count = 1,
		.frame_arr = {
			{7, 2},
		},
	}

)

struct s_machine_data
{
	s_len_str name;
	b8 requires_resource;
	int size;
	int cost;
	int frame_count;
	s_v2i frame_arr[8];
};

enum e_stat
{
	e_stat_player_movement_speed,
	e_stat_player_tile_reach,
	e_stat_collector_speed,
	e_stat_processor_speed,
	e_stat_research_speed,
	e_stat_count,
};

struct s_stats
{
	float arr[e_stat_count];
};

data_enum(e_research,
	s_research_data
	g_research_data

	player_speed_1 {
		.cost = 20,
		.target_stat = maybe(e_stat_player_movement_speed),
		.value = 20
	}
	player_speed_2 {
		.requirement_count = 1,
		.requirement_arr = {e_research_player_speed_1},
		.cost = 200,
		.target_stat = maybe(e_stat_player_movement_speed),
		.value = 60,
	}
	player_speed_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_player_speed_2},
		.cost = 1000,
		.target_stat = maybe(e_stat_player_movement_speed),
		.value = 100,
	}
	player_speed_4 {
		.requirement_count = 1,
		.requirement_arr = {e_research_player_speed_3},
		.cost = 3000,
		.target_stat = maybe(e_stat_player_movement_speed),
		.value = 300,
	}

	player_tile_reach_1 {
		.cost = 20,
		.target_stat = maybe(e_stat_player_tile_reach),
		.value = 2
	}
	player_tile_reach_2 {
		.requirement_count = 1,
		.requirement_arr = {e_research_player_tile_reach_1},
		.cost = 200,
		.target_stat = maybe(e_stat_player_tile_reach),
		.value = 4,
	}
	player_tile_reach_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_player_tile_reach_2},
		.cost = 1000,
		.target_stat = maybe(e_stat_player_tile_reach),
		.value = 10,
	}
	player_tile_reach_4 {
		.requirement_count = 1,
		.requirement_arr = {e_research_player_tile_reach_3},
		.cost = 2000,
		.target_stat = maybe(e_stat_player_tile_reach),
		.value = 50,
	}

	pure_collector_1 {
		.cost = 1000,
	}

	collector_2 {
		.cost = 1000,
	}

	processor_2 {
		.cost = 1000,
	}

	collector_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_collector_2},
		.cost = 100000,
	}

	processor_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_processor_2},
		.cost = 100000,
	}

	research_2 {
		.cost = 2000,
	}
	research_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_research_2},
		.cost = 200000,
	}

	pure_collector_2 {
		.requirement_count = 1,
		.requirement_arr = {e_research_pure_collector_1},
		.cost = 1000,
	}
	pure_collector_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_pure_collector_2},
		.cost = 10000,
	}

	collector_speed_1 {
		.cost = 100,
		.target_stat = maybe(e_stat_collector_speed),
		.value = 25,
	}
	collector_speed_2 {
		.requirement_count = 1,
		.requirement_arr = {e_research_collector_speed_1},
		.cost = 1000,
		.target_stat = maybe(e_stat_collector_speed),
		.value = 50,
	}
	collector_speed_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_collector_speed_2},
		.cost = 20000,
		.target_stat = maybe(e_stat_collector_speed),
		.value = 100,
	}

	processor_speed_1 {
		.cost = 100,
		.target_stat = maybe(e_stat_processor_speed),
		.value = 25,
	}
	processor_speed_2 {
		.requirement_count = 1,
		.requirement_arr = {e_research_processor_speed_1},
		.cost = 1000,
		.target_stat = maybe(e_stat_processor_speed),
		.value = 50,
	}
	processor_speed_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_processor_speed_2},
		.cost = 20000,
		.target_stat = maybe(e_stat_processor_speed),
		.value = 100,
	}

	research_speed_1 {
		.cost = 100,
		.target_stat = maybe(e_stat_research_speed),
		.value = 25,
	}
	research_speed_2 {
		.requirement_count = 1,
		.requirement_arr = {e_research_research_speed_1},
		.cost = 1000,
		.target_stat = maybe(e_stat_research_speed),
		.value = 50,
	}
	research_speed_3 {
		.requirement_count = 1,
		.requirement_arr = {e_research_research_speed_2},
		.cost = 20000,
		.target_stat = maybe(e_stat_research_speed),
		.value = 100,
	}

	win {
		.requirement_count = 2,
		.requirement_arr = {e_research_collector_3, e_research_processor_3},
		.cost = 2000000,
	}
)

struct s_research_data
{
	int requirement_count;
	e_research requirement_arr[4];
	int cost;
	s_maybe<e_stat> target_stat;
	float value;
};

struct s_soft_game_data
{
	float collector_timer;
	float pure_collector_timer;
	float processor_timer;
	float research_timer;

	float last_step_sound_timestamp;

	float last_non_spammy_timestamp;

	s_maybe<float> open_inventory_timestamp;
	s_hold_input hold_input;
	s_press_input press_input;
	int frames_to_freeze;
	s_frame_data frame_data;
	b8 tried_to_submit_score;
	int update_count;
	float shake_intensity;
	float start_screen_shake_timestamp;
	float start_restart_timestamp;
	s_list<s_particle, 65536> particle_arr;
	float wanted_zoom;
	float zoom;

	s_entity_manager<s_entity, c_max_entities> entity_arr;

	s_list<s_timed_msg, 8> timed_msg_arr;

	int currency;
	int raw_currency;

	e_tile natural_terrain_arr[c_max_tiles][c_max_tiles];
	e_machine machine_arr[c_max_tiles][c_max_tiles];
	b8 purchased_chunk_arr[c_chunk_count][c_chunk_count];
	s_maybe<e_machine> machine_to_place;
	int machine_count_arr[e_machine_count];
	s_maybe<e_research> current_research;
	s_maybe<float> research_completed_timestamp_arr[e_research_count];
	int spent_on_research_arr[e_research_count];
	s_v2i shift_start;
	float machine_animation_time;
	float researcher_animation_time;
};

struct s_hard_game_data
{
	s_state state1;
	int update_count;
};

// struct s_light
// {
// 	s_v2 pos;
// 	float radius;
// 	float smoothness;
// 	s_v4 color;
// };

struct s_render_pass
{
	int render_instance_count[e_shader_count][e_texture_count][e_mesh_count];
	int render_instance_max_elements[e_shader_count][e_texture_count][e_mesh_count];
	int render_group_index_arr[e_shader_count][e_texture_count][e_mesh_count];
	void* render_instance_arr[e_shader_count][e_texture_count][e_mesh_count];
	s_list<s_render_group, 128> render_group_arr;
};

struct s_tutorial
{
	s_maybe<float> moved;
	s_maybe<float> zoomed;
	s_maybe<float> opened_inventory;
	s_maybe<float> placed_collector;
	s_maybe<float> placed_processor;
	s_maybe<float> started_a_research;
	s_maybe<float> placed_researcher;
	s_maybe<float> deleted_machine;
	s_maybe<float> used_q;
	s_maybe<float> used_shift;
	s_maybe<float> unlocked_chunk;
	s_maybe<float> tutorial_end;
};

struct s_game
{
	s_tutorial tutorial;
	b8 disable_walk_sound;
	b8 fast_player_speed;
	b8 player_super_reach;
	b8 free_research;
	b8 disable_damage_numbers;
	b8 disable_gold_numbers;
	b8 disable_lights;
	float completed_dash_tutorial_timestamp;
	s_lerpable music_speed;
	s_lerpable music_volume;
	int next_entity_id;
	#if defined(m_debug)
	b8 cheat_menu_enabled;
	#endif
	b8 reload_shaders;
	b8 any_key_pressed;
	s_linear_arena arena;
	s_linear_arena update_frame_arena;
	s_linear_arena render_frame_arena;
	s_circular_arena circular_arena;
	u32 ubo;
	s_texture texture_arr[e_texture_count];
	s_mesh mesh_arr[e_mesh_count];
	s_shader shader_arr[e_shader_count];
	float render_time;
	float update_time;
	f64 accumulator;
	f64 time_before;
	u32 curr_fbo;
	int speed_index;
	s_font font;
	s_rng rng;
	float speed;
	s_input_name_state input_name_state;
	int num_times_we_attacked_an_enemy;
	int num_times_we_dashed;

	s_atlas atlas;
	s_atlas superku;

	#if defined(m_winhttp)
	HINTERNET session;
	#endif

	s_len_str tooltip;

	s_fbo light_fbo;

	// s_list<s_light, 256> multiplicative_light_arr;
	// s_list<s_light, 256> additive_light_arr;

	b8 disable_music;
	b8 hide_timer;
	b8 turn_off_all_sounds;

	int update_count_at_win_time;

	s_state state0;

	s_list<s_leaderboard_entry, c_max_leaderboard_entries> leaderboard_arr;
	b8 leaderboard_received;
	int curr_leaderboard_page;

	s_str_builder<256> leaderboard_session_token;
	s_str_builder<256> leaderboard_public_uid;
	s_str_builder<256> leaderboard_nice_name;
	s_str_builder<256> leaderboard_player_identifier;
	int leaderboard_player_id;

	s_key_state input_arr[c_max_keys];
	s_list<s_key_event, 128> key_events;
	s_list<char, 128> char_events;

	b8 do_soft_reset;
	b8 do_hard_reset;

	s_hard_game_data hard_data;
	s_soft_game_data soft_data;

	s_render_pass render_pass_arr[c_render_pass_count];
};


#include "gen_meta/game.cpp.funcs"
#include "gen_meta/game.h.globals"