#include "shader_header.h"

VS_OUTPUT_DEFAULT vs( VS_INPUT_DEFAULT input )
{
	VS_OUTPUT_DEFAULT result;

	float4 vertex_world_pos = mul(object_transform, float4(input.vertex_pos, 1.0f) );
	result.vertex_world_pos = vertex_world_pos.xyz;
	result.pixel_pos = mul( projection_matrix,mul(world_view_matrix, vertex_world_pos));

	result.texcoord.x = object_texrect.x + (input.texcoord.x * object_texrect.z);
	result.texcoord.y = object_texrect.y + (input.texcoord.y * object_texrect.w);
	result.color = object_color;
	// result.normal = mul( (float3x3)object_transform , input.normal);
	result.normal = mul( (float3x3)object_transform , input.normal);

	result.camera_world_pos = camera_pos;


	return result;
}
