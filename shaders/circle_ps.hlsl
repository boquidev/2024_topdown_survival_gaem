
#include "shader_header.h"


PS_OUTPUT_DEFAULT ps( VS_OUTPUT_DEFAULT input, uint tid : SV_PrimitiveID)
{
   PS_OUTPUT_DEFAULT result;
   float2 vector_from_center = float2((input.texcoord.x*2)-1, (input.texcoord.y*2)-1);
   clip(1-length(vector_from_center));

   float4 texcolor = color_texture.Sample(sampler0, input.texcoord);
   clip(texcolor.a - 0.0001f);
   result.color = float4(
      texcolor.r * input.color.r,
      texcolor.g * input.color.g,
      texcolor.b * input.color.b,
      texcolor.a * input.color.a
   );
      
	float pixel_value = 1-(length(input.vertex_world_pos-input.camera_world_pos.xyz)/100);
	result.depth = input.camera_world_pos.w *float4(pixel_value, pixel_value, pixel_value, 1);
   
   return result;
}