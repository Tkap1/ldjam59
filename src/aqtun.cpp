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
	if (x == 0) {
		do_x_check = false;
	}
	if(y == 0) {
		do_y_check = false;
	}

	bool top_left_check     = do_x_check &&                 map->active[y    ][x - 1] && map->entity_arr[y    ][x - 1].type == e_entity_wall;
	bool top_right_check    =                               map->active[y    ][x    ] && map->entity_arr[y    ][x    ].type == e_entity_wall;
	bool bottom_right_check = do_y_check &&                 map->active[y - 1][x    ] && map->entity_arr[y - 1][x    ].type == e_entity_wall;
	bool bottom_left_check  = (do_x_check && do_y_check) && map->active[y - 1][x - 1] && map->entity_arr[y - 1][x - 1].type == e_entity_wall;

	if (top_left_check)     result.tile_index |= c_dualgrid_tile_top_left;
	if (top_right_check)    result.tile_index |= c_dualgrid_tile_top_right;
	if (bottom_right_check) result.tile_index |= c_dualgrid_tile_bottom_right;
	if (bottom_left_check)  result.tile_index |= c_dualgrid_tile_bottom_left;

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