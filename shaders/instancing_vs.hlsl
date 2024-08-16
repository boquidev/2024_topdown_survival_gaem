#include "shader_header.h"


VS_OUTPUT_DEFAULT vs(VS_INPUT_DEFAULT input)
{
   VS_OUTPUT_DEFAULT result;
   result.normal = float3(0,0,-1);
   float4 vertex_world_pos = mul(input.instance_object_transform, float4(input.vertex_pos, 1.0f));
   result.vertex_world_pos = vertex_world_pos.xyz;
   result.pixel_pos = mul( projection_matrix, mul( world_view_matrix, vertex_world_pos) );
   result.camera_world_pos = camera_pos;

   result.texcoord = input.texcoord; //TODO: add texrect to the instance data
   result.color = input.instance_color;

   
   return result;
}