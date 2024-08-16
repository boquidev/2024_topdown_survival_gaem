#include "shader_header.h"

VS_OUTPUT_DEFAULT vs(VS_INPUT_DEFAULT input){
	VS_OUTPUT_DEFAULT result;

	result.pixel_pos = mul(object_transform , float4(input.vertex_pos, 1));
	result.color = object_color;
	result.texcoord.x = input.texcoord.x;
	result.texcoord.y = input.texcoord.y;

	
	result.normal = 0;
	result.vertex_world_pos = result.pixel_pos.xyz;
	result.camera_world_pos = 0;

	return result;
}