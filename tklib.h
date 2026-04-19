
#include <stdio.h>

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

#pragma warning(push)
#pragma warning(disable: 4505) // @Note(tkap, 12/04/2025): "unreferenced function with internal linkage has been removed"
#pragma warning(disable: 4514) // @Note(tkap, 12/04/2025): "unreferenced inline function has been removed"

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		macros start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// nocheckin
#define invalid_else else { assert(!"Invalid else"); }
#define invalid_default_case default: { assert(!"Invalid default case"); }
#define assert(cond) if(!(cond)) { on_failed_assert(#cond, __FILE__, __LINE__); }

#define xcase break; case

#define array_count(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define foreach_ptr__(a, index_name, element_name, array) if(0) finished##a: ; else for(auto element_name = &(array).data[0];;) if(1) goto body##a; else while(1) if(1) goto finished##a; else body##a: for(int index_name = 0; index_name < (array).count && (bool)(element_name = &(array)[index_name]); index_name++)
#define foreach_ptr_(a, index_name, element_name, array) foreach_ptr__(a, index_name, element_name, array)
#define foreach_ptr(index_name, element_name, array) foreach_ptr_(__LINE__, index_name, element_name, array)

#define foreach_val__(a, index_name, element_name, array) if(0) finished##a: ; else for(auto element_name = (array).data[0];;) if(1) goto body##a; else while(1) if(1) goto finished##a; else body##a: for(int index_name = 0; index_name < (array).count && (void*)&(element_name = (array)[index_name]); index_name++)
#define foreach_val_(a, index_name, element_name, array) foreach_val__(a, index_name, element_name, array)
#define foreach_val(index_name, element_name, array) foreach_val_(__LINE__, index_name, element_name, array)

#define func static
#define global static
#define zero {}
#define null nullptr
#define data_enum(...)

#define m_advanced_easings \
X(ease_linear, e_ease_linear) \
X(ease_in_expo, e_ease_in_expo) \
X(ease_in_quad, e_ease_in_quad) \
X(ease_out_quad, e_ease_out_quad) \
X(ease_out_expo, e_ease_out_expo) \
X(ease_out_elastic, e_ease_out_elastic) \
X(ease_out_elastic2, e_ease_out_elastic2) \
X(ease_out_back, e_ease_out_back)

#define for_enum(mname, menum) for(menum mname = {}; mname < menum##_count; mname = (menum)(mname + 1))
#define expand_str(mstr) (mstr).count, (mstr).str


#if defined(__SANITIZE_ADDRESS__)
#define poison_memory(memory, size) __asan_poison_memory_region(memory, size);
#define unpoison_memory(memory, size) __asan_unpoison_memory_region(memory, size);
#else
#define poison_memory(memory, size);
#define unpoison_memory(memory, size);
#endif

#define breakable_block for(int _ignored_ = 1; _ignored_ > 0; _ignored_ -= 1)


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		macros end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		typedefs start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef double f64;
typedef float f32;

typedef bool b8;
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		typedefs end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		constants start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
global constexpr u64 c_max_u8 = UINT8_MAX;
global constexpr u64 c_max_u16 = UINT16_MAX;
global constexpr u64 c_max_u32 = UINT32_MAX;
global constexpr u64 c_max_u64 = UINT64_MAX;

global constexpr u64 c_max_s8 = INT8_MAX;
global constexpr u64 c_max_s16 = INT16_MAX;
global constexpr u64 c_max_s32 = INT32_MAX;
global constexpr u64 c_max_s64 = INT64_MAX;

global constexpr float c_pi = 3.1415926f;
global constexpr float c_tau = 6.2831853071f;

global constexpr int c_kb = 1024;
global constexpr int c_mb = c_kb * 1024;
global constexpr int c_gb = c_mb * 1024;

global constexpr float epsilon = 0.000001f;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		constants end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		enums start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
enum e_token
{
	e_token_invalid,
	e_token_identifier,
	e_token_operator,
	e_token_int,
	e_token_eof,
};

#define X(a, b) b,
enum e_ease
{
	m_advanced_easings
	e_ease_count,
};
#undef X

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		enums end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		structs start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
union s_v2
{
	struct {
		float x;
		float y;
	};
	struct {
		float r;
		float g;
	};
	float all[2];
};

union s_v3
{
	struct {
		float x;
		float y;
		float z;
	};
	struct {
		float r;
		float g;
		float b;
	};
	struct {
		s_v2 xy;
		float z0;
	};
	struct {
		s_v2 rg;
		float z1;
	};
	float all[3];
};

union s_v4
{
	struct {
		float x;
		float y;
		float z;
		float w;
	};
	struct {
		float r;
		float g;
		float b;
		float a;
	};
	struct {
		s_v3 xyz;
		float w0;
	};
	struct {
		s_v3 rgb;
		float a0;
	};
	struct {
		s_v2 xy;
		float z1;
		float w1;
	};
	float all[4];
};

union s_rect
{
	struct {
		float x;
		float y;
		float w;
		float h;
	};
	struct {
		s_v2 pos;
		s_v2 size;
	};
};

struct s_v2i
{
	int x;
	int y;
};

template <typename t>
struct s_maybe
{
	b8 valid;
	t value;
};

struct s_len_str
{
	int count;
	char* str;

	inline char operator[](int index);
};

struct s_memory
{
	int size;
	u8* ptr;
};

struct s_time_data
{
	float passed;
	float percent;
	float inv_percent;
};

template <int n>
struct s_str_builder
{
	int count;
	char str[n + 1]; // @Note(tkap, 12/04/2025): +1 because in vsnprintf will write null character, but we don't deal with null chars
};

union s_m4
{
	float all[16];
	float all2[4][4];
};

struct s_linear_arena
{
	int used;
	int capacity;
	u8* memory;
};

struct s_circular_arena
{
	int capacity;
	int used;
	u8* memory;
};

struct s_rng
{
	u64 seed;
};

struct s_tokenizer
{
	s_len_str at;
	int line;
};

struct s_token
{
	e_token type;
	s_len_str str;
	int line;
};

template <typename t, int n>
struct s_list
{
	// int count = 0; // nocheckin
	int count;
	t data[n];

	t& operator[](int index);
	t pop_last();
	t* add(t new_element);
	void add_array(t* arr, int count);
	void remove_and_swap(int index);
	void remove_and_shift(int index);
	t get_last();
	b8 is_full();
	void add_if_not_full(t new_element);
	void insert(int where, t new_element);
};

template <typename t, int n0>
struct s_array
{
	t data[n0];
	t& operator[](int index);
};

template <typename t, int n0, int n1>
struct s_array2d
{
	s_array<t, n1> data[n0];
	s_array<t, n1>& operator[](int index);
};

struct s_ray
{
	s_v3 pos;
	s_v3 dir;
};

struct s_temp_state
{
	b8 is_temporary;
	int id;
};

struct s_state_transition
{
	b8 active;
	s_time_data time_data;
};

struct s_state
{
	b8 clear_state;
	b8 do_pop;
	s_list<s_temp_state, 8> stack;
	s_maybe<s_temp_state> next_state;
	float transition_timestamp;
	float transition_duration;
};

struct s_animator
{
	#if defined(m_debug)
	b8 can_add_keyframe;
	#endif // m_debug

	int step_count;
	float step_start_time_arr[8];
	float step_end_time_arr[8];
};

#if defined(m_winhttp)
struct s_http_request
{
	int status_code;
	s_memory memory;
};
#endif



// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		structs end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		functions start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

func void on_failed_assert(char* condition, char* file, int line)
{
	printf("FAILED ASSERT '%s'\n  %s(%i)\n", condition, file, line);

	#if defined(_WIN32)
	__debugbreak();
	#else
	__builtin_trap();
	#endif
}

template <typename t>
func void zero_struct(t* data)
{
	memset(data, 0, sizeof(t));
}

func void zero_memory(void* memory, int size)
{
	memset(memory, 0, size);
}

template <size_t T>
func constexpr s_len_str S(const char (&str)[T])
{
	s_len_str result;
	result.count = T - 1;
	result.str = (char*)str;
	return result;
}

func constexpr s_len_str S(char* str)
{
	assert(str);
	s_len_str result;
	result.str = str;
	result.count = 0;
	while(str[result.count] != '\0') {
		result.count += 1;
	}
	return result;
}

inline char s_len_str::operator[](int index)
{
	char result = '\0';
	if(index < this->count) {
		result = this->str[index];
	}
	return result;
}

func u8* arena_alloc(s_linear_arena* arena, int requested_size)
{
	assert(requested_size > 0);
	int size = (requested_size + 7) & ~7;
	assert(arena->used + size <= arena->capacity);
	u8* result = arena->memory + arena->used;
	unpoison_memory(result, requested_size);
	arena->used += size;
	return result;
}

func u8* arena_alloc_zero(s_linear_arena* arena, int requested_size)
{
	u8* result = arena_alloc(arena, requested_size);
	zero_memory(result, requested_size);
	return result;
}

func s_circular_arena make_circular_arena_from_memory(u8* memory, int requested_capacity)
{
	int capacity = (requested_capacity + 7) & ~7;
	s_circular_arena result = zero;
	result.capacity = capacity;
	result.memory = memory;
	return result;
}

func u8* circular_arena_alloc(s_circular_arena* arena, int requested_size)
{
	assert(requested_size > 0);
	int size = (requested_size + 7) & ~7;
	assert(size <= arena->capacity);

	int space_left = arena->capacity - arena->used;
	b8 fits = space_left >= size;
	u8* result = null;
	if(fits) {
		result = arena->memory + arena->used;
	}
	else {
		arena->used = 0;
		result = arena->memory;
	}

	arena->used += size;
	return result;
}

func u8* circular_arena_alloc_zero(s_circular_arena* arena, int requested_size)
{
	u8* result = circular_arena_alloc(arena, requested_size);
	zero_memory(result, requested_size);
	return result;
}

func u8* read_file(char* path, s_linear_arena* arena, int* out_size)
{
	FILE* file = fopen(path, "rb");
	u8* result = null;
	if(file) {
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		if(out_size) {
			*out_size = size;
		}
		fseek(file, 0, SEEK_SET);
		result = arena_alloc(arena, size + 1);
		fread(result, 1, size, file);
		fclose(file);
		result[size] = '\0';
	}
	return result;
}

func void write_file(char* path, void* data, int data_size)
{
	FILE* file = fopen(path, "wb");
	assert(file);
	fwrite(data, data_size, 1, file);
	fclose(file);
}

func s_rect do_letterbox(s_v2 curr_size, s_v2 base_size)
{
	s_rect rect_result = zero;
	rect_result.size = curr_size;
	float curr_ar = curr_size.x / curr_size.y;
	float base_ar = base_size.x / base_size.y;

	// @Note(tkap, 30/12/2024): We are too wide
	if(curr_ar > base_ar) {
		rect_result.w = base_ar / curr_ar * curr_size.x;
		rect_result.x = (curr_size.x - rect_result.w) * 0.5f;
	}
	// @Note(tkap, 30/12/2024): We are too tall
	else if(base_ar > curr_ar) {
		rect_result.h = curr_ar / base_ar * curr_size.y;
		rect_result.y = (curr_size.y - rect_result.h) * 0.5f;
	}
	return rect_result;
}

template <typename t>
func void toggle(t* out, t a, t b)
{
	if(*out == a) {
		*out = b;
	}
	else if(*out == b) {
		*out = a;
	}
	invalid_else;
}

template <typename t>
func t toggle(t val, t a, t b)
{
	toggle(&val, a, b);
	return val;
}

template <typename t>
constexpr func s_maybe<t> maybe()
{
	s_maybe<t> result = zero;
	return result;
}

template <typename t>
constexpr func s_maybe<t> maybe(t value)
{
	s_maybe<t> result = zero;
	result.valid = true;
	result.value = value;
	return result;
}

template <typename t>
constexpr func void set_maybe_if_invalid(s_maybe<t>* m, t new_value)
{
	if(!m->valid) {
		*m = maybe(new_value);
	}
}

[[nodiscard]] func s_len_str substr_from_to_exclusive(s_len_str x, int start, int end)
{
	assert(start >= 0);
	assert(end > start);
	s_len_str result = zero;
	result.str = x.str + start;
	result.count = end - start;
	return result;
}

func b8 is_number(char c)
{
	return c >= '0' && c <= '9';
}

func b8 is_alpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

func b8 is_alpha_numeric(char c)
{
	return is_number(c) || is_alpha(c);
}

func b8 can_start_identifier(char c)
{
	return is_alpha(c) || c == '_';
}

func b8 can_continue_identifier(char c)
{
	return is_alpha(c) || is_number(c) || c == '_';
}

func float square(float x)
{
	float result = x * x;
	return result;
}

func float square_if_positive_else_zero(float x)
{
	float result;
	if(x > 0) {
		result = x * x;
	}
	else {
		result = 0;
	}
	return result;
}

func float v3_length_squared(s_v3 v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

func float v3_length(s_v3 v)
{
	return sqrtf(v3_length_squared(v));
}

func s_v3 v3_normalized(s_v3 v)
{
	s_v3 result = v;
	float length = v3_length(v);
	if(length != 0) {
		result.x /= length;
		result.y /= length;
		result.z /= length;
	}
	return result;
}

#pragma warning(push, 0)
// @Note(tkap, 12/06/2024): https://www.pcg-random.org/download.html
[[nodiscard]] func u32 randu(s_rng* rng)
{
	uint64_t oldstate = rng->seed;
	// Advance internal state
	rng->seed = oldstate * 6364136223846793005ULL + 1;
	// Calculate output function (XSH RR), uses old state for max ILP
	uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
#pragma warning(pop)

[[nodiscard]] func s_rng make_rng(u64 seed)
{
	s_rng rng = {};
	u32 _ = randu(&rng);
	rng.seed += seed;
	_ = randu(&rng);
	return rng;
}

[[nodiscard]] func float randf32(s_rng* rng)
{
	float result = (float)randu(rng) / (float)4294967295;
	return result;
}

[[nodiscard]] func float randf_range(s_rng* rng, float min_val, float max_val)
{
	float r = randf32(rng);
	return min_val + (max_val - min_val) * r;
}


func s_v4 rand_color(s_rng* rng)
{
	s_v4 result;
	result.r = randf32(rng);
	result.g = randf32(rng);
	result.b = randf32(rng);
	result.a = 1;
	return result;
}

func s_v4 rand_color_normalized(s_rng* rng)
{
	s_v4 result;
	result.r = randf32(rng);
	result.g = randf32(rng);
	result.b = randf32(rng);
	result.rgb = v3_normalized(result.rgb);
	result.a = 1;
	return result;
}

template <typename t0, typename t1, typename t2>
func s_v3 v3(t0 x, t1 y, t2 z)
{
	s_v3 result = zero;
	result.x = (float)x;
	result.y = (float)y;
	result.z = (float)z;
	return result;
}

template <typename t0>
func s_v3 v3(s_v2 xy, t0 z)
{
	s_v3 result = zero;
	result.x = xy.x;
	result.y = xy.y;
	result.z = (float)z;
	return result;
}

template <typename t0>
func s_v3 v3(t0 x)
{
	s_v3 result = zero;
	result.x = (float)x;
	result.y = (float)x;
	result.z = (float)x;
	return result;
}

func s_v4 v4(float x, float y, float z, float w)
{
	s_v4 result;
	result.x = x;
	result.y = y;
	result.z = z;
	result.w = w;
	return result;
}

template <typename t0, typename t1>
func s_v2 v2(t0 x, t1 y)
{
	s_v2 result = zero;
	result.x = (float)x;
	result.y = (float)y;
	return result;
}

template <typename t0>
func s_v2 v2(t0 x)
{
	s_v2 result = zero;
	result.x = (float)x;
	result.y = (float)x;
	return result;
}

func s_v2 v2(s_v2i v)
{
	s_v2 result = zero;
	result.x = (float)v.x;
	result.y = (float)v.y;
	return result;
}

func float v2_length(s_v2 a)
{
	return sqrtf(a.x * a.x + a.y * a.y);
}

func s_v2 operator+(s_v2 a, s_v2 b)
{
	return v2(
		a.x + b.x,
		a.y + b.y
	);
}

func void operator+=(s_v2& a, s_v2 b)
{
	a.x += b.x;
	a.y += b.y;
}

func s_v3 random_point_in_sphere(s_rng* rng, float radius)
{
	s_v3 pos;
	while(true) {
		pos = v3(
			randf_range(rng, -radius, radius),
			randf_range(rng, -radius, radius),
			randf_range(rng, -radius, radius)
		);
		float d = v3_length(pos);
		if(d <= radius) { break; }
	}
	return pos;
}

func s_v2 random_point_in_circle(s_rng* rng, float radius)
{
	s_v2 pos;
	while(true) {
		pos = v2(
			randf_range(rng, -radius, radius),
			randf_range(rng, -radius, radius)
		);
		float d = v2_length(pos);
		if(d <= radius) { break; }
	}
	return pos;
}

[[nodiscard]] func float randf32_11(s_rng* rng)
{
	float result = randf32(rng) * 2 - 1;
	return result;
}

func s_v2 random_point_in_rect_center(s_rng* rng, s_v2 size)
{
	s_v2 pos;
	pos.x = randf32_11(rng) * size.x * 0.5f;
	pos.y = randf32_11(rng) * size.y * 0.5f;
	return pos;
}

func s_time_data get_time_data(float curr, float timestamp, float duration)
{
	s_time_data result = zero;
	result.passed = curr - timestamp;
	if(duration <= 0) {
		result.percent = 1;
	}
	else {
		result.percent = result.passed / duration;
	}
	result.inv_percent = 1.0f - result.percent;
	return result;
}

func s_v2i v2i(int x, int y)
{
	s_v2i result;
	result.x = x;
	result.y = y;
	return result;
}

func s_v2 operator*(s_v2 a, float b)
{
	return v2(
		a.x * b,
		a.y * b
	);
}

func s_v2 operator*(s_v2 a, s_v2 b)
{
	return v2(
		a.x * b.x,
		a.y * b.y
	);
}

func s_v2 operator*(s_v2 a, s_v2i b)
{
	return v2(
		a.x * b.x,
		a.y * b.y
	);
}

func s_v2i operator+(s_v2i a, s_v2i b)
{
	return v2i(
		a.x + b.x,
		a.y + b.y
	);
}

func void operator+=(s_v2i& a, s_v2i b)
{
	a.x += b.x;
	a.y += b.y;
}

func s_v2 operator-(s_v2 a, s_v2 b)
{
	return v2(
		a.x - b.x,
		a.y - b.y
	);
}

func s_v3 operator+(s_v3 a, s_v3 b)
{
	return v3(
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	);
}

func s_v3 operator*(s_v3 a, float b)
{
	return v3(
		a.x * b,
		a.y * b,
		a.z * b
	);
}

func s_v2 operator/(s_v2 a, float b)
{
	return v2(
		a.x / b,
		a.y / b
	);
}

func s_v3 operator/(s_v3 a, float b)
{
	return v3(
		a.x / b,
		a.y / b,
		a.z / b
	);
}

func b8 rect_vs_rect_topleft(s_v2 pos0, s_v2 size0, s_v2 pos1, s_v2 size1)
{
	b8 result = pos0.x + size0.x > pos1.x && pos0.x < pos1.x + size1.x &&
		pos0.y + size0.y > pos1.y && pos0.y < pos1.y + size1.y;
	return result;
}

func b8 rect_vs_rect_center(s_v2 pos0, s_v2 size0, s_v2 pos1, s_v2 size1)
{
	b8 result = rect_vs_rect_topleft(pos0 - size0 * 0.5f, size0, pos1 - size1 * 0.5f, size1);
	return result;
}

func b8 mouse_vs_rect_topleft(s_v2 mouse, s_v2 pos, s_v2 size)
{
	b8 result = rect_vs_rect_topleft(mouse, v2(1), pos, size);
	return result;
}

func b8 mouse_vs_rect_center(s_v2 mouse, s_v2 pos, s_v2 size)
{
	b8 result = rect_vs_rect_center(mouse, v2(1), pos, size);
	return result;
}

template <int n>
func void str_into_builder(s_str_builder<n>* builder, s_len_str str)
{
	assert(str.count <= n);
	memcpy(builder->str, str.str, str.count);
	builder->count = str.count;
}

template <int n>
func s_len_str builder_to_len_str(s_str_builder<n>* builder)
{
	s_len_str result = zero;
	result.str = builder->str;
	result.count = builder->count;
	return result;
}

template <int n>
func s_len_str builder_to_len_str_alloc(s_str_builder<n>* builder, s_linear_arena* arena)
{
	s_len_str result = zero;
	result.str = (char*)arena_alloc(arena, builder->count);
	memcpy(result.str, builder->str, builder->count);
	result.count = builder->count;
	return result;
}

template <int n>
func char* builder_to_cstr(s_str_builder<n>* builder, s_circular_arena* arena)
{
	char* result = (char*)circular_arena_alloc(arena, builder->count + 1);
	memcpy(result, builder->str, builder->count);
	result[builder->count] = '\0';
	return result;
}

template <int n0, int n1>
func b8 builder_equals(s_str_builder<n0>* a, s_str_builder<n1>* b)
{
	b8 result = a->count == b->count && memcmp(a->str, b->str, a->count) == 0;
	return result;
}

template <int n>
func b8 is_builder_full(s_str_builder<n>* builder)
{
	b8 result = builder->count >= n;
	return result;
}

template <int n>
func void builder_insert_char(s_str_builder<n>* builder, int index, char c)
{
	assert(index >= 0);
	assert(index <= builder->count);
	int num_to_the_right = builder->count - index;
	if(num_to_the_right > 0) {
		memmove(&builder->str[index + 1], &builder->str[index], num_to_the_right);
	}
	builder->str[index] = c;
	builder->count += 1;
}

template <int n>
func void builder_remove_char_at(s_str_builder<n>* builder, int index)
{
	assert(index >= 0);
	assert(index < builder->count);

	builder->count -= 1;
	memmove(&builder->str[index], &builder->str[index + 1], builder->count - index);
	builder->str[builder->count] = 0;
}

template <int n>
func void builder_add(s_str_builder<n>* builder, char* format, ...)
{
	va_list args;
	va_start(args, format);
	int written = vsnprintf(&builder->str[builder->count], n - builder->count + 1, format, args);
	assert(written > 0);
	builder->count += written;
	assert(builder->count <= n);
	va_end(args);
}

func s_m4 m4_multiply(s_m4 a, s_m4 b)
{
	s_m4 result = zero;
	for(int i = 0; i < 4; i += 1) {
		for(int j = 0; j < 4; j += 1) {
			for(int k = 0; k < 4; k += 1) {
				result.all2[j][i] += a.all2[k][i] * b.all2[j][k];
			}
		}
	}
	return result;
}

func s_m4 m4_scale(s_v3 v)
{
	s_m4 result = {
		v.x, 0, 0, 0,
		0, v.y, 0, 0,
		0, 0, v.z, 0,
		0, 0, 0, 1,
	};
	return result;
}

func s_m4 operator*(s_m4 a, s_m4 b)
{
	s_m4 result = m4_multiply(a, b);
	return result;
}

func void operator*=(s_m4& a, s_m4 b)
{
	a = m4_multiply(a, b);
}

func void scale_m4_by_radius(s_m4* out, float radius)
{
	*out = m4_multiply(*out, m4_scale(v3(radius * 2)));
}

func s_m4 m4_translate(s_v3 v)
{
	s_m4 result = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		v.x, v.y, v.z, 1,
	};
	return result;
}

func s_m4 m4_identity()
{
	s_m4 result = zero;
	result.all2[0][0] = 1;
	result.all2[1][1] = 1;
	result.all2[2][2] = 1;
	result.all2[3][3] = 1;
	return result;
}

func s_m4 make_perspective(float fov, float aspect_ratio, float near, float far)
{
	s_m4 result = zero;

	// See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

	float Cotangent = 1.0f / tanf(fov * (c_pi / 360.0f));

	result.all2[0][0] = Cotangent / aspect_ratio;

	result.all2[1][1] = Cotangent;

	result.all2[2][3] = -1.0f;
	result.all2[2][2] = (near + far) / (near - far);
	result.all2[3][2] = (2.0f * near * far) / (near - far);
	result.all2[3][3] = 0.0f;

	return result;
}

func s_v3 operator-(s_v3 a, s_v3 b)
{
	return v3(
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	);
}

func s_v3 v3_cross(s_v3 a, s_v3 b)
{
	s_v3 Result;

	Result.x = (a.y * b.z) - (a.z * b.y);
	Result.y = (a.z * b.x) - (a.x * b.z);
	Result.z = (a.x * b.y) - (a.y * b.x);

	return (Result);
}

func float v3_dot(s_v3 a, s_v3 b)
{
	float Result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
	return (Result);
}

func s_m4 look_at(s_v3 eye, s_v3 target, s_v3 up)
{
	s_m4 world_to_cam = zero;
	s_v3 front = v3_normalized(target - eye);
	s_v3 side = v3_normalized(v3_cross(front, up));
	s_v3 top = v3_normalized(v3_cross(side, front));

	world_to_cam.all[0] = side.x;
	world_to_cam.all[1] = top.x;
	world_to_cam.all[2] = -front.x;
	world_to_cam.all[3] = 0;

	world_to_cam.all[4] = side.y;
	world_to_cam.all[5] = top.y;
	world_to_cam.all[6] = -front.y;
	world_to_cam.all[7] = 0;

	world_to_cam.all[8] = side.z;
	world_to_cam.all[9] = top.z;
	world_to_cam.all[10] = -front.z;
	world_to_cam.all[11] = 0;

	s_v3 x = v3(world_to_cam.all[0], world_to_cam.all[4], world_to_cam.all[8]);
	s_v3 y = v3(world_to_cam.all[1], world_to_cam.all[5], world_to_cam.all[9]);
	s_v3 z = v3(world_to_cam.all[2], world_to_cam.all[6], world_to_cam.all[10]);

	world_to_cam.all[12] = -v3_dot(x, eye);
	world_to_cam.all[13] = -v3_dot(y, eye);
	world_to_cam.all[14] = -v3_dot(z, eye);
	world_to_cam.all[15] = 1.0f;

	return world_to_cam;
}

func s_m4 m4_rotate(float angle, s_v3 axis)
{

	s_m4 result = m4_identity();

	axis = v3_normalized(axis);

	float SinTheta = sinf(angle);
	float CosTheta = cosf(angle);
	float CosValue = 1.0f - CosTheta;

	result.all2[0][0] = (axis.x * axis.x * CosValue) + CosTheta;
	result.all2[0][1] = (axis.x * axis.y * CosValue) + (axis.z * SinTheta);
	result.all2[0][2] = (axis.x * axis.z * CosValue) - (axis.y * SinTheta);

	result.all2[1][0] = (axis.y * axis.x * CosValue) - (axis.z * SinTheta);
	result.all2[1][1] = (axis.y * axis.y * CosValue) + CosTheta;
	result.all2[1][2] = (axis.y * axis.z * CosValue) + (axis.x * SinTheta);

	result.all2[2][0] = (axis.z * axis.x * CosValue) + (axis.y * SinTheta);
	result.all2[2][1] = (axis.z * axis.y * CosValue) - (axis.x * SinTheta);
	result.all2[2][2] = (axis.z * axis.z * CosValue) + CosTheta;

	return result;
}

func s_v3 get_pos_from_m4(s_m4 m)
{
	s_v3 pos = v3(
		m.all2[3][0],
		m.all2[3][1],
		m.all2[3][2]
	);
	return pos;
}

func s_linear_arena make_arena_from_memory(u8* memory, int requested_capacity)
{
	int capacity = (requested_capacity + 7) & ~7;
	s_linear_arena result = zero;
	result.capacity = capacity;
	result.memory = memory;
	poison_memory(memory, capacity);
	return result;
}

func s_linear_arena make_arena_from_malloc(int requested_capacity)
{
	int capacity = (requested_capacity + 7) & ~7;
	u8* memory = (u8*)malloc(capacity);
	s_linear_arena result = make_arena_from_memory(memory, capacity);
	return result;
}

template <typename t, int n>
t& s_list<t, n>::operator[](int index)
{
	assert(index >= 0);
	assert(index < this->count);
	return this->data[index];
}

template <typename t, int n>
t s_list<t, n>::pop_last()
{
	assert(this->count > 0);
	this->count -= 1;
	t result = this->data[this->count];
	return result;
}

template <typename t, int n>
t* s_list<t, n>::add(t new_element)
{
	assert(this->count < n);
	t* result = &this->data[this->count];
	this->data[this->count] = new_element;
	this->count += 1;
	return result;
}

template <typename t, int n>
void s_list<t, n>::add_array(t* arr, int other_count)
{
	assert(this->count + other_count <= n);
	memcpy(&this->data[this->count + 1], arr, other_count * sizeof(t));
	this->count += other_count;
}

template <typename t, int n>
void s_list<t, n>::remove_and_swap(int index)
{
	assert(index < this->count);
	this->count -= 1;
	this->data[index] = this->data[this->count];
}

template <typename t, int n>
t s_list<t, n>::get_last()
{
	assert(this->count > 0)
	t result = data[this->count - 1];
	return result;
}

template <typename t, int n>
b8 s_list<t, n>::is_full()
{
	b8 result = this->count >= n;
	return result;
}

template <typename t, int n>
void s_list<t, n>::add_if_not_full(t new_element)
{
	if(!this->is_full()) {
		this->add(new_element);
	}
}

template <typename t, int n>
void s_list<t, n>::insert(int where, t new_element)
{
	assert(where >= 0);
	assert(where <= this->count);
	assert(this->count < n);
	int elements_right = this->count - where;
	if(elements_right > 0) {
		memmove(&this->data[where + 1], &this->data[where], sizeof(t) * elements_right);
	}
	this->data[where] = new_element;
	this->count += 1;
}

template <typename t, int n0>
t& s_array<t, n0>::operator[](int index)
{
	assert(index >= 0);
	assert(index < n0);
	t& result = this->data[index];
	return result;
}

template <typename t, int n0, int n1>
s_array<t, n1>& s_array2d<t, n0, n1>::operator[](int index)
{
	assert(index >= 0);
	assert(index < n0);
	s_array<t, n1>& result = this->data[index];
	return result;
}

func b8 str_equals(s_len_str a, s_len_str b)
{
	b8 result = a.count == b.count && memcmp(a.str, b.str, a.count) == 0;
	return result;
}

func s_len_str str_advance(s_len_str str, int count)
{
	s_len_str result = str;
	str.str += count;
	str.count -= count;
	assert(str.count >= 0);
	return str;
}

func void str_advance(s_len_str* str, int count)
{
	*str = str_advance(*str, count);
}

func s_token next_token(s_tokenizer* tokenizer)
{
	s_token token = zero;
	while(true) {
		if(tokenizer->at[0] == '\0') {
			token.line = tokenizer->line;
			token.type = e_token_eof;
			break;
		}
		if(tokenizer->at[0] == ' ' || tokenizer->at[0] == '\t' || tokenizer->at[0] == '\r') {
			str_advance(&tokenizer->at, 1);
			continue;
		}
		if(tokenizer->at[0] == '\n') {
			str_advance(&tokenizer->at, 1);
			tokenizer->line += 1;
			continue;
		}
		if(tokenizer->at[0] == '/' && tokenizer->at[1] == '/') {
			while(tokenizer->at[0] != '\n') {
				str_advance(&tokenizer->at, 1);
			}
			tokenizer->line += 1;
			continue;
		}
		break;
	}

	if(token.type == e_token_invalid && can_start_identifier(tokenizer->at[0])) {
		token.str = tokenizer->at;
		token.str.count = 0;
		token.type = e_token_identifier;
		token.line = tokenizer->line;
		while(can_continue_identifier(tokenizer->at[0])) {
			token.str.count += 1;
			str_advance(&tokenizer->at, 1);
		}
	}

	if(token.type == e_token_invalid && is_number(tokenizer->at[0])) {
		token.type = e_token_int;
		token.str = tokenizer->at;
		token.str.count = 0;
		token.line = tokenizer->line;

		while(is_number(tokenizer->at[0])) {
			token.str.count += 1;
			str_advance(&tokenizer->at, 1);
		}
	}

	constexpr char one_length_operator_arr[] = {
		'(', ')', ';', '{', '}',
	};
	for(int i = 0; i < array_count(one_length_operator_arr); i += 1) {
		if(token.type == e_token_invalid && tokenizer->at[0] == one_length_operator_arr[i]) {
			token.str = tokenizer->at;
			token.str.count = 1;
			token.type = e_token_operator;
			token.line = tokenizer->line;
			str_advance(&tokenizer->at, 1);
		}
	}

	if(token.type == e_token_invalid) {
		assert(!"Bad token");
	}
	return token;
}

func s_token peek_token(s_tokenizer tokenizer)
{
	s_token token = next_token(&tokenizer);
	return token;
}

func b8 consume_token(s_tokenizer* tokenizer, s_len_str str)
{
	b8 result = false;
	s_tokenizer tokenizer_copy = *tokenizer;
	s_token token = next_token(&tokenizer_copy);
	if(str_equals(token.str, str)) {
		*tokenizer = tokenizer_copy;
		result = true;
	}
	return result;
}

func b8 consume_token(s_tokenizer* tokenizer, e_token type, s_token* out_token)
{
	b8 result = false;
	s_tokenizer tokenizer_copy = *tokenizer;
	s_token token = next_token(&tokenizer_copy);
	if(token.type == type) {
		*tokenizer = tokenizer_copy;
		*out_token = token;
		result = true;
	}
	return result;
}

func b8 consume_identifier(s_tokenizer* tokenizer, s_token* out_token)
{
	b8 result = consume_token(tokenizer, e_token_identifier, out_token);
	return result;
}

func s_v4 make_rrr(float r)
{
	s_v4 result;
	result.r = r;
	result.g = r;
	result.b = r;
	result.a = 1;
	return result;
}

func s_v4 make_rgb(float r, float g, float b)
{
	s_v4 result;
	result.r = r;
	result.g = g;
	result.b = b;
	result.a = 1;
	return result;
}

func s_v4 make_rgba(float r, float g, float b, float a)
{
	s_v4 result;
	result.r = r;
	result.g = g;
	result.b = b;
	result.a = a;
	return result;
}

func s_v4 make_ra(float r, float a)
{
	s_v4 result;
	result.r = r;
	result.g = r;
	result.b = r;
	result.a = a;
	return result;
}

[[nodiscard]]
func float range_lerp(float curr, float curr_start, float curr_end, float target_start, float target_end)
{
	float p = (curr - curr_start) / (curr_end - curr_start);
	float result = target_start + (target_end - target_start) * p;
	return result;
}

[[nodiscard]]
func int circular_index(int index, int size)
{
	assert(size > 0);
	if(index >= 0) {
		return index % size;
	}
	return (size - 1) - ((-index - 1) % size);
}

template <typename t>
func t at_least(t a, t b)
{
	return a > b ? a : b;
}

template <typename t>
func t at_most(t a, t b)
{
	return b > a ? a : b;
}

template <typename t>
func t clamp(t current, t min_val, t max_val)
{
	return at_most(max_val, at_least(min_val, current));
}

[[nodiscard]]
func float smoothstep(float edge0, float edge1, float x)
{
	// Scale, bias and saturate x to 0..1 range
	x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	// Evaluate polynomial
	return x * x * (3 - 2 * x);
}

template <typename t>
func t min(t a, t b)
{
	return a <= b ? a : b;
}

template <typename t>
func t max(t a, t b)
{
	t result = a > b ? a : b;
	return result;
}

func void operator*=(s_v3& a, float b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;
}

func s_v3 v3_set_mag(s_v3 v, float len)
{
	v = v3_normalized(v);
	v *= len;
	return v;
}

func int sign_as_int(float x)
{
	int result = x >= 0 ? 1 : -1;
	return result;
}

func float sign_as_float(float x)
{
	float result = x >= 0 ? 1.0f : -1.0f;
	return result;
}

func float go_towards(float from, float to, float amount)
{
	assert(amount >= 0);

	float result = from;
	float dist = to - from;
	int s = sign_as_int(dist);
	result += min(fabsf(dist), amount) * s;
	return result;
}

func void operator+=(s_v3& a, s_v3 b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
}

func s_v3 go_towards_v3(s_v3 from, s_v3 to, float amount)
{
	assert(amount >= 0);

	s_v3 result = from;
	s_v3 dir = to - from;
	s_v3 dir_n = v3_normalized(dir);
	result.x = go_towards(from.x, to.x, amount * fabsf(dir_n.x));
	result.y = go_towards(from.y, to.y, amount * fabsf(dir_n.y));
	result.z = go_towards(from.z, to.z, amount * fabsf(dir_n.z));
	return result;
}

func float v3_distance(s_v3 a, s_v3 b)
{
	s_v3 c = a - b;
	float result = v3_length(c);
	return result;
}

func b8 sphere_vs_sphere(s_v3 pos1, float r1, s_v3 pos2, float r2)
{
	float dist = v3_distance(pos1, pos2);
	b8 result = dist < (r1 + r2);
	return result;
}

func s_v4 hex_to_rgb(u32 hex)
{
	s_v4 result;
	result.x = ((hex & 0xFF0000) >> 16) / 255.0f;
	result.y = ((hex & 0x00FF00) >> 8) / 255.0f;
	result.z = ((hex & 0x0000FF) >> 0) / 255.0f;
	result.w = 1;
	return result;
}

// @Note(tkap, 19/04/2025): This might be fucked
func s_m4 make_orthographic(float left, float right, float bottom, float top, float near, float far)
{
	s_m4 result = zero;

	// xy values are mapped to -1 to 1 range for viewport mapping
	// z values are mapped to 0 to 1 range instead of -1 to 1 for depth writing
	result.all2[0][0] = 2.0f / (right - left);
	result.all2[1][1] = 2.0f / (top - bottom);
	result.all2[2][2] = 1.0f / (near - far);
	result.all2[3][3] = 1.0f;

	result.all2[3][0] = (left + right) / (left - right);
	result.all2[3][1] = (bottom + top) / (bottom - top);
	result.all2[3][2] = 0.5f * (near + far) / (near - far);

	return result;
}

func float lerp(float a, float b, float t)
{
	float result = a + (b - a) * t;
	return result;
}

func s_v2 lerp_v2(s_v2 a, s_v2 b, float t)
{
	s_v2 result = zero;
	result.x = lerp(a.x, b.x, t);
	result.y = lerp(a.y, b.y, t);
	return result;
}

func s_v3 lerp_v3(s_v3 a, s_v3 b, float t)
{
	s_v3 result = zero;
	result.x = lerp(a.x, b.x, t);
	result.y = lerp(a.y, b.y, t);
	result.z = lerp(a.z, b.z, t);
	return result;
}

func s_m4 m4_inverse(const s_m4 m)
{
	s_m4 result = zero;
	float det;
	int i;

	result.all[0] = m.all[5]  * m.all[10] * m.all[15] -
		m.all[5]  * m.all[11] * m.all[14] -
		m.all[9]  * m.all[6]  * m.all[15] +
		m.all[9]  * m.all[7]  * m.all[14] +
		m.all[13] * m.all[6]  * m.all[11] -
		m.all[13] * m.all[7]  * m.all[10];

	result.all[4] = -m.all[4]  * m.all[10] * m.all[15] +
		m.all[4]  * m.all[11] * m.all[14] +
		m.all[8]  * m.all[6]  * m.all[15] -
		m.all[8]  * m.all[7]  * m.all[14] -
		m.all[12] * m.all[6]  * m.all[11] +
		m.all[12] * m.all[7]  * m.all[10];

	result.all[8] = m.all[4]  * m.all[9] * m.all[15] -
		m.all[4]  * m.all[11] * m.all[13] -
		m.all[8]  * m.all[5] * m.all[15] +
		m.all[8]  * m.all[7] * m.all[13] +
		m.all[12] * m.all[5] * m.all[11] -
		m.all[12] * m.all[7] * m.all[9];

	result.all[12] = -m.all[4]  * m.all[9] * m.all[14] +
			m.all[4]  * m.all[10] * m.all[13] +
			m.all[8]  * m.all[5] * m.all[14] -
			m.all[8]  * m.all[6] * m.all[13] -
			m.all[12] * m.all[5] * m.all[10] +
			m.all[12] * m.all[6] * m.all[9];

	result.all[1] = -m.all[1]  * m.all[10] * m.all[15] +
		m.all[1]  * m.all[11] * m.all[14] +
		m.all[9]  * m.all[2] * m.all[15] -
		m.all[9]  * m.all[3] * m.all[14] -
		m.all[13] * m.all[2] * m.all[11] +
		m.all[13] * m.all[3] * m.all[10];

	result.all[5] = m.all[0]  * m.all[10] * m.all[15] -
		m.all[0]  * m.all[11] * m.all[14] -
		m.all[8]  * m.all[2] * m.all[15] +
		m.all[8]  * m.all[3] * m.all[14] +
		m.all[12] * m.all[2] * m.all[11] -
		m.all[12] * m.all[3] * m.all[10];

	result.all[9] = -m.all[0]  * m.all[9] * m.all[15] +
		m.all[0]  * m.all[11] * m.all[13] +
		m.all[8]  * m.all[1] * m.all[15] -
		m.all[8]  * m.all[3] * m.all[13] -
		m.all[12] * m.all[1] * m.all[11] +
		m.all[12] * m.all[3] * m.all[9];

	result.all[13] = m.all[0]  * m.all[9] * m.all[14] -
		m.all[0]  * m.all[10] * m.all[13] -
		m.all[8]  * m.all[1] * m.all[14] +
		m.all[8]  * m.all[2] * m.all[13] +
		m.all[12] * m.all[1] * m.all[10] -
		m.all[12] * m.all[2] * m.all[9];

	result.all[2] = m.all[1]  * m.all[6] * m.all[15] -
		m.all[1]  * m.all[7] * m.all[14] -
		m.all[5]  * m.all[2] * m.all[15] +
		m.all[5]  * m.all[3] * m.all[14] +
		m.all[13] * m.all[2] * m.all[7] -
		m.all[13] * m.all[3] * m.all[6];

	result.all[6] = -m.all[0]  * m.all[6] * m.all[15] +
		m.all[0]  * m.all[7] * m.all[14] +
		m.all[4]  * m.all[2] * m.all[15] -
		m.all[4]  * m.all[3] * m.all[14] -
		m.all[12] * m.all[2] * m.all[7] +
		m.all[12] * m.all[3] * m.all[6];

	result.all[10] = m.all[0]  * m.all[5] * m.all[15] -
		m.all[0]  * m.all[7] * m.all[13] -
		m.all[4]  * m.all[1] * m.all[15] +
		m.all[4]  * m.all[3] * m.all[13] +
		m.all[12] * m.all[1] * m.all[7] -
		m.all[12] * m.all[3] * m.all[5];

	result.all[14] = -m.all[0]  * m.all[5] * m.all[14] +
		m.all[0]  * m.all[6] * m.all[13] +
		m.all[4]  * m.all[1] * m.all[14] -
		m.all[4]  * m.all[2] * m.all[13] -
		m.all[12] * m.all[1] * m.all[6] +
		m.all[12] * m.all[2] * m.all[5];

	result.all[3] = -m.all[1] * m.all[6] * m.all[11] +
		m.all[1] * m.all[7] * m.all[10] +
		m.all[5] * m.all[2] * m.all[11] -
		m.all[5] * m.all[3] * m.all[10] -
		m.all[9] * m.all[2] * m.all[7] +
		m.all[9] * m.all[3] * m.all[6];

	result.all[7] = m.all[0] * m.all[6] * m.all[11] -
		m.all[0] * m.all[7] * m.all[10] -
		m.all[4] * m.all[2] * m.all[11] +
		m.all[4] * m.all[3] * m.all[10] +
		m.all[8] * m.all[2] * m.all[7] -
		m.all[8] * m.all[3] * m.all[6];

	result.all[11] = -m.all[0] * m.all[5] * m.all[11] +
		m.all[0] * m.all[7] * m.all[9] +
		m.all[4] * m.all[1] * m.all[11] -
		m.all[4] * m.all[3] * m.all[9] -
		m.all[8] * m.all[1] * m.all[7] +
		m.all[8] * m.all[3] * m.all[5];

	result.all[15] = m.all[0] * m.all[5] * m.all[10] -
		m.all[0] * m.all[6] * m.all[9] -
		m.all[4] * m.all[1] * m.all[10] +
		m.all[4] * m.all[2] * m.all[9] +
		m.all[8] * m.all[1] * m.all[6] -
		m.all[8] * m.all[2] * m.all[5];

	det = m.all[0] * result.all[0] + m.all[1] * result.all[4] + m.all[2] * result.all[8] + m.all[3] * result.all[12];

	if(det == 0)
		return result;

	det = 1.0f / det;

	for (i = 0; i < 16; i += 1) {
		result.all[i] = result.all[i] * det;
	}

	return result;
}

func s_v4 v4_multiply_m4(s_v4 v, s_m4 m)
{
	s_v4 result;

	result.x = m.all[0] * v.x + m.all[4] * v.y + m.all[8]  * v.z + m.all[12] * v.w;
	result.y = m.all[1] * v.x + m.all[5] * v.y + m.all[9]  * v.z + m.all[13] * v.w;
	result.z = m.all[2] * v.x + m.all[6] * v.y + m.all[10] * v.z + m.all[14] * v.w;
	result.w = m.all[3] * v.x + m.all[7] * v.y + m.all[11] * v.z + m.all[15] * v.w;

	return result;
}

func s_v2 v2_multiply_m4(s_v2 v, s_m4 m)
{
	s_v2 result = v4_multiply_m4(v4(v.x, v.y, 0.0f, 1.0f), m).xy;
	return result;
}

func s_ray get_camera_ray(s_v3 cam_pos, s_m4 view, s_m4 projection, s_v2 mouse, s_v2 world_size)
{

	float x = (2.0f * mouse.x) / world_size.x - 1.0f;

	// @Note(tkap, 19/06/2022): y may not need to be reversed
	float y = 1.0f - (2.0f * mouse.y) / world_size.y;

	s_v3 ray_nds = v3(x, y, -1);

	s_v4 ray_clip = v4(ray_nds.x, ray_nds.y, ray_nds.z, 1);

	s_v4 ray_eye = v4_multiply_m4(ray_clip, m4_inverse(projection));
	ray_eye = v4(ray_eye.x, ray_eye.y, -1.0, 0.0);

	s_v3 ray_wor = v4_multiply_m4(ray_eye, m4_inverse(view)).xyz;
	// don't forget to normalize the vector at some point
	ray_wor = v3_normalized(ray_wor);

	s_ray result = zero;
	result.pos = cam_pos;
	result.dir = ray_wor;
	return result;
}

func s_v3 ray_at_y(s_ray ray, float y)
{
	float t = (y - ray.pos.y) / ray.dir.y;
	return ray.pos + ray.dir * t;
}

func float sinf2(float x)
{
	float result = sinf(x) * 0.5f + 0.5f;
	return result;
}

func float sin_range(float min_val, float max_val, float x)
{
	float result = lerp(min_val, max_val, sinf(x) * 0.5f + 0.5f);
	return result;
}

func int ceilfi(float x)
{
	int result = (int)ceilf(x);
	return result;
}

func int floorfi(float x)
{
	int result = (int)floorf(x);
	return result;
}

func int roundfi(float x)
{
	int result = (int)roundf(x);
	return result;
}

[[nodiscard]]
func float ilerp(float start, float end, float val)
{
	float b = end - start;
	if(b == 0) { return val; }
	return (val - start) / b;
}

[[nodiscard]]
func float ilerp_clamp(float start, float end, float value)
{
	float result = ilerp(start, end, clamp(value, start, end));
	return result;
}


func float handle_advanced_easing(float x, float x_start, float x_end)
{
	x = clamp(ilerp_clamp(x_start, x_end, x), 0.0f, 1.0f);
	return x;
}

func float ease_in_expo(float x)
{
	if(x == 0) { return 0; }
	return powf(2, 10 * x - 10);
}

func float ease_linear(float x)
{
	return x;
}

func float ease_in_quad(float x)
{
	return x * x;
}

func float ease_out_quad(float x)
{
	float x2 = 1 - x;
	return 1 - x2 * x2;
}

func float ease_out_expo(float x)
{
	if(x == 1) { return 1; }
	return 1 - powf(2, -10 * x);
}

func float ease_out_elastic(float x)
{
	constexpr float c4 = (2 * c_pi) / 3;
	if(x == 0 || x == 1) { return x; }
	return powf(2, -5 * x) * sinf((x * 5 - 0.75f) * c4) + 1;
}

func float ease_out_elastic2(float x)
{
	constexpr float c4 = (2 * c_pi) / 3;
	if(x == 0 || x == 1) { return x; }
	return powf(2, -10 * x) * sinf((x * 10 - 0.75f) * c4) + 1;
}

func float ease_out_back(float x)
{
	float c1 = 1.70158f;
	float c3 = c1 + 1;
	return 1 + c3 * powf(x - 1, 3) + c1 * powf(x - 1, 2);
}

#define X(fname, ename) \
func float fname##_advanced(float x, float x_start, float x_end, float target_start, float target_end) \
{ \
	x = handle_advanced_easing(x, x_start, x_end); \
	return lerp(target_start, target_end, fname(x)); \
}
m_advanced_easings
#undef X

func s_v4 multiply_rgb_clamped(s_v4 color, float val)
{
	color.r = clamp(color.r * val, 0.0f, 1.0f);
	color.g = clamp(color.g * val, 0.0f, 1.0f);
	color.b = clamp(color.b * val, 0.0f, 1.0f);
	return color;
}

func s_v4 multiply_rgb(s_v4 color, float val)
{
	color.r *= val;
	color.g *= val;
	color.b *= val;
	return color;
}

func s_v4 lerp_color(s_v4 a, s_v4 b, float t)
{
	s_v4 result;
	result.x = lerp(a.x, b.x, t);
	result.y = lerp(a.y, b.y, t);
	result.z = lerp(a.z, b.z, t);
	result.w = lerp(a.w, b.w, t);
	return result;
}

func float v2_length_squared(s_v2 a)
{
	float result = a.x * a.x + a.y * a.y;
	return result;
}

func float v2_distance(s_v2 a, s_v2 b)
{
	return v2_length(a - b);
}

func float v2_distance_squared(s_v2 a, s_v2 b)
{
	return v2_length_squared(a - b);
}

func float lerp_snap(float a, float b, float t, float max_diff)
{
	float result = a + (b - a) * t;
	if(fabsf(result - b) < max_diff) {
		result = b;
	}
	return result;
}

func s_v2 lerp_snap(s_v2 a, s_v2 b, float t)
{
	s_v2 result;
	float dist = v2_distance(a, b);
	if(dist < 1.0f) {
		t = 1;
	}
	result.x = lerp(a.x, b.x, t);
	result.y = lerp(a.y, b.y, t);
	return result;
}

func void operator-=(s_v2& a, s_v2 b)
{
	a.x -= b.x;
	a.y -= b.y;
}

func b8 operator==(s_v2i a, s_v2i b)
{
	b8 result = a.x == b.x && a.y == b.y;
	return result;
}

[[nodiscard]] func u64 randu64(s_rng* rng)
{
	u64 result = (u64)(randf32(rng) * (double)c_max_u64);
	return result;
}

// min inclusive, max inclusive
[[nodiscard]] func int rand_range_ii(s_rng* rng, int min_val, int max_val)
{
	if(min_val > max_val) {
		int temp = min_val;
		min_val = max_val;
		max_val = temp;
	}

	int result = (int)(min_val + (randu(rng) % (max_val - min_val + 1)));
	return result;
}

// min inclusive, max exclusive
[[nodiscard]] func int rand_range_ie(s_rng* rng, int min_val, int max_val)
{
	if(min_val > max_val) {
		int temp = min_val;
		min_val = max_val;
		max_val = temp;
	}
	int result = (int)(min_val + (randu(rng) % (max_val - min_val)));
	return result;
}

[[nodiscard]] func b8 chance100(s_rng* rng, float chance)
{
	assert(chance >= 0);
	b8 result = chance / 100 >= randf32(rng);
	return result;
}

[[nodiscard]] func b8 chance1(s_rng* rng, f32 chance)
{
	assert(chance >= 0);
	return chance >= randf32(rng);
}

[[nodiscard]] func b8 rand_bool(s_rng* rng)
{
	return (b8)(randu(rng) & 1);
}

[[nodiscard]] func int while_chance1(s_rng* rng, float chance)
{
	int result = 0;
	while(chance1(rng, chance)) {
		result += 1;
	}
	return result;
}

func s_v2 v2_from_angle(float angle)
{
	s_v2 result = zero;
	result.x = cosf(angle);
	result.y = sinf(angle);
	return result;
}

func void operator*=(s_v2& a, float b)
{
	a.x *= b;
	a.y *= b;
}

func s_v2 v2_normalized(s_v2 v)
{
	float length = v2_length(v);
	s_v2 result = v;
	if(length > 0) {
		result.x /= length;
		result.y /= length;
	}
	return result;
}

func s_v2 v2_dir_from_to(s_v2 from, s_v2 to)
{
	s_v2 result = v2_normalized(to - from);
	return result;
}

func s_v3 v3_reflect(s_v3 a, s_v3 b)
{
	float dot = v3_dot(a, b) * 2;
	s_v3 result;
	result.x = a.x - b.x * dot;
	result.y = a.y - b.y * dot;
	result.z = a.z - b.z * dot;
	return result;
}

func float v2_dot(s_v2 a, s_v2 b)
{
	float result = (a.x * b.x) + (a.y * b.y);
	return result;
}

func s_v2 v2_reflect(s_v2 a, s_v2 b)
{
	float dot = v2_dot(a, b) * 2;
	s_v2 result;
	result.x = a.x - b.x * dot;
	result.y = a.y - b.y * dot;
	return result;
}

func s_v2 v2_set_mag(s_v2 v, float mag)
{
	s_v2 result = v2_normalized(v) * mag;
	return result;
}

func int get_spaces_for_column(int column)
{
	constexpr int tab_size = 4;
	static_assert(tab_size > 0);
	int result = tab_size - (column % tab_size);
	return result;
}

func s_v2 random_point_in_rect_edges(s_rng* rng, s_v2 size)
{
	struct s_edge {
		s_v2 start;
		s_v2 end;
	};
	s_v2 hs = size * 0.5f;
	s_edge edge_arr[4] = {
		{v2(-hs.x, -hs.y), v2(-hs.x, hs.y)}, // left
		{v2(-hs.x, -hs.y), v2(hs.x, -hs.y)}, // top
		{v2(hs.x, -hs.y), v2(hs.x, hs.y)}, // right
		{v2(-hs.x, hs.y), v2(hs.x, hs.y)}, // bottom
	};
	int index = randu(rng) % 4;
	s_v2 result = v2(
		randf_range(rng, edge_arr[index].start.x, edge_arr[index].end.x),
		randf_range(rng, edge_arr[index].start.y, edge_arr[index].end.y)
	);
	return result;
}

func void add_state_transition(s_state* state, int new_state, float time_now, float transition_duration)
{
	assert(transition_duration >= 0);
	if(state->next_state.valid) { return; }
	state->transition_duration = transition_duration;
	state->transition_timestamp = time_now;
	s_temp_state temp = zero;
	temp.id = new_state;
	state->next_state = maybe(temp);
}

func void add_state(s_state* state, int new_state)
{
	add_state_transition(state, new_state, 0, 0);
}

func void add_temporary_state_transition(s_state* state, int new_state, float time_now, float transition_duration)
{
	assert(transition_duration >= 0);
	if(state->next_state.valid) { return; }
	state->transition_duration = transition_duration;
	state->transition_timestamp = time_now;
	s_temp_state temp = zero;
	temp.id = new_state;
	temp.is_temporary = true;
	state->next_state = maybe(temp);
}

func void add_temporary_state(s_state* state, int new_state)
{
	add_temporary_state_transition(state, new_state, 0, 0);
}

func void pop_state_transition(s_state* state, float time_now, float transition_duration)
{
	assert(transition_duration >= 0);
	if(state->do_pop) { return; }
	state->transition_duration = transition_duration;
	state->transition_timestamp = time_now;
	state->do_pop = true;
}

func void pop_state(s_state* state)
{
	pop_state_transition(state, 0, 0);
}

func void clear_state(s_state* state)
{
	state->clear_state = true;
}

func int get_state(s_state* state)
{
	int result = state->stack.get_last().id;
	return result;
}

func s_maybe<int> get_previous_non_temporary_state(s_state* state)
{
	s_maybe<int> result = zero;
	int index = state->stack.count - 2;
	while(index >= 0) {
		s_temp_state temp = state->stack[index];
		if(!temp.is_temporary) {
			result = maybe(temp.id);
			break;
		}
		index -= 1;
	}
	return result;
}

func s_state_transition get_state_transition(s_state* state, float time_now)
{
	s_state_transition result = zero;
	result.time_data = get_time_data(time_now, state->transition_timestamp, state->transition_duration);
	if(state->transition_duration > 0 && result.time_data.percent > 0 && result.time_data.percent < 1) {
		result.active = true;
	}
	return result;
}

func b8 handle_state(s_state* state, float time_now)
{
	s_state_transition transition = get_state_transition(state, time_now);
	b8 result = false;
	b8 in_first_half_of_transition = transition.active && transition.time_data.percent < 0.5f;
	if(state->clear_state) {
		assert(state->next_state.valid);
		if(!in_first_half_of_transition) {
			state->clear_state = false;
			state->stack.count = 0;
			result = true;
		}
	}
	if(state->do_pop) {
		assert(!state->next_state.valid);
		if(!in_first_half_of_transition) {
			state->do_pop = false;
			while(true) {
				state->stack.pop_last();
				s_temp_state temp = state->stack.get_last();
				if(!temp.is_temporary) {
					break;
				}
			}
			result = true;
		}
	}
	if(state->next_state.valid) {
		if(!in_first_half_of_transition) {
			state->stack.add(state->next_state.value);
			state->next_state = zero;
			result = true;
		}
	}
	return result;
}

func void set_state(s_state* state, int new_state)
{
	add_state(state, new_state);
	handle_state(state, 0);
}

func float v2_angle(s_v2 v)
{
	float result = atan2f(v.y, v.x);
	return result;
}

func s_v2 v2_rotated(s_v2 v, float in_angle)
{
	float length = v2_length(v);
	float angle = in_angle + v2_angle(v);
	s_v2 result = v2(
		cosf(angle) * length,
		sinf(angle) * length
	);
	return result;
}


func s_v2 orbit_around_2d(s_v2 center, float distance, float angle)
{
	s_v2 result = v2(cosf(angle), sinf(angle)) * distance;
	result += center;
	return result;
}

func float interp_based_on_type(float dt, e_ease type)
{
	float result = 0;
	#define X(name, b) case e_##name: { result = name(dt); } break;

	switch(type) {
		m_advanced_easings
		invalid_default_case;
	}

	#undef X

	return result;
}

func void animator_after_animate(s_animator* animator, float duration)
{
	animator->step_end_time_arr[animator->step_count] = max(
		animator->step_end_time_arr[animator->step_count],
		animator->step_start_time_arr[animator->step_count] + duration
	);

	#if defined(m_debug)
	animator->can_add_keyframe = true;
	#endif // m_debug
}

func void animate_v2(s_animator* animator, s_v2 start, s_v2 end, float duration, s_v2* ptr, e_ease interp_type, float time)
{
	float dt = ilerp(animator->step_start_time_arr[animator->step_count], animator->step_start_time_arr[animator->step_count] + duration, time);
	if(dt >= 0) {
		dt = at_most(1.0f, dt);
		float dt2 = interp_based_on_type(dt, interp_type);
		*ptr = lerp_v2(start, end, dt2);
	}
	animator_after_animate(animator, duration);
}

func void animate_color(s_animator* animator, s_v4 start, s_v4 end, float duration, s_v4* ptr, e_ease interp_type, float time)
{
	float dt = ilerp(animator->step_start_time_arr[animator->step_count], animator->step_start_time_arr[animator->step_count] + duration, time);
	if(dt >= 0) {
		dt = at_most(1.0f, dt);
		float dt2 = interp_based_on_type(dt, interp_type);
		*ptr = lerp_color(start, end, dt2);
	}
	animator_after_animate(animator, duration);
}

func void animate_float(s_animator* animator, float start, float end, float duration, float* ptr, e_ease interp_type, float time)
{
	float dt = ilerp(animator->step_start_time_arr[animator->step_count], animator->step_start_time_arr[animator->step_count] + duration, time);
	if(dt >= 0) {
		dt = at_most(1.0f, dt);
		float dt2 = interp_based_on_type(dt, interp_type);
		*ptr = lerp(start, end, dt2);
	}
	animator_after_animate(animator, duration);
}

func void animate_gravity(s_animator* animator, float force, float gravity, float duration, float* ptr, float time)
{
	float dt = ilerp(animator->step_start_time_arr[animator->step_count], animator->step_start_time_arr[animator->step_count] + duration, time);
	if(dt >= 0) {
		dt = at_most(1.0f, dt);
		*ptr = -force * dt + 0.5f * gravity * (dt * dt);
	}
	animator_after_animate(animator, duration);
}

func void animator_end_keyframe(s_animator* animator, float delay)
{
	#if defined(m_debug)
	assert(animator->can_add_keyframe);
	animator->can_add_keyframe = false;
	#endif // m_debug

	animator->step_count += 1;
	animator->step_start_time_arr[animator->step_count] = animator->step_end_time_arr[animator->step_count - 1] + delay;
}

template <typename t>
func void buffer_write(u8** cursor, t data)
{
	u8* t0 = *cursor;
	t* t1 = (t*)t0;
	*t1 = data;
	*cursor += sizeof(t);
}

func void buffer_memcpy(u8** cursor, void* data, int data_size)
{
	u8* t0 = *cursor;
	memcpy(t0, data, data_size);
	*cursor += data_size;
}


template <typename t>
func t buffer_read(u8** cursor)
{
	u8* t0 = *cursor;
	t* t1 = (t*)t0;
	*cursor += sizeof(t);
	return *t1;
}

func s_v4 hsv_to_rgb(float hue, float saturation, float value)
{
	s_v4 color;

	// Red channel
	float k = fmodf((5.0f + hue/60.0f), 6);
	float t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.r = (value - value*saturation*k);

	// Green channel
	k = fmodf((3.0f + hue/60.0f), 6);
	t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.g = (value - value*saturation*k);

	// Blue channel
	k = fmodf((1.0f + hue/60.0f), 6);
	t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.b = (value - value*saturation*k);

	color.a = 1;

	return color;
}

func float fract(float x)
{
	float result = x - (int)x;
	return result;
}

func constexpr b8 float_equals(float a, float b)
{
	return (a >= b - epsilon && a <= b + epsilon);
}


template <int n>
func int pick_rand_from_weight_arr(s_list<u32, n>* weight_arr, s_rng* rng)
{
	u64 total_weight = 0;
	for(int i = 0; i < weight_arr->count; i += 1) {
		total_weight += weight_arr->data[i];
	}

	int result = -1;
	for(int i = 0; i < weight_arr->count; i += 1) {
		u64 r = randu(rng) % total_weight;
		u32 weight = weight_arr->data[i];
		if(r < weight) {
			result = i;
			break;
		}
		else {
			total_weight -= weight;
		}
	}
	assert(result >= 0);
	return result;
}

func s_v2 rand_v2_11(s_rng* rng)
{
	s_v2 result = zero;
	result.x = randf32_11(rng);
	result.y = randf32_11(rng);
	return result;
}

func char to_upper_case(char c)
{
	char result = c;
	if(c >= 'a' && c <= 'z') {
		result -= 'a' - 'A';
	}
	return result;
}

func u32 float_to_radix(float val)
{
	u32 result = *(u32*)&val;
	if(result == 0x80000000) {
		result = ~result;
	}
	else {
		result |= 0x80000000;
	}
	return result;
}

template <typename t>
func void swap(t* a, t* b)
{
	t temp = *a;
	*a = *b;
	*b = temp;
}

// @Note(tkap, 19/05/2022): https://godotengine.org/qa/5770/how-to-lerp-between-two-angles
func float short_angle_dist(float from, float to)
{
	float max_angle = c_pi * 2;
	float difference = fmodf(to - from, max_angle);
	return fmodf(2 * difference, max_angle) - difference;
}

// @Note(tkap, 19/05/2022): https://godotengine.org/qa/5770/how-to-lerp-between-two-angles
func float lerp_angle(float from, float to, float weight)
{
	return from + short_angle_dist(from, to) * weight;
}

func void reset_linear_arena(s_linear_arena* arena)
{
	arena->used = 0;
	poison_memory(arena->memory, arena->capacity);
}

func s_v4 set_alpha(s_v4 color, float alpha)
{
	s_v4 result = color;
	result.a = alpha;
	return result;
}

template <typename t, int n>
void s_list<t, n>::remove_and_shift(int index)
{
	assert(index >= 0);
	assert(index < count);
	this->count -= 1;

	int to_move = this->count - index;
	if(to_move > 0) {
		memmove(this->data + index, this->data + index + 1, to_move * sizeof(t));
	}
}

template <typename t>
func void toggle_maybe(s_maybe<t>* m, t new_value)
{
	if(m->valid) {
		m->valid = false;
	}
	else {
		*m = maybe(new_value);
	}
}

template <typename t>
func b8 maybe_equals(s_maybe<t> m, int v)
{
	b8 result = m.valid && m.value == v;
	return result;
}

func s_rect make_rect(s_v2 pos, s_v2 size)
{
	s_rect result;
	result.pos = pos;
	result.size = size;
	return result;
}

func s_rect expand_rect_sides_from_center(s_rect rect, float add)
{
	rect.pos.x -= add;
	rect.pos.y -= add;
	rect.size.x += add * 2;
	rect.size.y += add * 2;
	return rect;
}

func b8 is_2d_index_out_of_bounds(s_v2i index, s_v2i bounds)
{
	b8 result = index.x >= 0 && index.x < bounds.x && index.y >= 0 && index.y < bounds.y;
	return !result;
}

#if defined(_WINDOWS_)
func wchar_t* ascii_to_wide(s_len_str str, s_circular_arena* arena)
{
	wchar_t* result = (wchar_t*)circular_arena_alloc_zero(arena, str.count * 2 + 2);
	for(int i = 0; i < str.count; i += 1) {
		result[i] = str[i];
	}
	return result;
}
#endif

#if defined(m_winhttp)
func int http_get_status_code(HINTERNET request)
{
	char buffer[32] = zero;
	{
		DWORD size = sizeof(buffer);
		DWORD index = 0;
		BOOL result = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, buffer, &size, &index);
		if(result != TRUE) {
			DWORD e = GetLastError();
			printf("%lu\n", e);
			assert(false);
		}
	}
	char temp[32] = zero;
	int buffer_cursor = 0;
	int temp_cursor = 0;
	// @Note(tkap, 08/08/2025): The data in 'buffer' seems to be in wide format
	while(buffer[buffer_cursor] != '\0') {
		temp[temp_cursor] = buffer[buffer_cursor];
		temp_cursor += 1;
		buffer_cursor += 2;
	}
	int result = atoi(temp);
	return result;
}

func b8 is_http_request_ok(int status_code)
{
	b8 result = status_code >= 200 && status_code <= 299;
	return result;
}

func void read_http_data(HINTERNET request, s_memory* memory, s_circular_arena* arena)
{
	memory->ptr = circular_arena_alloc_zero(arena, 1 * c_mb);
	while(true) {
		DWORD size = 0;
		if(!WinHttpQueryDataAvailable(request, &size)) {
			assert(false);
			break;
		}
		if(size == 0) break;
		assert(size + memory->size <= 1 * c_mb);

		DWORD downloaded = 0;
		if(!WinHttpReadData(request, memory->ptr + memory->size, size, &downloaded)) {
			assert(false);
		}
		memory->size += size;
	}
}

func s_http_request do_post_request(HINTERNET session, s_len_str server, s_len_str resource, s_len_str body, s_circular_arena* arena)
{
	INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;

	HINTERNET connect = WinHttpConnect(session, ascii_to_wide(server, arena), port, 0);
	assert(connect);
	HINTERNET request = WinHttpOpenRequest(connect, L"POST", ascii_to_wide(resource, arena), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	assert(request);

	DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
		SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
	WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));

	LPCWSTR headers = L"Content-Type: application/json";
	BOOL success = WinHttpSendRequest(request, headers, (DWORD)-1L, body.str, body.count, body.count, 0);
	assert(success != 0);
	success = WinHttpReceiveResponse(request, NULL);
	assert(success != 0);

	s_http_request result = zero;
	result.status_code = http_get_status_code(request);
	read_http_data(request, &result.memory, arena);

	if(request) {
		WinHttpCloseHandle(request);
	}
	if(connect) {
		WinHttpCloseHandle(connect);
	}

	return result;
}

func s_http_request do_get_request(HINTERNET session, s_len_str server, s_len_str resource, s_len_str headers, s_circular_arena* arena)
{
	INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
	HINTERNET connect = WinHttpConnect(session, ascii_to_wide(server, arena), port, 0);
	assert(connect);

	HINTERNET request = WinHttpOpenRequest(connect, L"GET", ascii_to_wide(resource, arena), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	assert(request);

	{
		wchar_t* headers_wide = ascii_to_wide(headers, arena);
		if(WinHttpAddRequestHeaders(request, headers_wide, (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD) != TRUE) {
			assert(false);
		}
	}

	BOOL success = FALSE;
	success = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL);
	assert(success == TRUE);

	success = WinHttpReceiveResponse(request, NULL);
	assert(success = TRUE);

	s_http_request result = zero;
	result.status_code = http_get_status_code(request);
	read_http_data(request, &result.memory, arena);

	if(request) {
		WinHttpCloseHandle(request);
	}
	if(connect) {
		WinHttpCloseHandle(connect);
	}

	return result;
}

#endif

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		functions end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#pragma warning(pop)