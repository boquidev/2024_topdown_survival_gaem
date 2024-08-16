#include "shader_header.h"

VS_OUTPUT_DEFAULT vs( VS_INPUT_DEFAULT input )
{
	VS_OUTPUT_DEFAULT result;
	float4 vertex_pos = float4( input.vertex_pos, 1.0f );
	//why this has w = 0.0 instead of 1.0 ????
	// i think it's cuz it is already normalized but i know shit
	float4 vertex_normal = float4( input.normal, 0.0f);

	float4 total_local_pos = float4(0,0,0,0);
	float4 total_normal = float4(0,0,0,0);

	for(int i=0; i<WEIGHTS_PER_VERTEX; i++)
	{
		matrix bone_transform = bone_transforms[input.bone_indices[i]];

		total_local_pos += mul( mul(bone_transform, vertex_pos) , input.weights[i]);
		total_normal += mul( mul(bone_transform, vertex_normal), input.weights[i]);
		// total_normal += mul( mul( (float3x3)bone_transform, input.normal), input.weights[i]);
	}


	float4 vertex_world_pos = mul(object_transform, total_local_pos );
	result.vertex_world_pos = vertex_world_pos.xyz;
	result.pixel_pos = mul( projection_matrix, mul(world_view_matrix, vertex_world_pos));
	// a little funny deformation
	// vertex_world_pos.y += 5*sin(vertex_world_pos.x*10)/10;

	result.texcoord.x = object_texrect.x + (input.texcoord.x * object_texrect.z);
	result.texcoord.y = object_texrect.y + (input.texcoord.y * object_texrect.w);
	result.color = object_color;	
	// result.normal = mul( (float3x3)object_transform , input.normal);
	result.normal = mul( (float3x3)object_transform , total_normal.xyz);

	// maybe get this from cpu
	// result.camera_world_pos.xyz = normalize(mul(float4(0,0,1,1), world_rotation).xyz);
	// result.camera_world_pos.xyz = lerp(result.camera_world_pos.xyz, camera_pos.xyz, camera_pos.w);
	// result.camera_world_pos.w = camera_pos.w;
	result.camera_world_pos = camera_pos;


	return result;
}
