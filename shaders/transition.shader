
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

shared_var vec4 v_color;
shared_var vec3 v_normal;
shared_var vec2 v_vertex_uv;
shared_var vec2 v_instance_uv;
shared_var float v_mix_weight;
shared_var vec4 v_mix_color;

#if defined(m_vertex)
void main()
{
	vec3 vertex = vertex_pos;
	gl_Position = projection * view * instance_model * vec4(vertex, 1);
	v_color = vertex_color * instance_color;
	v_normal = vertex_normal;
	v_vertex_uv = vertex_uv;
	v_mix_weight = instance_mix_weight;
	v_mix_color = instance_mix_color;

	vec2 uv_arr[6] = vec2[](
		vec2(instance_uv_min.x, instance_uv_min.y),
		vec2(instance_uv_max.x, instance_uv_min.y),
		vec2(instance_uv_max.x, instance_uv_max.y),
		vec2(instance_uv_min.x, instance_uv_min.y),
		vec2(instance_uv_max.x, instance_uv_max.y),
		vec2(instance_uv_min.x, instance_uv_max.y)
	);
	v_instance_uv = uv_arr[gl_VertexID % 6];
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;

out vec4 out_color;


vec2 rotate(vec2 pos, float a)
{
	float coss = cos(a);
	float sinn = sin(a);
	return vec2(
		coss * pos.x - sinn * pos.y,
		sinn * pos.x + coss * pos.y
	);
}

void main()
{
	vec2 uv = v_instance_uv - 0.5;
	float len = length(uv);

	float t_time = transition_time;
	if(t_time <= 0.5) {
		t_time *= 2.0;
	}
	else {
		t_time = 1.0 - (t_time - 0.5) * 2.0;
	}
	vec2 uv2 = rotate(uv, (len * 20.0 * t_time)) + 0.5;
	vec4 texture_color = texture(in_texture, uv2);
	out_color = texture_color * v_color;

	float glow = clamp(1.0 - len * 2.0, 0.0, 1.0) * t_time * 2.0;
	out_color.rgb *= 1.0 - glow;
}
#endif