//cbuffer Common : register(b0)
//{
//	float4x4 ViewProj;
//};

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

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output;

	//output.pos = mul(ViewProj, float4(input.pos.xyz, 1.0f));
	output.pos = float4(input.pos.xyz, 1.0f);
	output.col = input.col;
	output.uv = input.uv;

	return output;
}