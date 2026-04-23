
#include "gen_meta/game.h.enums"

global constexpr s_v2 c_player_size = {0.2f, 0.5f};
global constexpr float c_action_cooldown = 0.25f;
global constexpr int c_map_width = 5;
global constexpr int c_map_height = 128;
global constexpr int c_map_version = 1;
global constexpr int c_map_count = 20;
global constexpr float c_player_range = 1.5f;
global constexpr float c_enemy_range = 0.2f;
global constexpr float c_teleport_cooldown = 99999;
global constexpr float c_win_transition_duration = c_action_cooldown * 2;
global constexpr s_v2 c_enemy_size = {0.5f, 0.5f};

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
#define invalid_default_xcase break; default: { assert(!"Invalid default case"); }
#define invalid_else else { assert(!"Invalid else"); }

#if defined(_WIN32) && defined(m_debug)
#define m_dll_export extern "C" __declspec(dllexport)
#else // _WIN32
#define m_dll_export
#endif

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

enum e_pickup
{
	e_pickup_end,
	e_pickup_spike,
	e_pickup_teleport,
	e_pickup_plus_2_actions,
};

struct s_entity
{
	e_entity type;
	int id;
	float timer;
	s_v3 pos;
	s_v3 prev_pos;
	float spawn_timestamp;
	float duration;
	s_v3 target_pos;
	s_v3 dir;
	int enemy_type;
	union {

		// @Note(tkap, 04/10/2025): Player
		struct {
			s_maybe<float> is_jumping;
		};

		// @Note(tkap, 04/10/2025): Enemy
		struct {
			float speed;
			float animation_time;
		};

		// @Note(tkap, 04/10/2025): Wall
		struct {
			b8 is_fence;
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

		// @Note(tkap, 31/07/2025): Pickup
		struct {
			e_pickup pickup_type;
			s_maybe<float> last_teleport_timestamp;
		};

		// @Note(tkap, 31/07/2025): Dying enemy
		struct {
			s_v2 size;
			float gravity;
			u64 seed;
			s_v2 uv_min;
			s_v2 uv_max;
			s_v2i animation_frame;
			float rotation;
		};
	};
};

struct s_editor_entity
{
	e_entity type;
	int sub_type;
};

struct s_map
{
	int version;
	b8 active[c_map_height][c_map_width];
	s_editor_entity entity_arr[c_map_height][c_map_width];
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


struct s_transition
{
	b8 active;
	float timestamp;
	float duration;
};


struct s_soft_game_data
{
	float last_non_spammy_timestamp;

	s_hold_input hold_input;
	s_press_input press_input;
	int frames_to_freeze;
	b8 tried_to_submit_score;
	int update_count;
	float shake_intensity;
	float start_screen_shake_timestamp;
	s_list<s_particle, 65536> particle_arr;

	s_entity_manager<s_entity, c_max_entities> entity_arr;

	s_maybe<float> want_to_move_forward_timestamp;
	s_maybe<float> want_to_move_left_timestamp;
	s_maybe<float> want_to_move_right_timestamp;
	s_maybe<float> want_to_jump_timestamp;
	s_maybe<float> want_to_attack_timestamp;
	s_maybe<float> last_action_success_timestamp;
	float last_action_timestamp;
	s_maybe<float> last_attack_timestamp;
	s_maybe<float> win_timestamp;
	s_maybe<float> lose_timestamp;
	s_maybe<float> teleport_timestamp;
	s_v2 teleport_destination;
	float lose_delay;

	int num_free_actions;

	s_rng rng;

	s_transition fail_action_effect;
	s_transition teleported_timestamp;

	s_list<s_timed_msg, 8> timed_msg_arr;
};

struct s_hard_game_data
{
	int update_count;
	int current_map;

	s_array<u64, c_map_count> level_seed;

	s_transition map_win_transition;
};

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
	s_maybe<float> tutorial_end;
};

enum
{
	e_save,
	e_load,
};

enum e_editor_entity
{
	e_editor_entity_wall,
	e_editor_entity_end,
	e_editor_entity_fence,
	e_editor_entity_spike,
	e_editor_entity_enemy,
	e_editor_entity_teleport,
	e_editor_entity_plus_2_actions,
};

struct s_editor
{
	s_v2 cam_pos;
	s_maybe<e_editor_entity> to_place;
	s_map map;
};

struct s_game
{
	s_tutorial tutorial;
	b8 disable_lights;
	s_lerpable music_speed;
	s_lerpable music_volume;
	int next_entity_id;

	s_map map;

	b8 in_editor;
	s_maybe<int> saving_or_loading_map;
	int map_to_save_or_load_index;
	s_editor editor;

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

	s_atlas atlas;
	s_atlas atlas2;

	#if defined(m_winhttp)
	HINTERNET session;
	#endif

	s_len_str tooltip;

	s_fbo game_fbo;
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
// #include "gen_meta/game.h.globals"