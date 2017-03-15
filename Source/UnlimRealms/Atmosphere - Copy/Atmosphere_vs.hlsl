#include "Atmosphere.hlsli"

cbuffer Common : register(b0)
{
	float4x4 ViewProj;
	float4 CameraPos;
};

struct VS_INPUT
{
	float3 pos	: POSITION;
};

struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float3 wpos	: TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output;

	output.pos = mul(ViewProj, float4(input.pos.xyz, 1.0f));
	output.wpos.xyz = input.pos.xyz;

	return output;
}