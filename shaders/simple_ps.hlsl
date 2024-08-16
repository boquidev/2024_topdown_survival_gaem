#include "shader_header.h"

float4 ps(VS_OUTPUT_DEFAULT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
	float4 texcolor = color_texture.Sample( sampler0, input.texcoord );
	clip(texcolor.a *input.color.a -0.05);

	float4 result = float4(
		texcolor.r * input.color.r, 
		texcolor.g * input.color.g,
		texcolor.b * input.color.b,
		texcolor.a * input.color.a);


	return result;
}