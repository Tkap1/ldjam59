
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
shared_var vec3 v_frag_pos;
shared_var vec2 v_uv;
shared_var vec4 v_frag_pos_light_space;

#if defined(m_vertex)
void main()
{
	gl_Position = projection * view * instance_model * vec4(vertex_pos, 1);
	v_color = vertex_color * instance_color;
	v_uv = vertex_uv;
	v_frag_pos = (instance_model * vec4(vertex_pos, 1)).xyz;
	v_frag_pos_light_space = light_space_matrix * (instance_model * vec4(vertex_pos, 1));
	mat3 normal_matrix = transpose(inverse(mat3(instance_model)));
	v_normal = normalize(normal_matrix * vertex_normal);
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;
uniform sampler2D shadow_map;

out vec4 out_color;

float shadow_calculation(vec4 fragPosLightSpace)
{
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(shadow_map, projCoords.xy).r;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// calculate bias (based on depth map resolution and slope)
	vec3 normal = normalize(v_normal);
	float bias = max(0.05 * (1.0 - dot(normal, -light_dir)), 0.005);
	// check whether current frag pos is in shadow
	// float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
	// PCF
	float shadow = 0.0;
	vec2 texelSize = 1.0 / vec2(textureSize(shadow_map, 0));
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			float pcfDepth = texture(shadow_map, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if(projCoords.z > 1.0) {
		shadow = 0.0;
	}

	return shadow;
}

void main()
{
	vec3 normal = normalize(v_normal);
	vec3 color = vec3(0.0);

	vec2 uv = vec2(v_uv.x, 1.0 - v_uv.y);
	vec4 texture_color = texture(in_texture, uv);

	float shadow = shadow_calculation(v_frag_pos_light_space);
	float shadow_inv = 1.0 - shadow;
	vec3 light_color = vec3(1.0);
	vec3 ambient = light_color * 0.3;
	float diff = max(dot(-light_dir, normal), 0.0);
	vec3 diffuse = light_color * diff;

	color += (ambient + shadow_inv * diffuse) * texture_color.rgb;
	out_color = vec4(color, 1.0);
}
#endif