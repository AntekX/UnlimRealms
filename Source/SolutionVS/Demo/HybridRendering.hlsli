
#pragma pack_matrix(row_major)

#include "LogDepth.hlsli"
#include "Lighting.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

struct SceneConstants
{
	float4x4 viewProj;
	float4x4 viewProjInv;
	float4 cameraPos;
	float4 cameraDir;
};

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
	float4 target2	: SV_Target2;
};
