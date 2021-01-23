
#pragma pack_matrix(row_major)

#include "LogDepth.hlsli"
#include "Lighting.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

struct SceneConstants
{
	float4x4 Proj;
	float4x4 ViewProj;
	float4x4 ViewProjInv;
	float4x4 ViewProjPrev;
	float4x4 ViewProjInvPrev;
	float4 CameraPos;
	float4 CameraDir;
	float4 TargetSize; // w, h, 1/w, 1/h
	float4 LightBufferSize; // w, h, 1/w, 1/h
	float4 DebugVec0;
	bool OverrideMaterial;
	uint FrameNumber;
	uint SamplesPerLight;
	bool PerFrameJitter;
	float2 LightBufferDownscale; // d, 1/d
	float2 __pad0;
	LightingDesc Lighting;
	AtmosphereDesc Atmosphere;
	MeshMaterialDesc Material;
};

// Instance format must be up to date with GrafAccelerationStructureInstance
static const uint InstanceStride = 64;

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

// Shadow buffer
// following setup considers R32_UINT format

static const uint ShadowBufferAccumulatedSamples = 0xff;
static const uint ShadowBufferEntriesPerPixel = 4;
static const uint ShadowBufferEntryMask = 0xff;
static const uint ShadowBufferBitsPerEntry = 8;

float ShadowBufferEntryUnpack(uint entryPacked)
{
	return float(entryPacked) / ShadowBufferEntryMask;
}

uint ShadowBufferEntryPack(float shadowFactor)
{
	return (uint)min(floor(shadowFactor * ShadowBufferEntryMask + 0.5), ShadowBufferEntryMask);
}

float ShadowBufferGetLightOcclusion(uint bufferData, uint lightIdx)
{
	uint entryData = (bufferData >> (lightIdx * ShadowBufferBitsPerEntry)) & ShadowBufferEntryMask;
	return ShadowBufferEntryUnpack(entryData);
}

// Ray tracing shader table
static const uint RTShaderId_MissDirect = 0;

struct RayDataDirect
{
	// TODO
	bool occluded;
};

// Gemotry buffer utils

struct GBufferData
{
	float ClipDepth;
	float3 Normal;
};

struct GBufferPacked
{
	float Depth;
	float4 Image0;
	float4 Image1;
	float4 Image2;
};

GBufferData UnpackGBufferData(in GBufferPacked gbPacked)
{
	GBufferData gbData = (GBufferData)0;

	gbData.ClipDepth = gbPacked.Depth;
	gbData.Normal = gbPacked.Image1.xyz;

	return gbData;
}

GBufferData LoadGBufferData(in int2 imagePos,
	in Texture2D<float>		geometryDepth,
	in Texture2D<float4>	geometryImage0,
	in Texture2D<float4>	geometryImage1,
	in Texture2D<float4>	geometryImage2)
{
	GBufferPacked gbPacked = (GBufferPacked)0;

	gbPacked.Depth = geometryDepth.Load(int3(imagePos.xy, 0));
	gbPacked.Image0 = geometryImage0.Load(int3(imagePos.xy, 0));
	gbPacked.Image1 = geometryImage1.Load(int3(imagePos.xy, 0));
	gbPacked.Image2 = geometryImage2.Load(int3(imagePos.xy, 0));

	return UnpackGBufferData(gbPacked);
}