#include "shader_header.h"

// Output color information to the first render target (e.g., RT0)
// float4 ColorOutput : SV_Target0;
// Output depth information to the second render target (e.g., RT1)
// float DepthOutput : SV_Target1;


PS_OUTPUT_DEFAULT ps( VS_OUTPUT_DEFAULT input, uint tid : SV_PrimitiveID)
{
	PS_OUTPUT_DEFAULT result;
	float4 texcolor = color_texture.Sample( sampler0, input.texcoord );
	float result_alpha = texcolor.a*input.color.a;
	clip(result_alpha-0.0001f);
	result.color = float4(
		texcolor.r * (0.5+input.color.r)/1.5, 
		texcolor.g * (0.5+input.color.g)/1.5,
		texcolor.b * (0.5+input.color.b)/1.5,
		result_alpha);

	float3 N = normalize(input.normal);
	//TODO: VERTICES STILL HAVE NO NORMALS SO THIS WILL ALWAYS BE 0
	// i don't know what this does

	float3 light_direction = normalize(float3(-1,-1, 1));
	// float light = saturate( (N.x+N.y+N.z) );
	float light = saturate((2*(-dot(N, light_direction))));

	// FRESNEL
	float3 camera_to_vertex = normalize(input.vertex_world_pos - input.camera_world_pos.xyz);
	// float3 camera_vector = lerp(input.camera_world_pos.xyz, camera_to_vertex, input.camera_world_pos.w);
	float3 camera_vector = camera_to_vertex;

	// this works better with a more realistic shading
	float fresnel = saturate(1+(2.0f*(dot(camera_vector, N)))); 
	// result.color.rgb *= fresnel;

	// CLIP MASK

	//TODO: get this value from a constant buffer or from the material of the vertices
	float FRESNEL_MULTIPLIER = 1.0;
	float clip_mask = FRESNEL_MULTIPLIER*(0.5f+(1.0f-saturate(fresnel)));
	// result.color.rgb *= clip_mask;

	// FINAL COLOR 
	float shadow = light-.1f*fresnel;
	
   float map_count = 2;// this value indicates how many light values, and how many shadow values there will be
   float mapped_light = float(ceil(shadow*map_count)/(map_count));
	result.color = saturate(result.color + (mapped_light*float4( .1f, .1f, .2f, 0)) - ((1-mapped_light)*(float4(1,1,1,0)-float4( .7f, .5f, .5f, 0))));
	

	float depth_value = 1-(length(input.vertex_world_pos-input.camera_world_pos.xyz)/100);

	// to be either 1 or 0
	float3 camera_right_vector = normalize(float3(camera_to_vertex.z, 0, -camera_to_vertex.x));
	float3 camera_up_vector = cross(camera_to_vertex, camera_right_vector);
	float3 normal_color = float3(dot(camera_right_vector, N), dot(camera_up_vector, N), -dot(camera_to_vertex, N));

	// FROM THE APP LAYER I CAN SET DEPTH WRITING AND THAT CHANGES THE W VALUE OF THE CAMERA POS
	float eased_depth_value = (ease_in_circular(depth_value));
	
	//TODO: WHEN DEPTH IS NOT 1, if THE blend_state is active it APPLIES THE ALPHA TO THE RGB VALUES 
	// SO THEY WILL NOT BE EXACTLY THE SAME WHEN READ FROM THE POST PROCESSING SHADER
	//TODO: make 2 different textures for normal and depth
	
	result.depth = input.camera_world_pos.w * float4(normal_color, eased_depth_value);
	// result.depth = float4(normal_color, 1);

   // result.color = result.depth;

	// result.color = float4(DEBUG_COLORS[(tid)],1);


	// result.depth = float4(1,1,1,1);
	
	return result;
}