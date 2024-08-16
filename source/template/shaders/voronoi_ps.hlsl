#include "shader_header.h"

float4 ps(VS_OUTPUT_DEFAULT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
   float4 result;
   result.rg = input.texcoord;
   result.b = 0;
	result.a = 1;
	return result;
}