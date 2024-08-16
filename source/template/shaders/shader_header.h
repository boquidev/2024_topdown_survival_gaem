
// VERTEX CONSTANT BUFFERS

cbuffer object_buffer : register(b0)
{
	matrix object_transform; 
	float4 object_color;
	float4 object_texrect;
};

cbuffer projection_view_buffer : register(b1)
{
	matrix world_view_matrix;
};
cbuffer projection_view_buffer : register(b2)
{
	matrix projection_matrix;
};

cbuffer camera_pos_buffer : register(b3){
	float4 camera_pos;
	// float3 camera_pos;
	// float depth_writing;
	// float3 camera_looking_direction;
}

static const int WEIGHTS_PER_VERTEX = 3;
static const uint MAX_BONE_COUNT = 100;
cbuffer bone_transforms_buffer : register(b4)
{
	float4x4 bone_transforms [MAX_BONE_COUNT];
}

// PIXEL CONSTANT BUFFERS

cbuffer Constants : register(b0)
{
	int4 screen_size;
}

cbuffer Constant : register(b1)
{
   float time;
   // float outer_border;
   // float inner_border;
};


Texture2D<float4> color_texture : register(t0);
Texture2D<float4> depth_texture : register(t1);
sampler sampler0 : register(s0);


// STRUCTS


struct VS_INPUT_DEFAULT
{
   float3 vertex_pos : POSITION;
   float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;

   float3 weights : BONE_WEIGHTS;
   uint3 bone_indices : BONE_INDICES;

   // PER INSTANCE DATA

   float4x4 instance_object_transform : INSTANCE_OBJECT_TRANSFORM;
   float4 instance_color : INSTANCE_COLOR;
	float4 instance_texrect : INSTANCE_TEXRECT;
};

struct VS_OUTPUT_DEFAULT
{
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
	float4 color : COLOR0;

	float3 vertex_world_pos : COLOR1;
	
	// w value of camera is 1 or 0 depending on if perspective is on or off
	// it's kind of dumb and this should be passed directly to the pixel shader instead of always having to be passed through the vs
	float4 camera_world_pos : COLOR2;
};

struct PS_OUTPUT_DEFAULT
{
	float4 color : SV_Target0;
	float4 depth : SV_Target1;
};



// EXTRA

static const float3 DEBUG_COLORS [] = {
		float3(1,0,0),
		float3(0,1,0),
		float3(0,0,1),
		float3(0.5f,0,0),
		float3(0,0.5f,0),
		float3(0,0,0.5f),
		float3(0.5f,0.5f,0),
		float3(0.5f,0,0.5f),
		float3(0,0.5f,0.5f),
		float3(1,1,0),
		float3(1,0,1),
		float3(0,1,1),
		float3(0,0,0),
		float3(1,1,1),
		float3(0.5f,0.5f,0.5f),
		float3(0,0.5f,1),
		float3(1,0.5f,0),
		float3(0.5f,1,0),
		float3(0.5f,0,1),
		float3(0, 1, 0.5f),
		float3(1, 0, 0.5f),
	};
	
float ease_in_circular(float x)
{
   return 1-sqrt(1-(x*x));
}


// some pixel shader code that i borrowed from msdn docs
// it shows an example on how to use materials to perform Phong shading 

/*

cbuffer MaterialConstantBuffer : register(b2)
{
    float4 lightColor;
    float4 Ka;
    float4 Kd;
    float4 Ks;
    float4 shininess;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float3 outVec : POSITION0;
    float3 normal : NORMAL0;
    float3 light : POSITION1;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 L = normalize(input.light);
    float3 V = normalize(input.outVec);
    float3 R = normalize(reflect(L, input.normal));

    float4 diffuse = Ka + (lightColor * Kd * max(dot(input.normal, L), 0.0f));
    diffuse = saturate(diffuse);

    float4 specular = Ks * pow(max(dot(R, V), 0.0f), shininess.x - 50.0f);
    specular = saturate(specular);

    float4 finalColor = diffuse + specular;

    return finalColor;
}

*/