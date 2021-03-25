
#pragma pack_matrix(row_major)

#include "LogDepth.hlsli"
#include "Lighting.hlsli"
#include "HDRRender/HDRRender.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

struct SceneConstants
{
	float4x4 View;
	float4x4 Proj;
	float4x4 ViewProj;
	float4x4 ViewProjInv;
	float4x4 ViewPrev;
	float4x4 ProjPrev;
	float4x4 ViewProjPrev;
	float4x4 ViewProjInvPrev;
	float4 CameraPos;
	float4 CameraDir;
	float4 TargetSize; // w, h, 1/w, 1/h
	float4 LightBufferSize; // w, h, 1/w, 1/h
	float4 PrecomputedSkySize;
	float4 DebugVec0;
	float2 LightBufferDownscale; // d, 1/d
	float2 __pad0;
	uint FrameNumber;
	uint SamplesPerLight;
	uint AccumulationFrameCount;
	bool PerFrameJitter;
	float3 __pad1;
	bool OverrideMaterial;
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

#define SHADOW_BUFFER_APPLY_HISTORY_IN_RAYGEN 0 // allows to apply temporal accumulation without separate filter pass
#define SHADOW_BUFFER_ONE_LIGHT_PER_FRAME 0 // enables light sources tracing distribution between multiple frames (requires temporal accumulation enabled)
#define SHADOW_BUFFER_UINT32 0

#if (SHADOW_BUFFER_UINT32)

// following setup considers R32_UINT format
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

#else

// 4 channel unorm or float buffer
static const uint ShadowBufferEntriesPerPixel = 4;

#endif // shadow buffer

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
	float3 BaseColor;
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
	gbData.BaseColor = gbPacked.Image0.xyz;

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

// Lighting common

MaterialInputs GetMaterial(const SceneConstants sceneCB, const GBufferData gbData)
{
	MaterialInputs material = (MaterialInputs)0;
	
	initMaterial(material);
	material.normal = gbData.Normal;
	if (sceneCB.OverrideMaterial)
	{
		material.baseColor.xyz = sceneCB.Material.BaseColor;
		material.roughness = sceneCB.Material.Roughness;
		material.metallic = sceneCB.Material.Metallic;
		material.reflectance = sceneCB.Material.Reflectance;
	}
	else
	{
		material.baseColor.xyz = gbData.BaseColor.xyz;
		material.roughness = sceneCB.Material.Roughness;
		material.metallic = sceneCB.Material.Metallic;
		material.reflectance = sceneCB.Material.Reflectance;
	}

	return material;
}

LightingParams GetMaterialLightingParams(const SceneConstants sceneCB, const GBufferData gbData, const float3 worldPos)
{
	MaterialInputs material = GetMaterial(sceneCB, gbData);
	LightingParams lightingParams;
	getLightingParams(worldPos, sceneCB.CameraPos.xyz, material, lightingParams);
	return lightingParams;
}

float4 CalculateSkyLight(const SceneConstants sceneCB, const float3 position, const float3 direction)
{
	float height = lerp(sceneCB.Atmosphere.InnerRadius, sceneCB.Atmosphere.OuterRadius, 0.05);
	float4 color = 0.0;
	for (uint ilight = 0; ilight < sceneCB.Lighting.LightSourceCount; ++ilight)
	{
		LightDesc light = sceneCB.Lighting.LightSources[ilight];
		if (LightType_Directional != light.Type)
			continue;
		float3 worldFrom = position + float3(0.0, height, 0.0);
		float3 worldTo = worldFrom + direction;
		color += AtmosphericScatteringSky(sceneCB.Atmosphere, light, worldTo, worldFrom);
	}
	color.w = min(1.0, color.w);
	return color;
}

float3 SkyImagePosToDirection(const SceneConstants sceneCB, const uint2 imagePos)
{
	float2 imageUV = (float2(imagePos)+0.5) * sceneCB.PrecomputedSkySize.zw;
	float azimuth = imageUV.x * TwoPi;
	float inclination = imageUV.y * Pi - HalfPi;
	float3 direction;
	float inclinationCos = cos(inclination);
	direction.x = cos(azimuth) * inclinationCos;
	direction.z = sin(azimuth) * inclinationCos;
	direction.y = sin(inclination);
	return direction;
}

float2 SkyImageUVFromDirection(const SceneConstants sceneCB, const float3 direction)
{
	float inclination = asin(direction.y) + HalfPi;
	float azimuth = atan2(direction.z, direction.x);
	if (azimuth < 0) azimuth = TwoPi + azimuth;
	float2 imageUV;
	imageUV.x = azimuth * OneOverTwoPi;
	imageUV.y = inclination * OneOverPi;
	return imageUV;
}

float4 GetSkyLight(const SceneConstants sceneCB, const Texture2D<float4> precomputedSky, sampler skySampler,
	const float3 position, const float3 direction)
{
	float2 skyUV = SkyImageUVFromDirection(sceneCB, direction);
	float4 skyLight = precomputedSky.SampleLevel(skySampler, skyUV, 0);
	return skyLight;
}

// Sampling

static const int2 QuadSampleOfs[4] = {
	{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }
};