#include "definitions.h"


internal void
concat_char_strings(char* s1, char* s2, char* output)
{
	while(*s1)
	{
		*output = *s1;
		output++;
		s1++;
	}
	while(*s2)
	{
		*output = *s2;
		output++;
		s2++;
	}
}

internal b8
is_alphanumeric(u8 code)
{
   return 
        ('a' <= code && code <= 'z' )    ||
        ('A' <= code && code <= 'Z')    ||
        ('0' <= code && code <= '9')      ||
        ('_' == code)
        ;
}

internal b8
is_letter(u8 code)
{
	return 
        ('a' <= code && code <= 'z' )    ||
        ('A' <= code && code <= 'Z');
}

struct String
{
	char* text;
	u32 length;
};

internal String 
string(char* text)
{
	String result = {0};
	result.text = text;
	while(*text)
	{
		result.length++;
		text++;
	}
	return result;
}

internal b32 
compare_chars(char* s1, char* s2)
{
	while(*s1 && *s2)
	{
		if(*s1 != *s2){
			return false;
		}
		s1++;
		s2++;
	}
	return true;	
}

internal bool
compare_strings(String s1, String s2)
{
	if(s1.length != s2.length){
		return false;
	}
	for(u32 i=0 ; s1.text[i] && s2.text[i]; i++)
	{
		if(s1.text[i] != s2.text[i])
			return false;
	}
	return s1.text[0] == s2.text[0];
}

internal bool
compare_strings(String s1, char* s2)
{
	for(u32 i=0 ; s1.text[i] && s2[i] && i<s1.length; i++)
	{
		if(s1.text[i] != s2[i])
			return false;
	}
	return true;
}

// return value is -1 if substr is not a sub string of str, returns the pos it found the substring otherwise
internal s32
find_substring(String str, String substr){
	UNTIL(i, str.length-substr.length){
		if(compare_strings(substr, {str.text+i, substr.length})){
			return i;
		}
	}
	return -1;
}

internal char
char_to_number(char c)
{
    return c - 48;
}

internal s32
string_to_int(String s)
{
	ASSERT(s.length);
	s32 sign = 1;
	s32 bigger_digit_pos = 0;
	if(*s.text == '-')
	{
		bigger_digit_pos = 1;
		sign = -1;
	}
	s32 result = 0;
	s32 power_of_10 = 1;
	for(s32 i = s.length-1; bigger_digit_pos <= i ; i--)
	{
		char digit = char_to_number(s.text[i]);
		result += digit*power_of_10;
		power_of_10 *= 10;
	}
	return result * sign;
}

internal bool
string_to_bool(String s)
{
	if(!s.text || compare_strings(s, "false") || compare_strings(s, "False"))
		return false;
	else
	{
		if( compare_strings(s, "true") ||
			compare_strings(s, "True") )
			return true;
		else
			return false;
	}
}

internal String
bool_to_string(b32 b)
{
	if(b)
		return string("true");
	else
		return string("false");
}

internal String
u64_to_string(u64 n, Memory_arena* arena)
{
	String result = {0};
	result.text = (char*)arena_push_size(arena,0);

	if(!n)
	{
		*(char*)arena_push_size(arena, 1) = '0';
		arena_push_size(arena, 1);
		result.length = 1;
		return result;
	}
	u8 digits = 0;
	u64 temp = n;

	u32 i=0;
	while(temp)
	{
		temp = temp/10;
		i++;
		digits++;
	}	
	arena_push_size(arena, digits);
	result.length = i;
	for(;digits;digits--)
	{
		result.text[i-1] = '0' + (n%10);
		n = n/10;
		i--;		
	}
	*arena_push_size(arena, 1) = 0; // 0 ending string
	return result;
}

internal String
u32_to_string(u32 n, Memory_arena* arena)
{
	u32 i=0;
	String result = {0};
	result.text = (char*)arena_push_size(arena, 0);
	if(!n) // if number is 0
	{
		*(char*)arena_push_size(arena, 1) = '0';
		arena_push_size(arena, 1); // 0 ending string
		result.length = 1;
		return result;
	}
	u8 digits = 0;
	u32 temp = n;
	while(temp)
	{
		temp = temp/10;
		i++;
		digits++;
	}
	arena_push_size(arena, digits);
	result.length= i;
	for(;digits; digits--)
	{
		result.text[i-1] = '0' + (n%10);
		n = n/10;
		i--;
	}
	*arena_push_size(arena, 1) = 0; // 0 ending string
	return result;
}

// THIS NEEDS THE MEMORY ARENA
internal String 
s32_to_string(s32 n, Memory_arena* arena)
{
	u32 i=0;
	String result = {0};
	result.text = (char*)arena_push_size(arena, 0);
	if(n < 0)
	{ 
		arena_push_data(arena, "-", 1);
		n = -(n);
		i++;
	}
	if(!n) // if number is 0
	{
		*(char*)arena_push_size(arena, 1) = '0';
		arena_push_size(arena, 1); // 0 ending string
		result.length = 1;
		return result;
	}
	u8 digits = 0;
	s32 temp = n;
	while(temp)
	{
		temp = temp/10;
		i++;
		digits++;
	}
	arena_push_size(arena, digits);
	result.length= i;
	for(;digits; digits--)
	{
		result.text[i-1] = '0' + (n%10);
		n = n/10;
		i--;
	}
	arena_push_size(arena, 1); //0 ending string
	return result;
}

// THIS NEEDS THE MEMORY ARENA
internal String
concat_strings(String s1, String s2, Memory_arena* arena)
{
	String result = {0};
	result.length = s1.length + s2.length;
	result.text = (char*)arena_push_size(arena, result.length);
	copy_mem(s1.text, result.text, s1.length);
	copy_mem(s2.text, &result.text[s1.length], s2.length);

	arena_push_size(arena, 1); // 0 ending string
	return result;
}

internal String
filepath_substring_until_last_slash(String filepath, Memory_arena* arena)
{
	u32 last_slash_pos = 0;
	UNTIL(char_i, filepath.length)
	{
		if(filepath.text[char_i] == '/')
		{
			last_slash_pos = char_i;
		}
	}
	
	String result;

	if(last_slash_pos)
	{
		result.text = ARENA_PUSH_STRUCTS(arena, char, last_slash_pos + 2); // +1 null terminator
		result.length = last_slash_pos+1;
		copy_mem(filepath.text, result.text, result.length);
	}
	else
	{
		result.text = ARENA_PUSH_STRUCT(arena, char);
		result.length = 0;
	}

	return result;
}

internal String
buffer_and_length_to_string(char* buffer, u32 length)
{
	return {buffer, length};
}

internal u32
get_previous_word_from_cursor(String buffer, u32 cursor_pos)
{
	if(!cursor_pos)
	{
		return 0;
	}
	else
	{
		char pseudo_initial_char = buffer.text[cursor_pos-1];
		b8 is_initial_char_alphanumeric = true;
		b8 already_moved = false;
		while(cursor_pos
		&&
			(
				buffer.text[cursor_pos-1] == pseudo_initial_char
			|| ((is_initial_char_alphanumeric) && is_alphanumeric(buffer.text[cursor_pos-1])) 
			)
		)
		{
			cursor_pos--;
			if(!already_moved && cursor_pos)
			{
				pseudo_initial_char = buffer.text[cursor_pos-1];
				is_initial_char_alphanumeric = is_alphanumeric(pseudo_initial_char);
			}
			already_moved = true;
		}
		
		return cursor_pos;
	}
}

internal u32
get_next_word_from_cursor(String buffer, u32 cursor_pos)
{
	if(buffer.length <= cursor_pos)
	{
		return buffer.length;
	}
	else
	{
		char pseudo_initial_char = buffer.text[cursor_pos];
		b8 is_initial_char_alphanumeric = true;

		b8 already_moved = false;

		while(cursor_pos < buffer.length
		&&
			(
				buffer.text[cursor_pos] == pseudo_initial_char
			|| ((is_initial_char_alphanumeric) && is_alphanumeric(buffer.text[cursor_pos])) 
			)
		)
		{
			cursor_pos++;
			if(!already_moved)
			{
				pseudo_initial_char = buffer.text[cursor_pos];
				is_initial_char_alphanumeric = is_alphanumeric(pseudo_initial_char);
			}

			already_moved = true;
		}
		return cursor_pos;
	}
}