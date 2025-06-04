#include "definitions.h"
#include "boqui_math.h"
// string.h IS INCLUDED IN THIS FILE AFTER MEMORY ARENAS
// maybe with GUARDS i could include it just here
// #include "math"
// #include "color"

// Byte/Memory operations
internal void // for some reason this is too slow compared to freeing and re allocating the memory
set_mem(void* mem, u32 size, u8 value)
{
	u8* scan = (u8*)mem;
	UNTIL(i, size)
	{
		*scan = value;
		scan++;
	}
}

internal void
copy_mem(void* from, void* to, u32 size)
{
	UNTIL(i, size)
	{
		((u8*)to)[i] = ((u8*)from)[i];
	}
}

internal bool
compare_mem(void* p1, void* p2, u32 size)
{
	UNTIL(i, size)
	{
		if(*(u8*)p1 != *(u8*)p2)
			return false;
	}
	return true;
}


// MEMORY ARENAS YEAH
struct Memory_arena{
	u8* data;
	u32 used;
	u32 size;
};
typedef Memory_arena Data_stream;

internal u8*
arena_push_size(Memory_arena* arena, u32 size){
	ASSERT(size+arena->used < arena->size);
	u8* result = arena->data+arena->used;
	arena->used += size;
	return result;
}

internal void
arena_pop_back_size(Memory_arena* arena, u32 size){
	arena->used -= size;
	set_mem(arena->data+arena->used, size, 0);
}

// TODO: THIS IS ILLEGAL and very slow
// internal void
// arena_pop_size(Memory_arena* arena, void* data, u32 size){
// 	u8* first_unused_byte = arena->data+arena->used;
// 	u8* skipped_data = ((u8*)data)+size;
// 	ASSERT(data>=arena->data && skipped_data<=first_unused_byte);

// 	copy_mem(skipped_data,data, first_unused_byte-skipped_data);
// 	arena_pop_back_size(arena, size);
// }

internal u8*
arena_push_data(Memory_arena* arena, void* data, u32 size){
	ASSERT((arena->used + size) <= arena->size);
	u8* result = arena_push_size(arena, size);
	copy_mem(data, result, size);
	return result;
}

#define ARENA_PUSH_STRUCT(arena, type) ((type*)arena_push_size(arena, sizeof(type)))
#define ARENA_PUSH_STRUCTS(arena, type, count) ((type*)arena_push_size(arena, count*sizeof(type)))


void data_stream_push(Data_stream* data_stream, char* data, u32 data_size)
{
	UNTIL(i, data_size)
	{
		data_stream->data[data_stream->used++] = data[i];
	}
	ASSERT(data_stream->used < data_stream->size);
}

// pushes a null terminated string to the data_stream
void data_stream_push_cstring(Data_stream* data_stream, char* string_text)
{
	while(*string_text)
	{
		data_stream->data[data_stream->used++] = *string_text;
		string_text++;
	}
	ASSERT(data_stream->used<data_stream->size);
}

void data_stream_pop(Data_stream* data_stream, u32 pop_size)
{
	ASSERT(pop_size < data_stream->used);
	data_stream->used -= pop_size;
	set_mem(data_stream->data + data_stream->used, pop_size, 0);
}

#include "string.h"

struct Buffer{
	union
	{
		void* data;
		u8* bytes;
		char* text;
	};
	u32 size;
};
typedef Buffer File_data;


// Image / Texture / Screen / RectPixels
struct Surface
{
	u32 width;
	u32 height;
	void* data;
};

typedef Surface Image;

//TODO: clean_image_borders



// FIXED SIZE ARRAYS


#define ARRAY(type, name) type* name

#define ARRAY_DECLARATION(type, name, length, arena) \
	*(ARENA_PUSH_STRUCT(arena, u32)) = length;\
	type* name = ARENA_PUSH_STRUCTS(arena, type, length);

#define ARRAYLEN(array) *(((u32*)array)-1)

#define ARRAY_CREATE(type, target, length, arena) *(ARENA_PUSH_STRUCT(arena, u32)) = length;\
	target = ARENA_PUSH_STRUCTS(arena, type, length);

#define DEFINE_ARRAY(type, array_name, arena, ...) type* array_name;\
	{\
		type TEMP_ARRAY [] = __VA_ARGS__;\
		*(ARENA_PUSH_STRUCT(arena, u32)) = ARRAYCOUNT(TEMP_ARRAY);\
		array_name = ARENA_PUSH_STRUCTS(arena, type, ARRAYCOUNT(TEMP_ARRAY));\
		copy_mem(TEMP_ARRAY, array_name, sizeof(TEMP_ARRAY));\
	}




#define CONCAT_INNER(a, b) a ## b
#define CONCAT(a, b) CONCAT_INNER(a, b)

// TYPESAFE LINKED LISTS WITH MACRO FUNCTIONS
// erasing an element would leave a memory leak 
// so if the use case is too dynamic use something else
#define LIST(type, var_name) type* var_name[3]
#define CLEAR_LIST(l) l[0] = 0; l[1] = 0; l[2] = 0;
#define LIST_LAST(l) l[1]
// i have no idea why i am double casting it to a u32 and a size_t but if i don't do that everything breaks
#define LIST_SIZE(l) ((u32)(size_t)(l[2]))
#define NEXT_ELEM(node) *((void**)(node+1))
#define SKIP_ELEM(node) *(&(void*)node) = NEXT_ELEM(node)
#define LIST_GET(l,index, out) out = l[0];ASSERT(index<LIST_SIZE(l));UNTIL(unique_index##__LINE__,(index)){SKIP_ELEM(out);}
#define FOREACH(type, node, list) \
	for( type* node = list[0],*CONCAT(i,__LINE__)=0; \
	(*((u32*)&CONCAT(i,__LINE__)))<LIST_SIZE(list); \
	(*((u32*)&CONCAT(i,__LINE__)))++, SKIP_ELEM(node))

#define PUSH_BACK(l, arena, out){\
	if(!l[0]){\
		void** cast = &(void*)(l[0]);\
		*cast = arena_push_size(arena, sizeof(*l[0]) + sizeof(l[0]));\
		l[1] = l[0];\
	}else{\
		void** p_last_element = (void**)(l[1]+1);\
		void** cast = &(void*)(l[1]);\
		*cast = arena_push_size(arena, sizeof(*l[0]) + sizeof(l[0]));\
		*p_last_element = l[1];\
	}\
	size_t* tempsize = ((size_t*)l)+2;\
	*tempsize += 1;\
	out = l[1];\
}
//TODO: #define PUSH_FRONT()

//TODO: i have never used this, make sure this even works
#define LIST_POP_FRONT(l) l[0]; *(&(void*)l[0]) = *(void**)(l[0]+1); *((u32*)&(l[2])) -= 1

// THIS PRODUCES MEMORY LEAKS, this doesn't actually erase the node, it just skips it
#define LIST_ERASE_NEXT_ELEM(list, node) NEXT_ELEM(node) = NEXT_ELEM(NEXT_ELEM(node)); LIST_SIZE(l) -= 1
#define LIST_ERASE_ELEM(l, index) { \
      size_t pointer_size = sizeof(*l[0]); \
      ASSERT((index)<LIST_SIZE(l)); \
      void** next_elem_pointer = (void**)&l[0];\
      void** prev_elem_pointer = next_elem_pointer;\
      UNTIL(CONCAT(unique_index,__LINE__), (index))\
      {\
         u8* next_elem = ((u8*)*next_elem_pointer);\
         prev_elem_pointer = next_elem_pointer;\
         next_elem_pointer = (void**)(next_elem+pointer_size);\
      }\
      if(l[1] == *next_elem_pointer) *(void**)&l[1] = *prev_elem_pointer;\
      *next_elem_pointer = *(void**)(((u8*)*next_elem_pointer)+pointer_size);\
      *((size_t*)&l[2]) -= 1;\
   }


// TEMPORARILY ORPHAN 


// THIS APPLIES TO ANY TEXTURE NOT JUST FONTS
struct Tex_info{
	u32 texture_uid;
	s32 w, h; // texture dimensions

	// This are in case this is a texture from an atlas
	
	// the offsets are in pixels for uncentered textures
	s32 xoffset, yoffset;
	// this are normalized coordinates 0.0->1.0 with 1.0 being the atlas full width/height;
	Rect_float texrect; 
};

struct Font
{
	u32* texinfo_uids;
	u16 texinfos_count;
	u16 atlas_texinfo_uid;

	f32 lines_height; // this is a f32 cuz for some reason the stb_tt asks for a float

	u32 first_char;

	f32 ascent;
	f32 descent;
};

union Color
{
	struct{
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	};
	struct RGB_accessor{
		f32 r;
		f32 g;
		f32 b;
	} rgb;
	V4 v4;
};
internal Color
init_color(f32 r, f32 g, f32 b, f32 a)
{
	return {r,g,b,a};
}
internal b32
compare_colors(Color a, Color b)
{
	return a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a;
}
internal Color
operator *(f32 scalar, Color color){
	return {scalar*color.r, scalar*color.g, scalar*color.b, scalar*color.a};
}

internal Color
color_difference(Color c1, Color c2)
{
	return {c1.r-c2.r, c1.g-c2.g, c1.b-c2.b, c1.a-c2.a};
}

internal Color
operator -(Color c1, Color c2)
{
	return color_difference(c1, c2);
}

internal Color
color_addition(Color c1, Color c2)
{
	return {c1.r+c2.r, c1.g+c2.g, c1.b+c2.b, c1.a+c2.a};
}
internal Color
operator +(Color c1, Color c2)
{
	return color_addition(c1, c2);
}

internal Color
colors_product(Color c1, Color c2){
	return {c1.r*c2.r, c1.g*c2.g, c1.b*c2.b, c1.a*c2.a};
}

internal Color
operator *(Color c1, Color c2){
	return colors_product(c1, c2);
}


struct Color32
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

struct V4_u8
{
	u8 x;
	u8 y;
	u8 z;
	u8 w;
};

struct Color_u16
{
	u16 r;
	u16 g;
	u16 b;
	u16 a;
};

union Indices_triad16
{
   u16 indices[3];
   struct{
      u16 i1;
      u16 i2;
      u16 i3;
   };
};
typedef Indices_triad16 Triangle;

union Indices_triad32
{
	u32 indices[3];
	struct{
		u32 i1, i2, i3;
	};
};

internal Indices_triad16
indices_triad32_to_16(Indices_triad32 t)
{
	return {(u16)t.i1, (u16)t.i2, (u16)t.i3};
}
internal Indices_triad32
indices_triad16_to_32(Indices_triad16 t)
{
	return {t.i1, t.i2, t.i3};
}

struct Vertex
{
	V3 pos;
	V2 texcoord;
	V3 normal;

	union
	{
		V3 weights;
		f32 weights_array[3];
	};

	union
	{
		u32 bone_indices[3];
		Indices_triad32 bone_indices_triad;
	};
};

struct Voxel_chunk_vertex
{
	V3 pos;
	// Int2 texcoord;
	// V3 normal; // TURN THIS INTO JUST 6 different values
};

struct Sound_sample{
	s16* samples;
	u32 samples_count;
	u32 channels;
};

struct Audio_playback{
	u32 sound_uid;
	u32 initial_sample_t; 
	// with a u32 i could have 12.6 hours until it overflows (maybe 25)
	b32 loop;
};

struct Audio{
	Audio_playback* playbacks_array;
	u32 sample_t;
};

struct Selection_indices
{
	u16 hot;
	
	u16 pressed; // cursor primary button
	u16 clicked;

	u16 pressed2; // cursor secondary button
	u16 clicked2;

	// u16 selected;
};
static Selection_indices NULL_SELECTION_INDICES = {NULL_INDEX16,NULL_INDEX16,NULL_INDEX16};


internal Audio_playback*
 find_next_available_playback(Audio_playback* list){
	Audio_playback* result = 0;
	UNTIL(i, ARRAYLEN(list)){
		if(!list[i].initial_sample_t){
			result = &list[i];
			break;
		}
	}
	ASSERT(result);
	return result;
}

internal V2
calculate_delta_velocity(V2 velocity, V2 acceleration, f32 friction)
{// acceleration should be (paralysis_multiplier*entity->speed*(!entity->freezing_time_left)*entity->normalized_accel)
	return  (acceleration - (friction*velocity));
}

internal V2
calculate_delta_velocity_xy_frictions(V2 velocity, V2 acceleration, V2 frictions)
{
	return  acceleration - (v2_component_wise_product(frictions, velocity));
}


internal f32
px_x_to_screen(int pixels, Int2 client_size)
{
	return pixels * 2.0f / client_size.x;
}