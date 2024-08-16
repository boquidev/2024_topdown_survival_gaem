#include "shader_header.h"


VS_OUTPUT_DEFAULT vs( VS_INPUT_DEFAULT input )
{
   VS_OUTPUT_DEFAULT result;
   result.pixel_pos = float4(input.vertex_pos, 1.0f);
   result.texcoord = input.texcoord;
   result.normal = 0;
   result.color = 0;
   result.vertex_world_pos = 0;
   result.camera_world_pos = 0;

   return result;
}



// Sobel filter coefficients for horizontal and vertical directions
static const float3x3 SobelHorizontal =
{
   -1, 0, 1,
   -2, 0, 2,
   -1, 0, 1
};

static const float3x3 SobelVertical =
{
   -1, -2, -1,
   0,  0,  0,
   1,  2,  1
};

// PIXEL SHADER


float4 ps( VS_OUTPUT_DEFAULT input, uint tid : SV_PrimitiveID ) : SV_TARGET
{
   float4 result;

   // Initialize gradients
   float2 depth_gradient = float2(0,0);
   float2 normal_gradient = float2(0,0);
   float4 depth_value = depth_texture.Sample(sampler0, input.texcoord);

   float closer_depth = depth_value.w;

   // Sample the texture multiple times using the Sobel filter
   for (int i = -1; i <= 1; ++i)
   {
      for (int j = -1; j <= 1; ++j)
      {
         float2 sample_coord = input.texcoord + float2(i, j) / screen_size.xy;

         // Sample the depth texture
         float4 sample_value = depth_texture.Sample(sampler0, sample_coord);

         float depth_sample = sample_value.w;
         float3 normal_sample = sample_value.rgb;

         // Apply the Sobel filter to calculate gradients
         float sobel_h_value = SobelHorizontal[i + 1][j + 1];
         float sobel_v_value = SobelVertical[i + 1][j + 1];

         depth_gradient += float2(depth_sample * sobel_h_value, depth_sample * sobel_v_value);
         closer_depth = max(closer_depth, max(abs(depth_sample*sobel_h_value), abs(depth_sample*sobel_v_value))/2);

         // TODO: this is probably super wrong
         float normal_difference = length(depth_value.rgb-normal_sample)/20 ;
         float normal_x = sobel_h_value * (2-(1+normal_difference));
         float normal_y = sobel_v_value * (2-(1+normal_difference));
         normal_gradient += float2(normal_x, normal_y);
      }
   }

   // Calculate the magnitude of the gradient
   // float magnitude = (
   //    // abs(gradientX.x)+
   //    // abs(gradientX.y)+
   //    // abs(gradientX.z)+
   //    abs(gradientX) + 
   //    // abs(gradientY.x)+
   //    // abs(gradientY.y)+
   //    // abs(gradientY.z)+
   //    abs(gradientY)
   //    ) / 6;
   // float magnitude = (length(gradientX) + length(gradientY))/6;
   // float magnitude = (length(depth_gradient) + length(normal_gradient));
   // float magnitude = length(depth_gradient);
   float magnitude = (closer_depth-depth_value.w);
   

   // higher value means less discrimination between similar depths
   // lower value means more sensitivity between similar depths
   float min_value = 0.03f;
   float dif = 0.0001f;
   float multiplier = 1.0f/dif; 
   

   float interpolator = saturate(multiplier*(magnitude-min_value));

   float4 original_color = color_texture.Sample(sampler0, input.texcoord);

   float map_count = 255;
   float4 mapped_color = float4(
      ceil(original_color.r*map_count)/map_count, 
      ceil(original_color.g*map_count)/map_count, 
      ceil(original_color.b*map_count)/map_count, 1);
   // float4 mapped_color = original_color;

   //this is just to avoid conditionals for the black border lines
   result = lerp(mapped_color, float4(.0f , .0f, .0f,1), interpolator);

   // clip(length(gradientX.xyz)-6); 

   // result = float4(length(gradientX)/10, length(gradientY)/10, 0,1);

   // result = float4(depth_value.w, 0, 0, 1);
   // result = float4(depth_value.rgb, 1);
   // result = float4(magnitude, 0,0,1);


   // clip(magnitude-min_value);
   // result = (magnitude > min_value) ? float4(0,0,0,1) : float4(1,1,1,1);
   // result = float4(1-ease_in_circular(depth_texture.Sample(sampler0, input.texcoord ).w), 0, 0,1);
   
   // result = float4(depth_texture.Sample(sampler0, input.texcoord ).w, 0, 0,1);



   // result = color_texture.Sample( sampler0, input.texcoord);
   // result = float4(ease_in_circular(ease_in_circular(depth_texture.Sample( sampler0, input.texcoord ).w)), 0,0,1);
   float a = depth_value.w;
   // result = float4(a,a,a,1);
   return result;
}
