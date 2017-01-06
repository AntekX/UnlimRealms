cbuffer Common : register(b0)
{
	float4x4 ViewProj;
};

struct VS_INPUT
{
	float3 pos	: POSITION;
	float3 norm : NORMAL;
	float4 col	: COLOR0;
};

struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float3 norm : NORMAL;
	float4 col	: COLOR0;
	float3 wpos : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output;

	output.pos = mul(ViewProj, float4(input.pos.xyz, 1.0f));
	output.norm = input.norm;
	output.col = input.col;
	output.wpos = input.pos.xyz;

	return output;
}