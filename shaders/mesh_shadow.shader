
#if defined(m_vertex)
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec4 vertex_color;
layout (location = 3) in vec2 vertex_uv;
layout (location = 4) in vec4 instance_color;
layout (location = 5) in vec4 instance_uv_min;
layout (location = 6) in vec4 instance_uv_max;
layout (location = 7) in float instance_mix_weight;
layout (location = 8) in vec4 instance_mix_color;
layout (location = 9) in mat4 instance_model;
#endif

#if defined(m_vertex)
void main()
{
	vec3 vertex = vertex_pos;
	gl_Position = projection * view * instance_model * vec4(vertex, 1);
}
#endif

#if !defined(m_vertex)

out vec4 out_color;

void main()
{
	out_color = vec4(1.0);
}
#endif