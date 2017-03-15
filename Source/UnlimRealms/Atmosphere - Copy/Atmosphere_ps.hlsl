#include "Atmosphere.hlsli"

cbuffer Common : register(b0)
{
	float4x4 ViewProj;
	float4 CameraPos;
};

struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float3 wpos	: TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	return atmosphereScatteringSky(input.wpos.xyz, CameraPos.xyz);
}