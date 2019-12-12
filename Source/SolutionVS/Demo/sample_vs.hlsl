static const uint InstanceCount = 24;
cbuffer Common : register(b0)
{
	float4x4 Transform[InstanceCount];
	float4 Desc;
};

struct VS_INPUT
{
	float3 pos	: POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};

static const uint ColorsCount = 7;
static const float3 Colors[ColorsCount] = {
	{ 0, 0, 1 },
	{ 0, 1, 0 },
	{ 0, 1, 1 },
	{ 1, 0, 0 },
	{ 1, 0, 1 },
	{ 1, 1, 0 },
	{ 1, 1, 1 }
};

PS_INPUT main(VS_INPUT input, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
	PS_INPUT output;

	output.pos = float4(input.pos.xyz, 1.0);
	//output.pos = mul(Transform[instanceID + uint(Desc.x)], float4(input.pos.xyz, 1.0f));
	//output.pos = float4(input.pos.xyz, 1.0f);
	output.col = input.col;
	//output.col.xyz = Colors[(instanceID + vertexID) % ColorsCount] * 0.5 + 0.5;
	//output.col.xyz = Colors[uint(Desc.x / (InstanceCount / 3))];
	output.col.xyz = 1.0;
	
	output.uv = input.uv;

	return output;
}