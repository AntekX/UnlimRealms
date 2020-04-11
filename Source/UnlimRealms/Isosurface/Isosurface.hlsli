///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "LightingCommon.hlsli"
#include "../Atmosphere/AtmosphericScattering.hlsli"

cbuffer Common : register(b0)
{
	float4x4 ViewProj;
	float4 CameraPos;
	AtmosphereDesc Atmosphere;
	LightDesc Light;
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
	float4 wpos : TEXCOORD0;
};