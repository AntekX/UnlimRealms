///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "../Atmosphere/AtmosphericScattering.hlsli"

cbuffer Common : register(b0)
{
    float4x4 ViewProj;
    float4 CameraPos;
    AtmosphereDesc Atmosphere;
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