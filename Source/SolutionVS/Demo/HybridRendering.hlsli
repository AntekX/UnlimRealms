
#pragma pack_matrix(row_major)

#include "LogDepth.hlsli"
#include "Lighting.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

struct SceneConstants
{
	float4x4 Proj;
	float4x4 ViewProj;
	float4x4 ViewProjInv;
	float4 CameraPos;
	float4 CameraDir;
	float4 SourceSize; // w, h, 1/w, 1/h
	float4 TargetSize; // w, h, 1/w, 1/h
	float4 DebugVec0;
	bool OverrideMaterial;
	float3 __pad0;
	LightingDesc Lighting;
	AtmosphereDesc Atmosphere;
	MeshMaterialDesc Material;
};

struct MeshVertexInput
{
	float3 Pos		: POSITION;
	float3 Norm		: NORMAL;
	float2 TexCoord	: TEXCOORD;
};

struct MeshPixelInput
{
	float4 Pos		: SV_POSITION;
	float3 Norm		: NORMAL;
	float2 TexCoord	: TEXCOORD;
};

struct MeshPixelOutput
{
	float4 Target0	: SV_Target0;
	float4 Target1	: SV_Target1;
	float4 Target2	: SV_Target2;
};
