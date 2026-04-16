
#if defined(m_vertex)
layout (location = 0) in vec2 vertex_uv;
layout (location = 1) in vec3 instance_topleft;
layout (location = 2) in vec3 instance_topright;
layout (location = 3) in vec3 instance_bottomleft;
layout (location = 4) in vec3 instance_bottomright;
layout (location = 5) in vec4 instance_color;
#endif

shared_var vec4 v_color;
shared_var vec2 v_uv;

#if defined(m_vertex)
void main()
{
	vec3 vertex_arr[6] = vec3[](
		instance_topleft, instance_topright, instance_bottomright,
		instance_topleft, instance_bottomright, instance_bottomleft
	);
	vec3 vertex = vertex_arr[gl_VertexID % 6];
	gl_Position = projection * view * vec4(vertex, 1);

	v_color = instance_color;
	v_uv = vertex_uv;
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;

out vec4 out_color;

void main()
{
	vec4 texture_color = texture(in_texture, v_uv);
	int x = int(floor(v_uv.x * 200.0));
	int y = int(floor(v_uv.y * 200.0));
	float s = (x + y) % 2 == 0 ? 0.4 : 0.3;
	texture_color.rgb *= s;

	out_color = texture_color * v_color;
	if(out_color.a <= 0.0) { discard; }
}
#endif