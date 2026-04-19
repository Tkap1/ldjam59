


func s_render_flush_data make_render_flush_data(s_v3 cam_pos, s_v3 player_pos, s_m4 view_inv)
{
	s_render_flush_data result = zero;
	result.view = m4_identity();
	result.view_inv = view_inv;
	result.projection = m4_identity();
	result.light_view = m4_identity();
	result.light_projection = m4_identity();
	result.cam_pos = cam_pos;
	result.cull_mode = e_cull_mode_disabled;
	result.blend_mode = e_blend_mode_normal;
	result.depth_mode = e_depth_mode_read_and_write;
	result.player_pos = player_pos;
	return result;
}

func s_mesh make_mesh_from_vertices(s_vertex* vertex_arr, int vertex_count)
{
	s_mesh result = zero;
	gl(glGenVertexArrays(1, &result.vao));
	gl(glBindVertexArray(result.vao));

	s_gl_attrib_manager attrib_manager = zero;

	{
		gl(glGenBuffers(1, &result.vertex_vbo));
		gl(glBindBuffer(GL_ARRAY_BUFFER, result.vertex_vbo));

		attrib_manager_add_float(&attrib_manager, 3); // pos
		attrib_manager_add_float(&attrib_manager, 3); // normal
		attrib_manager_add_float(&attrib_manager, 4); // rgba
		attrib_manager_add_float(&attrib_manager, 2); // uv
		attrib_manager_finish(&attrib_manager);

		result.vertex_count = vertex_count;
		gl(glBufferData(GL_ARRAY_BUFFER, sizeof(s_vertex) * vertex_count, vertex_arr, GL_STATIC_DRAW));
	}

	{
		gl(glGenBuffers(1, &result.instance_vbo.id));
		gl(glBindBuffer(GL_ARRAY_BUFFER, result.instance_vbo.id));

		attrib_manager_add_float_divisor(&attrib_manager, 4); // instance color
		attrib_manager_add_float_divisor(&attrib_manager, 2); // uv min
		attrib_manager_add_float_divisor(&attrib_manager, 2); // uv max
		attrib_manager_add_float_divisor(&attrib_manager, 1); // mix weight
		attrib_manager_add_float_divisor(&attrib_manager, 4); // mix color
		attrib_manager_add_float_divisor(&attrib_manager, 4); // model matrix
		attrib_manager_add_float_divisor(&attrib_manager, 4); // model matrix
		attrib_manager_add_float_divisor(&attrib_manager, 4); // model matrix
		attrib_manager_add_float_divisor(&attrib_manager, 4); // model matrix
		attrib_manager_finish(&attrib_manager);
	}
	return result;
}

func s_mesh make_plane_mesh()
{
	s_mesh result = zero;
	gl(glGenVertexArrays(1, &result.vao));
	gl(glBindVertexArray(result.vao));

	s_gl_attrib_manager attrib_manager = zero;
	constexpr int vertex_count = 6;
	result.vertex_count = vertex_count;

	{
		gl(glGenBuffers(1, &result.vertex_vbo));
		gl(glBindBuffer(GL_ARRAY_BUFFER, result.vertex_vbo));

		s_v2 uv_arr[6] = {
			v2(0, 1), v2(1, 1), v2(1, 0),
			v2(0, 1), v2(1, 0), v2(0, 0),
		};

		attrib_manager_add_float(&attrib_manager, 2); // uv
		attrib_manager_finish(&attrib_manager);

		gl(glBufferData(GL_ARRAY_BUFFER, sizeof(uv_arr) * vertex_count, uv_arr, GL_STATIC_DRAW));
	}


	{
		gl(glGenBuffers(1, &result.instance_vbo.id));
		gl(glBindBuffer(GL_ARRAY_BUFFER, result.instance_vbo.id));

		// @Note(tkap, 09/10/2025): From the front
		attrib_manager_add_float_divisor(&attrib_manager, 3); // top left
		attrib_manager_add_float_divisor(&attrib_manager, 3); // top right
		attrib_manager_add_float_divisor(&attrib_manager, 3); // bottom left
		attrib_manager_add_float_divisor(&attrib_manager, 3); // bottom right
		attrib_manager_add_float_divisor(&attrib_manager, 4); // color
		attrib_manager_finish(&attrib_manager);
	}
	return result;
}

func s_ply_mesh parse_ply_mesh(char* path, s_linear_arena* arena)
{
	char* data = (char*)read_file(path, arena, null);
	char* cursor = data;
	s_ply_mesh result = zero;
	b8 has_color = strstr(cursor, "property uchar red") != null;

	{
		char* temp = strstr(cursor, "element vertex ") + 15;
		result.vertex_count = atoi(temp);
		assert(result.vertex_count <= c_max_vertices);
	}
	{
		char* temp = strstr(cursor, "element face ") + 13;
		result.face_count = atoi(temp);
		assert(result.face_count <= c_max_faces);
	}
	cursor = strstr(data, "end_header") + 11;

	{
		u8* temp = (u8*)cursor;
		for(int i = 0; i < result.vertex_count; i += 1) {
			memcpy(&result.vertex_arr[i].pos, temp, sizeof(s_v3));
			temp += sizeof(s_v3);
			memcpy(&result.vertex_arr[i].normal, temp, sizeof(s_v3));
			temp += sizeof(s_v3);
			if(has_color) {
				result.vertex_arr[i].color.x = (*temp) / 255.0f;
				temp += 1;
				result.vertex_arr[i].color.y = (*temp) / 255.0f;
				temp += 1;
				result.vertex_arr[i].color.z = (*temp) / 255.0f;
				temp += 1;
				result.vertex_arr[i].color.w = (*temp) / 255.0f;
				temp += 1;
			}
			else {
				result.vertex_arr[i].color = make_rrr(1);
			}
			memcpy(&result.vertex_arr[i].uv, temp, sizeof(s_v2));
			temp += sizeof(s_v2);
		}
		cursor = (char*)temp;
	}
	{
		s_ply_face* temp = (s_ply_face*)cursor;
		for(int i = 0; i < result.face_count; i += 1) {
			result.face_arr[i] = *temp;
			assert(result.face_arr[i].index_count == 3);
			temp += 1;
		}
	}
	return result;
}

func s_mesh make_mesh_from_ply_file(char* file, s_linear_arena* arena)
{
	s_ply_mesh ply_mesh = parse_ply_mesh(file, arena);
	int vertex_count = ply_mesh.face_count * 3;
	assert(vertex_count < c_max_vertices);
	s_vertex vertex_arr[c_max_vertices] = zero;
	for(int i = 0; i < ply_mesh.face_count; i += 1) {
		vertex_arr[i * 3 + 0] = ply_mesh.vertex_arr[ply_mesh.face_arr[i].index_arr[0]];
		vertex_arr[i * 3 + 1] = ply_mesh.vertex_arr[ply_mesh.face_arr[i].index_arr[1]];
		vertex_arr[i * 3 + 2] = ply_mesh.vertex_arr[ply_mesh.face_arr[i].index_arr[2]];
	}
	s_mesh result = make_mesh_from_vertices(vertex_arr, vertex_count);
	return result;
}

func s_mesh make_mesh_from_obj_file(char* file, s_linear_arena* arena)
{
	s_obj_mesh* obj_mesh = parse_obj_mesh(file, arena);
	int vertex_count = obj_mesh->face_count * 3;
	assert(vertex_count < c_max_vertices);
	s_vertex vertex_arr[c_max_vertices] = zero;
	for(int i = 0; i < obj_mesh->face_count; i += 1) {
		for(int j = 0; j < 3; j += 1) {
			vertex_arr[i * 3 + j].pos = obj_mesh->vertex_arr[obj_mesh->face_arr[i].vertex_index[j] - 1].pos;
			vertex_arr[i * 3 + j].normal = obj_mesh->normal_arr[obj_mesh->face_arr[i].normal_index[j] - 1];
			vertex_arr[i * 3 + j].uv = obj_mesh->uv_arr[obj_mesh->face_arr[i].uv_index[j] - 1];
			vertex_arr[i * 3 + j].color.rgb = obj_mesh->vertex_arr[obj_mesh->face_arr[i].vertex_index[j] - 1].color;
			vertex_arr[i * 3 + j].color.a = 1;
		}
	}
	s_mesh result = make_mesh_from_vertices(vertex_arr, vertex_count);
	return result;
}

func void set_cull_mode(e_cull_mode mode)
{
	switch(mode) {
		case e_cull_mode_disabled: {
			glDisable(GL_CULL_FACE);
		} break;

		case e_cull_mode_back_ccw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
		} break;

		case e_cull_mode_front_ccw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glFrontFace(GL_CCW);
		} break;

		case e_cull_mode_back_cw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CW);
		} break;

		case e_cull_mode_front_cw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glFrontFace(GL_CW);
		} break;
		invalid_default_case;
	}
}

func void set_depth_mode(e_depth_mode mode)
{
	switch(mode) {
		case e_depth_mode_no_read_no_write: {
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
		} break;
		case e_depth_mode_read_and_write: {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);
		} break;
		case e_depth_mode_read_no_write: {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_FALSE);
		} break;
		case e_depth_mode_no_read_yes_write: {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_ALWAYS);
			glDepthMask(GL_TRUE);
		} break;
		invalid_default_case;
	}
}

func void set_blend_mode(e_blend_mode mode)
{
	switch(mode) {
		case e_blend_mode_disabled: {
			glDisable(GL_BLEND);
		} break;
		case e_blend_mode_additive: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		} break;
		case e_blend_mode_premultiply_alpha: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		} break;
		case e_blend_mode_multiply: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		} break;
		case e_blend_mode_multiply_inv: {
			glEnable(GL_BLEND);
			// glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
			glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_COLOR, GL_ZERO, GL_ONE);
		} break;
		case e_blend_mode_normal: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		} break;
		case e_blend_mode_additive_no_alpha: {
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
		} break;
		invalid_default_case;
	}
}

func void set_window_size(int width, int height)
{
	SDL_SetWindowSize(g_platform_data->window, width, height);
}

func s_texture load_texture_from_file(char* path, u32 filtering)
{
	int width, height, num_channels;
	void* data = stbi_load(path, &width, &height, &num_channels, 4);
	assert(data);

	s_texture result = load_texture_from_data(data, width, height, GL_RGBA, filtering);
	return result;
}

func s_texture load_texture_from_data(void* data, int width, int height, int format, u32 filtering)
{
	s_texture result = zero;

	gl(glGenTextures(1, &result.id));
	gl(glBindTexture(GL_TEXTURE_2D, result.id));
	gl(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering));
	gl(glGenerateMipmap(GL_TEXTURE_2D));

	return result;
}

// @TODO(tkap, 13/10/2024): premultiply??
func s_font load_font_from_file(char* file, int font_size, s_linear_arena* arena)
{
	u8* file_data = read_file(file, arena, null);
	s_font font = {};
	font.size = (float)font_size;

	assert(file_data);

	stbtt_fontinfo info = {};
	stbtt_InitFont(&info, file_data, 0);

	stbtt_GetFontVMetrics(&info, &font.ascent, &font.descent, &font.line_gap);

	font.scale = stbtt_ScaleForPixelHeight(&info, (float)font_size);
	constexpr int max_chars = 128;
	int bitmap_count = 0;
	u8* bitmap_arr[max_chars];
	const int padding = 10;

	int columns = floorfi((float)(4096 / (font_size + padding)));
	int rows = ceilfi((max_chars - columns) / (float)columns) + 1;

	int total_width = floorfi((float)(columns * (font_size + padding)));
	// @Note(tkap, 20/10/2023): We need to align the texture width to 4 bytes! Very important! Thanks to tk_dev
	total_width = (total_width + 3) & ~3;
	int total_height = floorfi((float)(rows * (font_size + padding)));

	for(int char_i = 0; char_i < max_chars; char_i++)
	{
		s_glyph glyph = {};
		u8* bitmap = stbtt_GetCodepointBitmap(&info, 0, font.scale, char_i, &glyph.width, &glyph.height, 0, 0);
		stbtt_GetCodepointBox(&info, char_i, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);
		stbtt_GetGlyphHMetrics(&info, char_i, &glyph.advance_width, NULL);

		font.glyph_arr[char_i] = glyph;
		bitmap_arr[bitmap_count++] = bitmap;
	}

	u8* gl_bitmap = (u8*)arena_alloc_zero(arena, sizeof(u8) * 1 * total_width * total_height);

	for(int char_i = 0; char_i < max_chars; char_i++) {
		s_glyph* glyph = &font.glyph_arr[char_i];
		u8* bitmap = bitmap_arr[char_i];
		int column = char_i % columns;
		int row = char_i / columns;
		for(int y = 0; y < glyph->height; y++) {
			for(int x = 0; x < glyph->width; x++) {
				int current_x = floorfi((float)(column * (font_size + padding)));
				int current_y = floorfi((float)(row * (font_size + padding)));
				u8 src_pixel = bitmap[x + y * glyph->width];
				u8* dst_pixel = &gl_bitmap[((current_x + x) + (current_y + y) * total_width)];
				dst_pixel[0] = src_pixel;
			}
		}

		glyph->uv_min.x = column / (float)columns;
		glyph->uv_max.x = glyph->uv_min.x + (glyph->width / (float)total_width);

		// @Hack(tkap, 07/04/2025): Without this letters are cut off on the left side
		glyph->uv_min.x -= 0.001f;

		glyph->uv_min.y = row / (float)rows;

		// @Note(tkap, 17/05/2023): For some reason uv_max.y is off by 1 pixel (checked the texture in renderoc), which causes the text to be slightly miss-positioned
		// in the Y axis. "glyph->height - 1" fixes it.
		glyph->uv_max.y = glyph->uv_min.y + (glyph->height / (float)total_height);

		// @Note(tkap, 17/05/2023): Otherwise the line above makes the text be cut off at the bottom by 1 pixel...
		// glyph->uv_max.y += 0.01f;
	}

	for(int bitmap_i = 0; bitmap_i < bitmap_count; bitmap_i++) {
		stbtt_FreeBitmap(bitmap_arr[bitmap_i], NULL);
	}

	game->texture_arr[e_texture_font] = load_texture_from_data(gl_bitmap, total_width, total_height, m_gl_single_channel, GL_LINEAR);

	return font;
}

func s_v2 draw_text(s_len_str text, s_v2 in_pos, float font_size, s_v4 color, b8 centered, s_font* font, s_draw_data draw_data, int render_pass_index)
{
	float scale = font->scale * (font_size / font->size);

	assert(text.count > 0);
	if(centered) {
		s_v2 text_size = get_text_size(text, font, font_size);
		in_pos.x -= text_size.x / 2;
		in_pos.y -= text_size.y / 2;
	}
	s_v2 pos = in_pos;
	pos.y += font->ascent * scale;

	s_text_iterator it = {};
	while(iterate_text(&it, text, color)) {
		for(int char_i = 0; char_i < it.text.count; char_i++) {
			int c = it.text[char_i];
			if(c <= 0 || c >= 128) { continue; }

			if(c == '\n' || c == '\r') {
				pos.x = in_pos.x;
				pos.y += font_size;
				continue;
			}

			s_glyph glyph = font->glyph_arr[c];
			s_v2 draw_size = v2((glyph.x1 - glyph.x0) * scale, (glyph.y1 - glyph.y0) * scale);

			s_v2 glyph_pos = pos;
			glyph_pos.x += glyph.x0 * scale;
			glyph_pos.y += -glyph.y0 * scale;

			// t.flags |= e_render_flag_use_texture | e_render_flag_text;
			s_v3 tpos = v3(glyph_pos, 0.0f);

			s_v2 center = tpos.xy + draw_size / 2 * v2(1, -1);
			s_v2 bottomleft = tpos.xy;


			// s_m4 model = m4_translate(v3(tpos.xy, draw_data.z));
			// model = m4_multiply(model, m4_scale(v3(draw_size, 1)));

			// t.color = it.color;
			s_v2 uv_min = glyph.uv_min;
			s_v2 uv_max = glyph.uv_max;
			// swap(&uv_min.y, &uv_max.y);
			// t.origin_offset = c_origin_bottomleft;

			s_v4 temp_color = it.color;
			temp_color.a = color.a;
			draw_texture_screen(tpos.xy, draw_size, temp_color, e_texture_font, e_shader_text, uv_min, uv_max, draw_data, render_pass_index);

			// draw_generic(game_renderer, &t, render_pass, render_data.shader, font->texture.game_id, e_mesh_rect);

			pos.x += glyph.advance_width * scale;

		}
	}

	return v2(pos.x, in_pos.y);
}

// @TODO(tkap, 31/10/2023): Handle new lines
func s_v2 get_text_size_with_count(s_len_str in_text, s_font* font, float font_size, int count, int in_column)
{
	assert(count >= 0);
	if(count <= 0) { return {}; }

	int column = in_column;

	s_v2 size = {};
	float max_width = 0;
	float scale = font->scale * (font_size / font->size);
	size.y = font_size;

	s_len_str text = substr_from_to_exclusive(in_text, 0, count);
	s_text_iterator it = {};
	while(iterate_text(&it, text, make_rrr(0))) {
		for(int char_i = 0; char_i < it.text.count; char_i++) {
			char c = it.text[char_i];
			s_glyph glyph = font->glyph_arr[c];
			if(c == '\t') {
				int spaces = get_spaces_for_column(column);
				size.x += glyph.advance_width * scale * spaces;
				column += spaces;
			}
			else if(c == '\n') {
				size.y += font_size;
				size.x = 0;
				column = 0;
			}
			else {
				size.x += glyph.advance_width * scale;
				column += 1;
			}
			max_width = max(size.x, max_width);
		}
	}
	size.x = max_width;

	return size;
}

func s_v2 get_text_size(s_len_str text, s_font* font, float font_size)
{
	return get_text_size_with_count(text, font, font_size, text.count, 0);
}

func b8 iterate_text(s_text_iterator* it, s_len_str text, s_v4 color)
{
	if(it->index >= text.count) { return false; }

	if(it->color_stack.count <= 0) {
		it->color_stack.add(color);
	}

	it->color = it->color_stack.get_last();

	int index = it->index;
	int advance = 0;
	while(index < text.count) {
		char c = text[index];
		char next_c = index < text.count - 1 ? text[index + 1] : 0;
		if(c == '$' && next_c == '$') {
			s_len_str red_str = substr_from_to_exclusive(text, index + 2, index + 4);
			s_len_str green_str = substr_from_to_exclusive(text, index + 4, index + 6);
			s_len_str blue_str = substr_from_to_exclusive(text, index + 6, index + 8);
			float red = hex_str_to_int(red_str) / 255.0f;
			float green = hex_str_to_int(green_str) / 255.0f;
			float blue = hex_str_to_int(blue_str) / 255.0f;
			s_v4 temp_color = make_rgb(red, green, blue);
			it->color_stack.add(temp_color);

			if(index == it->index) {
				index += 8;
				it->index += 8;
				it->color = it->color_stack.get_last();
				continue;
			}
			else {
				advance = 8;
				break;
			}
		}
		else if(c == '$' && next_c == '.') {
			if(index == it->index) {
				it->color_stack.pop_last();
				it->color = it->color_stack.get_last();
				index += 2;
				it->index += 2;
				continue;
			}
			else {
				advance = 2;
				it->color_stack.pop_last();
				break;
			}
		}
		index += 1;
	}
	it->text = substr_from_to_exclusive(text, it->index, index);
	it->index = index + advance;
	return true;
}


func int hex_str_to_int(s_len_str str)
{
	int result = 0;
	int tens = 0;
	for(int i = str.count - 1; i >= 0; i -= 1) {
		char c = str[i];
		int val = 0;
		if(is_number(c)) {
			val = c - '0';
		}
		else if(c >= 'a' && c <= 'f') {
			val = c - 'a' + 10;
		}
		else if(c >= 'A' && c <= 'F') {
			val = c - 'A' + 10;
		}
		result += val * (int)powf(16, (float)tens);
		tens += 1;
	}
	return result;
}

func s_len_str format_text(const char* text, ...)
{
	static constexpr int max_format_text_buffers = 16;
	static constexpr int max_text_buffer_length = 512;

	static char buffers[max_format_text_buffers][max_text_buffer_length] = {};
	static int index = 0;

	char* current_buffer = buffers[index];
	memset(current_buffer, 0, max_text_buffer_length);

	va_list args;
	va_start(args, text);
	#ifdef m_debug
	int written = vsnprintf(current_buffer, max_text_buffer_length, text, args);
	assert(written > 0 && written < max_text_buffer_length);
	#else
	vsnprintf(current_buffer, max_text_buffer_length, text, args);
	#endif
	va_end(args);

	index += 1;
	if(index >= max_format_text_buffers) { index = 0; }

	int len = (int)strlen(current_buffer);
	s_len_str result = zero;
	result.count = len;
	result.str = (char*)circular_arena_alloc(&game->circular_arena, len);
	memcpy(result.str, current_buffer, len);
	return result;
}


func s_len_str format_text2(const char* text, ...)
{
	static constexpr int max_format_text_buffers = 16;
	static constexpr int max_text_buffer_length = 512;

	static char buffers[max_format_text_buffers][max_text_buffer_length] = {};
	static int index = 0;

	char* current_buffer = buffers[index];
	memset(current_buffer, 0, max_text_buffer_length);

	va_list args;
	va_start(args, text);
	#ifdef m_debug
	int written = vsnprintf(current_buffer, max_text_buffer_length, text, args);
	assert(written > 0 && written < max_text_buffer_length);
	#else
	vsnprintf(current_buffer, max_text_buffer_length, text, args);
	#endif
	va_end(args);

	index += 1;
	if(index >= max_format_text_buffers) { index = 0; }

	return S(current_buffer);
}


func u8* try_really_hard_to_read_file(char* file, s_linear_arena* arena)
{
	u8* result = null;
	for(int i = 0; i < 100; i += 1) {
		result = read_file(file, arena, null);
		if(result) {
			break;
		}
		SDL_Delay(10);
	}
	return result;
}

func float update_time_to_render_time(float time, float interp_dt)
{
	float result = time + (float)c_update_delay * interp_dt;
	return result;
}

func s_m4 fullscreen_m4()
{
	s_m4 result = m4_translate(v3(c_world_center, 0.0f));
	result = m4_multiply(result, m4_scale(v3(c_world_size, 0.0f)));
	return result;
}

func s_time_format update_time_to_time_format(float update_time, b8 handle_hours)
{
	s_time_format result = zero;
	float milliseconds = update_time * 1000;

	if(handle_hours) {
		result.hours = floorfi(milliseconds / 1000 / 60 / 60);
		milliseconds -= result.hours * 1000 * 60 * 60;
	}

	result.minutes = floorfi(milliseconds / 1000 / 60);
	milliseconds -= result.minutes * 1000 * 60;

	result.seconds = floorfi(milliseconds / 1000);
	milliseconds -= result.seconds * 1000;

	result.milliseconds = floorfi(milliseconds);

	return result;
}

func s_time_format update_count_to_time_format(int update_count, b8 handle_hours)
{
	float update_time = update_count * (float)c_update_delay;
	s_time_format result = update_time_to_time_format(update_time, handle_hours);
	return result;
}

func s_obj_mesh* parse_obj_mesh(char* path, s_linear_arena* arena)
{
	s_obj_mesh* result = (s_obj_mesh*)arena_alloc_zero(arena, sizeof(s_obj_mesh));
	int file_size = 0;
	char* data = (char*)read_file(path, arena, &file_size);
	char* end_ptr = data + file_size;
	assert(data);
	char* cursor = strstr(data, "\nv ") + 1;
	while(memcmp(cursor, "v ", 2) == 0) {
		cursor += 2;
		char* end = null;
		result->vertex_arr[result->vertex_count].pos.x = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->vertex_arr[result->vertex_count].pos.y = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->vertex_arr[result->vertex_count].pos.z = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;

		result->vertex_arr[result->vertex_count].color.r = strtof(cursor, &end);
		if(end > cursor) {
			cursor = end;
			result->vertex_arr[result->vertex_count].color.g = strtof(cursor, &end);
			assert(end > cursor);
			cursor = end;
			result->vertex_arr[result->vertex_count].color.b = strtof(cursor, &end);
			assert(end > cursor);
			cursor = end;
		}
		else {
			result->vertex_arr[result->vertex_count].color = v3(1, 1, 1);
		}
		result->vertex_count += 1;

		while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
			cursor += 1;
		}
	}

	while(memcmp(cursor, "vn ", 3) == 0) {
		cursor += 3;
		char* end = null;
		result->normal_arr[result->normal_count].x = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->normal_arr[result->normal_count].y = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->normal_arr[result->normal_count].z = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->normal_count += 1;

		while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
			cursor += 1;
		}
	}

	while(memcmp(cursor, "vt ", 3) == 0) {
		cursor += 3;
		char* end = null;
		result->uv_arr[result->uv_count].x = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->uv_arr[result->uv_count].y = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->uv_count += 1;

		while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
			cursor += 1;
		}
	}

	while(memcmp(cursor, "f ", 2) != 0) {
		cursor += 1;
	}

	// @Note(tkap, 03/08/2025): This fixes sanitizer, but could be wrong somehow? surely not but needs testing
	while(cursor < end_ptr && memcmp(cursor, "f ", 2) == 0) {
		cursor += 2;
		char* end = null;

		for(int i = 0; i < 3; i += 1) {
			result->face_arr[result->face_count].vertex_index[i] = (int)strtol(cursor, &end, 10);
			assert(end > cursor);
			cursor = end + 1;
			result->face_arr[result->face_count].uv_index[i] = (int)strtol(cursor, &end, 10);
			assert(end > cursor);
			cursor = end + 1;
			result->face_arr[result->face_count].normal_index[i] = (int)strtol(cursor, &end, 10);
			assert(end > cursor);
			cursor = end;

			while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
				cursor += 1;
			}
		}
		result->face_count += 1;
		assert(result->face_count < c_max_faces);

	}

	return result;
}

func char* skip_whitespace(char* str)
{
	while(true) {
		if(*str == '0') { break; }
		else if(*str <= ' ') { str += 1; }
		else { break; }
	}
	return str;
}

func s_v2 wxy(float x, float y)
{
	s_v2 result = c_world_size * v2(x, y);
	return result;
}

func s_v2 wxy(float x)
{
	s_v2 result = c_world_size * v2(x, x);
	return result;
}

func s_v2 wcxy(float x, float y)
{
	s_v2 result = c_world_center * v2(x, y);
	return result;
}

func void update_particles(float delta, b8 do_draw, int render_pass_index)
{
	s_soft_game_data* soft_data = &game->soft_data;

	for(int i = c_first_index[e_entity_emitter]; i < c_last_index_plus_one[e_entity_emitter]; i += 1) {
		if(!soft_data->entity_arr.active[i]) { continue; }
		s_particle_emitter_a a = soft_data->entity_arr.data[i].emitter_a;
		s_particle_emitter_b* b = &soft_data->entity_arr.data[i].emitter_b;
		s_time_data spawn_time_data = get_time_data(game->render_time, b->last_emit_timestamp, 1.0f / b->particles_per_second);

		s_time_data expire_time_data = zero;

		// @Note(tkap, 23/04/2025): Not infinite
		if(b->duration >= 0) {
			expire_time_data = get_time_data(game->render_time, b->creation_timestamp, b->duration);
		}

		while(spawn_time_data.percent >= 1 && (expire_time_data.percent < 1 || !b->existed_in_previous_frame)) {
			spawn_time_data.percent -= 1;
			b->last_emit_timestamp = game->render_time;

			if(!b->paused) {
				for(int particle_i = 0; particle_i < b->particle_count; particle_i += 1) {
					b->num_alive_particles += 1;

					s_particle particle = zero;
					particle.emitter_index = i;
					particle.seed = randu(&game->rng);
					if(!a.follow_emitter) {
						particle.pos = a.pos;
					}

					switch(b->spawn_type) {
						xcase e_emitter_spawn_type_point: {
						};

						xcase e_emitter_spawn_type_circle: {
							particle.pos.xy += random_point_in_circle(&game->rng, b->spawn_data.x);
						};

						xcase e_emitter_spawn_type_sphere: {
							particle.pos += random_point_in_sphere(&game->rng, b->spawn_data.x);
						};

						xcase e_emitter_spawn_type_rect_center: {
							particle.pos.xy += random_point_in_rect_center(&game->rng, b->spawn_data.xy);
						};

						xcase e_emitter_spawn_type_rect_edge: {
							particle.pos.xy += random_point_in_rect_edges(&game->rng, b->spawn_data.xy);
						};

						xcase e_emitter_spawn_type_circle_outline: {
							float r = randf_range(&game->rng, 0, c_tau);
							particle.pos.x += cosf(r) * b->spawn_data.x;
							particle.pos.y += sinf(r) * b->spawn_data.x;
						};
					}
					particle.spawn_timestamp = game->render_time;
					soft_data->particle_arr.add_if_not_full(particle);
				}
			}
		}

		if(expire_time_data.percent >= 1 && b->num_alive_particles <= 0) {
			entity_manager_remove(&soft_data->entity_arr, e_entity_emitter, i);
		}
		b->existed_in_previous_frame = true;

		if(b->pause_after_update) {
			b->paused = true;
		}
	}

	foreach_ptr(particle_i, particle, soft_data->particle_arr) {
		s_rng rng = make_rng(particle->seed);
		s_particle_emitter_a data_a = soft_data->entity_arr.data[particle->emitter_index].emitter_a;
		s_particle_emitter_b* data_b = &soft_data->entity_arr.data[particle->emitter_index].emitter_b;
		float duration = data_a.particle_duration * (1.0f - randf32(&rng) * data_a.particle_duration_rand);
		s_time_data time_data = get_time_data(game->render_time, particle->spawn_timestamp, duration);
		float radius = data_a.radius * (1.0f - randf32(&rng) * data_a.radius_rand);
		float speed = data_a.speed * (1.0f - randf32(&rng) * data_a.speed_rand);
		radius *= 1.0f - time_data.percent * data_a.shrink;

		s_v3 dir = zero;
		dir.x = data_a.dir.x * (1.0f - randf32(&rng) * data_a.dir_rand.x * 2);
		dir.y = data_a.dir.y * (1.0f - randf32(&rng) * data_a.dir_rand.y * 2);
		dir.z = data_a.dir.z * (1.0f - randf32(&rng) * data_a.dir_rand.z * 2);
		dir = v3_normalized(dir);

		s_v4 color = get_particle_color(&rng, time_data.percent, data_a.color_arr);
		color.a = 1.0f - time_data.percent;
		particle->pos += dir * speed * delta;
		particle->pos.y += time_data.passed * data_a.gravity;

		s_v3 pos = particle->pos;
		if(data_a.follow_emitter) {
			pos += data_a.pos;
		}

		if(do_draw) {
			s_instance_data data = zero;
			data.model = m4_translate(pos);
			data.model *= m4_scale(v3(radius, radius, 1));
			// scale_m4_by_radius(&data.model, radius);
			data.color = color;
			add_to_render_group(data, e_shader_flat, e_texture_white, e_mesh_quad, render_pass_index);
		}
		if(time_data.percent >= 1) {
			soft_data->particle_arr.remove_and_swap(particle_i);
			particle_i -= 1;
			data_b->num_alive_particles -= 1;
		}
	}
}

template <typename t, int n>
func int entity_manager_add(s_entity_manager<t, n>* manager, e_entity type, t new_entity)
{
	new_entity.type = type;
	assert(manager->count[type] < g_entity_type_data[type].max_count);
	int index = manager->free_list[type][manager->count[type]];
	assert(!manager->active[index]);
	manager->data[index] = new_entity;
	manager->active[index] = true;
	manager->count[type] += 1;
	return index;
}

template <typename t, int n>
func int entity_manager_add_if_not_full(s_entity_manager<t, n>* manager, e_entity type, t new_entity)
{
	if(manager->count[type] >= g_entity_type_data[type].max_count) {
		return c_invalid_index;
	}
	int index = entity_manager_add(manager, type, new_entity);
	return index;
}

template <typename t, int n>
func void entity_manager_remove(s_entity_manager<t, n>* manager, e_entity type, int index)
{
	assert(manager->active[index]);
	assert(manager->count[type] > 0);
	manager->count[type] -= 1;
	manager->active[index] = false;
	manager->free_list[type][manager->count[type]] = index;
}

template <typename t, int n>
func void entity_manager_reset(s_entity_manager<t, n>* manager, e_entity type)
{
	manager->count[type] = 0;
	int first_index = c_first_index[type];
	int count = c_last_index_plus_one[type] - first_index;
	memset(&manager->active[first_index], 0, sizeof(manager->active[0]) * count);
	manager->free_list[type] = (int*)arena_alloc_zero(&game->arena, sizeof(int) * g_entity_type_data[type].max_count);
	for(int i = 0; i < g_entity_type_data[type].max_count; i += 1) {
		manager->free_list[type][i] = c_first_index[type] + i;
	}
}

func s_v4 get_particle_color(s_rng* rng, float percent, s_list<s_particle_color, 4> color_arr)
{
	s_particle_color choice[2] = {color_arr[0], color_arr[at_most(color_arr.count - 1, 1)]};

	foreach_val(color_i, color, color_arr) {
		if(color.percent <= percent) {
			choice[0] = color;
			choice[1] = color_arr[at_most(color_arr.count - 1, color_i + 1)];
		}
	}

	float p = ilerp(choice[0].percent, choice[1].percent, percent);
	s_v4 choice2[2] = zero;

	for(int i = 0; i < 2; i += 1) {
		s_v4 temp = choice[i].color;
		if(choice[i].color_rand_per_channel) {
			float r = (1.0f - randf32(rng) * choice[i].color_rand);
			float g = (1.0f - randf32(rng) * choice[i].color_rand);
			float b = (1.0f - randf32(rng) * choice[i].color_rand);
			temp.r *= r;
			temp.g *= g;
			temp.b *= b;
		}
		else {
			float r = (1.0f - randf32(rng) * choice[i].color_rand);
			temp = multiply_rgb(choice[i].color, r);
		}
		choice2[i] = temp;
	}
	s_v4 result = lerp_color(choice2[0], choice2[1], p);
	return result;
}

func void play_sound(e_sound sound_id, s_play_sound_data data)
{
	if(!game->turn_off_all_sounds) {
		g_platform_data->play_sound(sound_id, data);
	}
}

func void timer_activate(s_timer* timer, float time_now)
{
	timer->used_timestamp = time_now;
	timer->want_to_use_timestamp = 0;
}

func b8 timer_can_activate(s_timer timer, float time_now)
{
	b8 result = false;
	float time_passed_since_last_activation = time_now - (timer.used_timestamp + timer.duration);
	if(time_passed_since_last_activation >= timer.cooldown || timer.used_timestamp <= 0) {
		result = true;
	}
	return result;
}

func b8 timer_want_activate(s_timer timer, float time_now, float grace_period)
{
	b8 result = false;
	float passed = time_now - timer.want_to_use_timestamp;
	if(passed <= grace_period && timer.want_to_use_timestamp > 0) {
		result = true;
	}
	return result;
}

func b8 timer_can_and_want_activate(s_timer timer, float time_now, float grace_period)
{
	b8 can = timer_can_activate(timer, time_now);
	b8 want = timer_want_activate(timer, time_now, grace_period);
	b8 result = can && want;
	return result;
}

func b8 timer_is_active(s_timer timer, float time_now)
{
	float passed = time_now - timer.used_timestamp;
	b8 result = passed <= timer.duration && timer.used_timestamp > 0;
	return result;
}

func s_time_data timer_get_time_data(s_timer timer, float time_now, b8* active)
{
	if(active) {
		*active = timer_is_active(timer, time_now);
	}
	s_time_data time_data = get_time_data(time_now, timer.used_timestamp, timer.duration);
	return time_data;
}

func s_time_data timer_get_cooldown_time_data(s_timer timer, float time_now, b8* is_on_cooldown)
{
	if(is_on_cooldown) {
		*is_on_cooldown = !timer_can_activate(timer, time_now);
	}
	s_time_data time_data = get_time_data(time_now, timer.used_timestamp + timer.duration, timer.cooldown);
	return time_data;
}

func float get_rand_sound_speed(float x, s_rng* rng)
{
	assert(x > 1);
	float low = 1.0f / x;
	float high = x;
	float result = randf_range(rng, low, high);
	return result;
}

func s_audio_fade make_simple_fade(float p0, float p1)
{
	s_audio_fade fade = zero;
	fade.percent[0] = p0;
	fade.percent[1] = p1;
	fade.volume[0] = 1;
	fade.volume[1] = 0;
	return fade;
}

func char scancode_to_char(SDL_Scancode scancode)
{
	SDL_Keycode key = SDL_GetKeyFromScancode(scancode);
	char result = (char)key;
	return result;
}

#if defined(m_debug)
func b8 cheat_key(int key)
{
	b8 result = false;
	if(is_key_pressed(key, true)) {
		result = true;
	}
	return result;
}
#endif

func void pre_render(float delta)
{
	(void)delta;
	reset_linear_arena(&game->render_frame_arena);

	#if defined(_WIN32) && defined(m_debug)
	while(g_platform_data->hot_read_index[1] < g_platform_data->hot_write_index) {
		char* path = g_platform_data->hot_file_arr[g_platform_data->hot_read_index[1] % c_max_hot_files];
		if(strstr(path, ".shader")) {
			game->reload_shaders = true;
		}
		g_platform_data->hot_read_index[1] += 1;
	}
	#endif // _WIN32

	if(game->reload_shaders) {
		game->reload_shaders = false;

		for(int i = 0; i < e_shader_count; i += 1) {
			s_shader shader = load_shader_from_file(c_shader_path_arr[i], &game->render_frame_arena);
			if(shader.id > 0) {
				if(game->shader_arr[i].id > 0) {
					gl(glDeleteProgram(game->shader_arr[i].id));
				}
				game->shader_arr[i] = shader;

				#if defined(m_debug)
				printf("Loaded %s\n", c_shader_path_arr[i]);
				#endif // m_debug
			}
		}
	}

	for(int render_pass_i = 0; render_pass_i < c_render_pass_count; render_pass_i += 1) {
		for(int i = 0; i < e_shader_count; i += 1) {
			for(int j = 0; j < e_texture_count; j += 1) {
				for(int k = 0; k < e_mesh_count; k += 1) {
					game->render_pass_arr[render_pass_i].render_group_index_arr[i][j][k] = -1;
				}
			}
		}
		game->render_pass_arr[render_pass_i].render_group_arr.count = 0;
		memset(game->render_pass_arr[render_pass_i].render_instance_count, 0, sizeof(game->render_pass_arr[render_pass_i].render_instance_count));
	}

	bind_framebuffer(0);
	clear_framebuffer_depth(0);

	// {
	// 	if(game->disable_music) {
	// 		game->music_volume.target = 0;
	// 	}
	// 	else {
	// 		game->music_volume.target = 1;
	// 	}
	// 	do_lerpable_snap(&game->music_volume, delta * 5, 0.01f);
	// 	s_active_sound* music = find_playing_sound(e_sound_music);
	// 	if(music) {
	// 		music->data.volume = game->music_volume.curr;
	// 	}
	// }

}

func void do_basic_options(s_container* container, s_v2 button_size)
{
	{
		s_len_str text = format_text("Sound: %s", game->turn_off_all_sounds ? "Off" : "On");
		do_bool_button_ex(text, container_get_pos_and_advance(container), button_size, false, &game->turn_off_all_sounds);
	}

	{
		s_len_str text = format_text("Music: %s", game->disable_music ? "Off" : "On");
		do_bool_button_ex(text, container_get_pos_and_advance(container), button_size, false, &game->disable_music);
	}

	{
		s_len_str text = format_text("Timer: %s", game->hide_timer ? "Off" : "On");
		do_bool_button_ex(text, container_get_pos_and_advance(container), button_size, false, &game->hide_timer);
	}

}

func void attrib_manager_add_float(s_gl_attrib_manager* attrib_manager, int count)
{
	assert(count > 0);
	s_gl_attrib attrib = zero;
	attrib.count = count;
	attrib_manager->attrib_arr.add(attrib);
}

func void attrib_manager_add_float_divisor(s_gl_attrib_manager* attrib_manager, int count)
{
	assert(count > 0);
	s_gl_attrib attrib = zero;
	attrib.count = count;
	attrib.do_divisor = true;
	attrib_manager->attrib_arr.add(attrib);
}

func void attrib_manager_finish(s_gl_attrib_manager* attrib_manager)
{
	assert(attrib_manager->attrib_arr.count > 0);
	u32 stride = 0;
	foreach_val(attrib_i, attrib, attrib_manager->attrib_arr) {
		stride += sizeof(float) * attrib.count;
	}
	u8* offset = 0;
	foreach_val(attrib_i, attrib, attrib_manager->attrib_arr) {
		gl(glVertexAttribPointer(attrib_manager->index, attrib.count, GL_FLOAT, GL_FALSE, stride, offset));
		gl(glEnableVertexAttribArray(attrib_manager->index));
		if(attrib.do_divisor) {
			gl(glVertexAttribDivisor(attrib_manager->index, 1));
		}
		offset += sizeof(float) * attrib.count;
		attrib_manager->index += 1;
	}
	attrib_manager->attrib_arr.count = 0;
}

func void engine_init(s_platform_data* platform_data)
{
	SDL_StartTextInput();

	u8* cursor = platform_data->memory + sizeof(s_game);
	{
		game->arena = make_arena_from_memory(cursor, 10 * c_mb);
		cursor += 10 * c_mb;
	}
	{
		game->update_frame_arena = make_arena_from_memory(cursor, 10 * c_mb);
		cursor += 10 * c_mb;
	}
	{
		game->render_frame_arena = make_arena_from_memory(cursor, 400 * c_mb);
		cursor += 400 * c_mb;
	}
	{
		game->circular_arena = make_circular_arena_from_memory(cursor, 10 * c_mb);
		cursor += 10 * c_mb;
	}

	platform_data->cycle_frequency = SDL_GetPerformanceFrequency();
	platform_data->start_cycles = SDL_GetPerformanceCounter();

	platform_data->window_size.x = (int)c_world_size.x;
	platform_data->window_size.y = (int)c_world_size.y;

	g_platform_data->window = SDL_CreateWindow(
		c_game_name.str, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		(int)c_world_size.x, (int)c_world_size.y, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	SDL_SetWindowPosition(g_platform_data->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(g_platform_data->window);

	#if defined(__EMSCRIPTEN__)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	#endif

	g_platform_data->gl_context = SDL_GL_CreateContext(g_platform_data->window);
	SDL_GL_SetSwapInterval(1);

	#define X(type, name) name = (type)SDL_GL_GetProcAddress(#name); assert(name);
		m_gl_funcs
	#undef X

	{
		gl(glGenBuffers(1, &game->ubo));
		gl(glBindBuffer(GL_UNIFORM_BUFFER, game->ubo));
		gl(glBufferData(GL_UNIFORM_BUFFER, sizeof(s_uniform_data), NULL, GL_DYNAMIC_DRAW));
		gl(glBindBufferBase(GL_UNIFORM_BUFFER, 0, game->ubo));
	}

	{
		constexpr float c_size = 0.5f;
		s_vertex vertex_arr[] = {
			{v3(-c_size, -c_size, 0), v3(0, -1, 0), make_rrr(1), v2(0, 1)},
			{v3(c_size, -c_size, 0), v3(0, -1, 0), make_rrr(1), v2(1, 1)},
			{v3(c_size, c_size, 0), v3(0, -1, 0), make_rrr(1), v2(1, 0)},
			{v3(-c_size, -c_size, 0), v3(0, -1, 0), make_rrr(1), v2(0, 1)},
			{v3(c_size, c_size, 0), v3(0, -1, 0), make_rrr(1), v2(1, 0)},
			{v3(-c_size, c_size, 0), v3(0, -1, 0), make_rrr(1), v2(0, 0)},
		};
		game->mesh_arr[e_mesh_quad] = make_mesh_from_vertices(vertex_arr, array_count(vertex_arr));
	}

	game->mesh_arr[e_mesh_plane] = make_plane_mesh();

	{
		game->mesh_arr[e_mesh_cube] = make_mesh_from_obj_file("assets/cube.obj", &game->render_frame_arena);
		game->mesh_arr[e_mesh_sphere] = make_mesh_from_obj_file("assets/sphere.obj", &game->render_frame_arena);
	}

	{
		s_mesh* mesh = &game->mesh_arr[e_mesh_line];
		mesh->vertex_count = 6;
		gl(glGenVertexArrays(1, &mesh->vao));
		gl(glBindVertexArray(mesh->vao));

		gl(glGenBuffers(1, &mesh->instance_vbo.id));
		gl(glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo.id));

		s_gl_attrib_manager attrib_manager = zero;
		attrib_manager_add_float_divisor(&attrib_manager, 2); // line start
		attrib_manager_add_float_divisor(&attrib_manager, 2); // line end
		attrib_manager_add_float_divisor(&attrib_manager, 1); // line width
		attrib_manager_add_float_divisor(&attrib_manager, 4); // color
		attrib_manager_finish(&attrib_manager);
	}

	for(int i = 0; i < e_sound_count; i += 1) {
		platform_data->sound_arr[i] = platform_data->load_sound_from_file(c_sound_data_arr[i].path);
	}

	for(int i = 0; i < e_texture_count; i += 1) {
		char* path = c_texture_path_arr[i];
		if(strlen(path) > 0) {
			u32 filter = GL_NEAREST;
			game->texture_arr[i] = load_texture_from_file(path, filter);
		}
	}

	game->font = load_font_from_file("assets/Inconsolata-Regular.ttf", 128, &game->render_frame_arena);

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		atlas start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		game->atlas.texture = e_texture_atlas;
		game->atlas.texture_size = v2i(256, 256);
		game->atlas.sprite_size = v2i(16, 16);

		game->atlas2.texture = e_texture_atlas2;
		game->atlas2.texture_size = v2i(640, 640);
		game->atlas2.sprite_size = v2i(64, 64);
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		atlas end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	{
		u32* texture = &game->texture_arr[e_texture_light].id;
		game->light_fbo.size.x = (int)c_world_size.x;
		game->light_fbo.size.y = (int)c_world_size.y;
		gl(glGenFramebuffers(1, &game->light_fbo.id));
		bind_framebuffer(game->light_fbo.id);
		gl(glGenTextures(1, texture));
		gl(glBindTexture(GL_TEXTURE_2D, *texture));
		gl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, game->light_fbo.size.x, game->light_fbo.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
		gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		gl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0));
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		bind_framebuffer(0);
	}
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

	add_to_render_group(data, e_shader_billboard, e_texture_white, e_mesh_quad, render_pass_index);
}

func void draw_billboard_ex(s_atlas atlas, s_v3 pos, s_v2 size, s_v2i index, s_v4 color, float rotation, s_draw_data draw_data, int render_pass_index)
{
	s_instance_data data = zero;
	data.model = m4_translate(pos);
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

	add_to_render_group(data, e_shader_billboard, atlas.texture, e_mesh_quad, render_pass_index);
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

func void do_basic_render_flush(s_m4 ortho, int render_pass_index, s_m4 view_inv)
{
	s_render_flush_data data = make_render_flush_data(zero, zero, view_inv);
	data.projection = ortho;
	data.blend_mode = e_blend_mode_normal;
	data.depth_mode = e_depth_mode_no_read_no_write;
	render_flush(data, true, render_pass_index);
}

func s_v3 compute_initial_velocity(s_v3 start_pos, s_v3 target_pos, float total_time, float gravity)
{
	s_v3 result;

	result.x = (target_pos.x - start_pos.x) / total_time;
	result.y = (target_pos.y - start_pos.y) / total_time;
	result.z = (target_pos.z - start_pos.z + 0.5f * gravity * total_time * total_time) / total_time;

	return result;
}

func s_time_data get_time_data_maybe(float curr, s_maybe<float> timestamp, float duration)
{
	s_time_data result = zero;
	if(timestamp.valid) {
		result = get_time_data(curr, timestamp.value, duration);
	}
	return result;
}
