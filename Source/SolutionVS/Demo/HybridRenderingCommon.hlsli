#ifndef HYBRID_RENDERING_COMMON_HLSLI
#define HYBRID_RENDERING_COMMON_HLSLI

#include "ShaderLib/CommonTypes.hlsli"

// constant buffers

struct SceneConstants
{
	CFLOAT4x4	(View);
	CFLOAT4x4	(Proj);
	CFLOAT4x4	(ViewProj);
	CFLOAT4x4	(ViewProjInv);
	CFLOAT4x4	(ViewPrev);
	CFLOAT4x4	(ProjPrev);
	CFLOAT4x4	(ViewProjPrev);
	CFLOAT4x4	(ViewProjInvPrev);
	CFLOAT4		(CameraPos);
	CFLOAT4		(CameraDir);
	CFLOAT4		(TargetSize); // w, h, 1/w, 1/h
	CFLOAT4		(LightBufferSize); // w, h, 1/w, 1/h
	CFLOAT4		(PrecomputedSkySize);
	CFLOAT4		(DebugVec0);
	CFLOAT4		(DebugVec1);
	CFLOAT2		(LightBufferDownscale); // d, 1/d
	CFLOAT		(DirectLightFactor);
	CFLOAT		(IndirectLightFactor);
	CUINT		(FrameNumber);
	CUINT		(ShadowSamplesPerLight);
	CUINT		(ShadowAccumulationFrames);
	CUINT		(IndirectSamplesPerFrame);
	CUINT		(IndirectAccumulationFrames);
	CUINT		(PerFrameJitter);
	CUINT		(OverrideMaterial);
	CUINT		(__pad1);
	LightingDesc Lighting;
	AtmosphereDesc Atmosphere;
	MeshMaterialDesc Material;
};

// descriptors

DESCRIPTOR_ConstantBuffer(SceneConstants,	g_SceneCB,						0);
DESCRIPTOR_Sampler(							g_SamplerBilinear,				0);
DESCRIPTOR_Sampler(							g_SamplerTrilinear,				1);
DESCRIPTOR_Sampler(							g_SamplerTrilinearWrap,			2);
DESCRIPTOR_AccelerationStructure(			g_SceneStructure,				0);
DESCRIPTOR_ByteAddressBuffer(				g_InstanceBuffer,				1);
DESCRIPTOR_Texture2D(float,					g_GeometryDepth,				2);
DESCRIPTOR_Texture2D(float4,				g_GeometryImage0,				3);
DESCRIPTOR_Texture2D(float4,				g_GeometryImage1,				4);
DESCRIPTOR_Texture2D(float4,				g_GeometryImage2,				5);
DESCRIPTOR_Texture2D(float4,				g_ColorTexture,					6);
DESCRIPTOR_Texture2D(float4,				g_NormalTexture,				7);
DESCRIPTOR_Texture2D(float4,				g_MaskTexture,					8);
DESCRIPTOR_Texture2D(float4,				g_PrecomputedSky,				9);
DESCRIPTOR_Texture2D(uint4,					g_TracingInfo,					10);
DESCRIPTOR_Texture2D(float4,				g_ShadowResult,					11);
DESCRIPTOR_Texture2D(float4,				g_IndirectLight,				12);
DESCRIPTOR_Texture2D(float,					g_DepthHistory,					13);
DESCRIPTOR_Texture2D(uint4,					g_TracingHistory,				14);
DESCRIPTOR_Texture2D(float4,				g_ShadowHistory,				15);
DESCRIPTOR_Texture2D(float4,				g_IndirectLightHistory,			16);
DESCRIPTOR_Texture2D(float4,				g_ShadowMips,					17);
DESCRIPTOR_Texture2D(float4,				g_BlurSource,					18);
DESCRIPTOR_RWTexture2D(float4,				g_PrecomputedSkyTarget,			0);
DESCRIPTOR_RWTexture2D(float4,				g_LightingTarget,				1);
DESCRIPTOR_RWTexture2D(uint4,				g_TracingInfoTarget,			2);
DESCRIPTOR_RWTexture2D(float4,				g_ShadowTarget,					3);
DESCRIPTOR_RWTexture2D(float4,				g_IndirectLightTarget,			4);
DESCRIPTOR_RWTexture2D(float4,				g_BlurTarget,					5);
DESCRIPTOR_RWTexture2D(float4,				g_ShadowMip1,					6);
DESCRIPTOR_RWTexture2D(float4,				g_ShadowMip2,					7);
DESCRIPTOR_RWTexture2D(float4,				g_ShadowMip3,					8);
DESCRIPTOR_RWTexture2D(float4,				g_ShadowMip4,					9);

#endif