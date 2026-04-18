
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
	#if 0
	float scale_x = length(vec3(instance_model[0]));
	float scale_y = length(vec3(instance_model[1]));

	vec3 cam_right = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 up = vec3(0.0, 1.0, 0.0);

	vec3 world_pos = vec3(instance_model[3]);

	world_pos += cam_right * vertex_pos.x * scale_x + up * vertex_pos.y * scale_y;

	gl_Position = projection * view * vec4(world_pos, 1.0);

	// @Note(tkap, 18/04/2026): Rotate around Z axis
	#else
	float scale_x = length(vec3(instance_model[0]));
	float scale_y = length(vec3(instance_model[1]));

	// Extract Z-axis rotation from the model's X axis (column 0)
	vec3 model_right = normalize(vec3(instance_model[0]));
	float roll = atan(model_right.y, model_right.x);

	float cos_roll = cos(roll);
	float sin_roll = sin(roll);

	// Rotate both billboard axes by the roll angle in screen space
	vec3 cam_right = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 cam_up    = vec3(0.0, 1.0, 0.0);

	vec3 rotated_right = cos_roll * cam_right + sin_roll * cam_up;
	vec3 rotated_up    = -sin_roll * cam_right + cos_roll * cam_up;

	vec3 world_pos = vec3(instance_model[3]);

	world_pos += rotated_right * vertex_pos.x * scale_x + rotated_up * vertex_pos.y * scale_y;

	gl_Position = projection * view * vec4(world_pos, 1.0);
	#endif

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

void main()
{
	vec4 texture_color = texture(in_texture, v_instance_uv);
	out_color = texture_color * v_color;
	out_color = mix(out_color, v_mix_color, v_mix_weight);
	if(out_color.a <= 0.0) { discard; }
}
#endif