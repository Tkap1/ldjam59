

global constexpr s_v2 c_world_size = {1366, 768};
global constexpr float c_aspect_ratio = c_world_size.x / c_world_size.y;
global constexpr int c_num_tiles = 20;
global constexpr int c_tile_size = 32;
global constexpr s_v2 c_tile_size_v = {c_tile_size, c_tile_size};
global constexpr s_v2 c_world_center = {c_world_size.x * 0.5f, c_world_size.y * 0.5f};
global constexpr int c_max_vertices = 16384;
global constexpr int c_max_faces = 8192;
global constexpr int c_updates_per_second = 30;
global constexpr f64 c_update_delay = 1.0 / c_updates_per_second;
global constexpr float c_transition_time = 0.25f;
global constexpr float c_knockback = 400;
global constexpr char* c_key_color_str = "$$26A5D9";
global constexpr s_v4 c_key_color = {0.149020f, 0.647059f, 0.850980f, 1};
global constexpr int c_invalid_index = -10000000;

global constexpr float c_game_speed_arr[] = {
	0.0f, 0.01f, 0.1f, 0.25f, 0.5f,
	1,
	2, 4, 8, 16
};


global constexpr int c_max_keys = 1024;
global constexpr int c_left_button = c_max_keys - 2;
global constexpr int c_right_button = c_max_keys - 1;
global constexpr int c_left_shift = c_max_keys - 3;
global constexpr int c_left_ctrl = c_max_keys - 4;

global constexpr int c_max_leaderboard_entries = 2000;
global constexpr int c_leaderboard_visible_entries_per_page = 10;
global constexpr int c_leaderboard_id = 31932;

global constexpr s_len_str c_game_name = S("LD59");

global constexpr int c_render_pass_count = 4;