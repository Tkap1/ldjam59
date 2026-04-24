
#if !defined(m_cpu_side)
#define s_v2 vec2
#define s_v3 vec3
#define s_v4 vec4
#define s_m4 mat4
#endif

#if defined(m_cpu_side)
#pragma pack(push, 1)
struct s_uniform_data
#else // m_cpu_side
layout(std140) uniform ublock
#endif
{
	s_m4 view;
	s_m4 projection;
	s_v2 base_res;
	s_v2 window_size;
	s_v3 cam_pos;
	float render_time;
	s_v3 player_pos;
	float transition_time;
	s_v2 mouse;
	s_v2 camera_topleft;
	s_v2 camera_bottomright;
	float pad1;
	float pad2;
	s_v3 light_dir;
	float pad3;
	s_m4 light_space_matrix;
};
#if defined(m_cpu_side)
#pragma pack(pop)
#endif // m_cpu_side