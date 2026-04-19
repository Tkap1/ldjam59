global constexpr u8 c_dualgrid_tile_empty        = 0b0000;
global constexpr u8 c_dualgrid_tile_top_left     = 0b0001;
global constexpr u8 c_dualgrid_tile_top_right    = 0b0010;
global constexpr u8 c_dualgrid_tile_bottom_right = 0b0100;
global constexpr u8 c_dualgrid_tile_bottom_left  = 0b1000;
global constexpr u8 c_dualgrid_tile_full         = 0b1111;

constexpr float c_tile_rotations[4] = {
	c_pi * 0.5f,
	c_pi,
	c_pi * 1.5f,
	0,
};

struct s_tile_visual
{
	u8 tile_index;
	u8 rotation_index;
};

func u8 rotate_tile_bits(u8 value)
{
	constexpr int c_l_shift = 1;
	constexpr int c_right_shift = 4 - c_l_shift;

	u8 last_4_bits = (u8)(value & 0x0F);
	return (u8)(0x0F & ((last_4_bits << c_l_shift) | (last_4_bits >> c_right_shift)));
}

func s_tile_visual find_dominant_tile(s_tile_visual tile)
{
		if (tile.tile_index == c_dualgrid_tile_empty || tile.tile_index >= c_dualgrid_tile_full) return tile;

		u8 max = 0;
		for (u8 i = 0; i < 4; i++)
		{
				if (tile.tile_index > max)
				{
						max = tile.tile_index;
						tile.rotation_index = i;
				}
				tile.tile_index = rotate_tile_bits(tile.tile_index);
		}
		tile.tile_index = max;

		return tile;
}

func s_tile_visual get_tile_index(s_map* map, int x, int y)
{
	s_tile_visual result = {};

	b8 do_x_check = true;
	b8 do_y_check = true;
	b8 do_base_check = true;
	if(x >= c_map_width) {
		do_base_check = false;
	}
	if (x == 0) {
		do_x_check = false;
	}
	if(y == 0) {
		do_y_check = false;
	}

	s_v2i offset_arr[4] = {
		v2i(x - 1, y),
		v2i(x, y),
		v2i(x, y - 1),
		v2i(x - 1, y - 1),
	};
	b8 do_arr[4] = {
		do_x_check,
		do_base_check,
		do_base_check && do_y_check,
		do_x_check && do_y_check
	};

	b8 check_arr[4] = zero;

	for(int i = 0; i < 4; i += 1) {
		if(do_arr[i]) {
			s_v2i index = offset_arr[i];
			check_arr[i] = map->active[index.y][index.x] && map->entity_arr[index.y][index.x].type == e_entity_wall && !map->entity_arr[index.y][index.x].sub_type == 1;
		}
	}

	if (check_arr[0])     result.tile_index |= c_dualgrid_tile_top_left;
	if (check_arr[1])    result.tile_index |= c_dualgrid_tile_top_right;
	if (check_arr[2]) result.tile_index |= c_dualgrid_tile_bottom_right;
	if (check_arr[3])  result.tile_index |= c_dualgrid_tile_bottom_left;

	result = find_dominant_tile(result);

	// lookup table would be faster than a jump table
	switch (result.tile_index)
	{
			case 0:
					result.tile_index = 0; // empty
					break;
			case 8:
					result.tile_index = 1; // corner
					break;
			case 10:
					result.tile_index = 2; // diagonal
					break;
			case 12:
					result.tile_index = 3; // side
					break;
			case 14:
					result.tile_index = 4; // l-shape
					break;
			case 15:
					result.tile_index = 5; // full
					break;
	}

	return result;
}