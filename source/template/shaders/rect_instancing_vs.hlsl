#include "shader_header.h"


VS_OUTPUT_DEFAULT vs(VS_INPUT_DEFAULT input)
{
   VS_OUTPUT_DEFAULT result;
   result.normal = float3(0,0,-1);
   float4 vertex_world_pos = mul(input.instance_object_transform, float4(input.vertex_pos, 1.0f));
   result.vertex_world_pos = vertex_world_pos.xyz;
   // result.pixel_pos =  mul(projection_matrix, mul(world_view_matrix, vertex_world_pos)) ;
   result.pixel_pos = vertex_world_pos;
   result.pixel_pos.z = result.pixel_pos.z/1000.0f;
   result.camera_world_pos = camera_pos;

	result.texcoord.x = input.instance_texrect.x + (input.texcoord.x * input.instance_texrect.z);
	result.texcoord.y = input.instance_texrect.y + (input.texcoord.y * input.instance_texrect.w);

   result.color = input.instance_color;
   return result;
}