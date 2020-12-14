
#pragma pack_matrix(row_major)

#include "LogDepth.hlsli"
#include "Lighting.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

struct SceneConstants
{
	float4x4 viewProj;
};

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);

struct MeshVertexInput
{
	float3 pos	: POSITION;
	float3 norm : NORMAL;
};

struct MeshPixelInput
{
	float4 pos	: SV_POSITION;
	float3 norm : NORMAL;
};

struct MeshPixelOutput
{
	float4 target0	: SV_Target0;
	float4 target1	: SV_Target1;
};
