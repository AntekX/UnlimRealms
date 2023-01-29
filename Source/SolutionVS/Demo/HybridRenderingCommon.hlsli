#ifndef HYBRID_RENDERING_COMMON_HLSLI
#define HYBRID_RENDERING_COMMON_HLSLI

#include "ShaderLib/CommonTypes.hlsli"

// TEMP
#define RT_GI_TEST 1
#define RT_REFLECTION_TEST 0
#define RT_ALPHATEST 0
#define RT_ALPHATEST_VALUE 0.25

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
	CFLOAT4		(DebugVec2);
	CFLOAT4		(DebugVec3);
	CFLOAT2		(LightBufferDownscale); // d, 1/d
	CFLOAT		(DirectLightFactor);
	CFLOAT		(IndirectLightFactor);
	CFLOAT		(ShadowTemporalTolerance);
	CFLOAT		(ShadowTemporalThreshold);
	CFLOAT		(IndirectTemporalTolerance);
	CFLOAT		(IndirectTemporalThreshold);
	CUINT		(FrameNumber);
	CUINT		(ShadowSamplesPerLight);
	CUINT		(ShadowAccumulationFrames);
	CUINT		(IndirectSamplesPerFrame);
	CUINT		(IndirectAccumulationFrames);
	CUINT		(IndirectBouncesCount);
	CUINT		(PerFrameJitter);
	CUINT		(OverrideMaterial);
	//CUINT		(__pad1);
	LightingDesc Lighting;
	AtmosphereDesc Atmosphere;
	MeshMaterialDesc Material;
};

struct SubMeshConstants
{
	CUINT		(InstanceOfs);
	CUINT3		(__pad0);
};

struct BlurPassConstants
{
	CUINT		(PassIdx);
	CUINT		(KernelSizeFactor);
	CUINT2		(__pad0);
	CFLOAT4		(EdgeParams);
};

// sub mesh description
// resources indirection for bindless rendering

struct SubMeshDesc
{
	CUINT(VertexBufferDescriptor);
	CUINT(IndexBufferDescriptor);
	CUINT(PrimitivesOffset);
	CUINT(ColorMapDescriptor);
	CUINT(NormalMapDescriptor);
	CUINT(MaskMapDescriptor);
	CUINT(MaterialBufferIndex);
	CUINT(__pad0);
};
static const CUINT(SubMeshDescSize) = 32; // sizeof(SubMeshDesc)
static const CUINT(InstanceSize) = 64; // sizeof(GrafAccelerationStructureInstance)
static const CUINT(VertexSize) = 44; // sizeof(Mesh::Vertex)
static const CUINT(IndexSize) = 4; // sizeof(Mesh::Index)

// descriptors

DESCRIPTOR_ConstantBuffer(SceneConstants,		g_SceneCB,						0);
DESCRIPTOR_ConstantBuffer(SubMeshConstants,		g_SubMeshCB,					1);
DESCRIPTOR_ConstantBuffer(BlurPassConstants,	g_BlurPassCB,					2);
DESCRIPTOR_Sampler(								g_SamplerPoint,					0);
DESCRIPTOR_Sampler(								g_SamplerBilinear,				1);
DESCRIPTOR_Sampler(								g_SamplerBilinearWrap,			2);
DESCRIPTOR_Sampler(								g_SamplerTrilinear,				3);
DESCRIPTOR_Sampler(								g_SamplerTrilinearWrap,			4);
DESCRIPTOR_AccelerationStructure(				g_SceneStructure,				0);
DESCRIPTOR_ByteAddressBuffer(					g_InstanceBuffer,				1);
DESCRIPTOR_ByteAddressBuffer(					g_MeshDescBuffer,				2);
DESCRIPTOR_ByteAddressBuffer(					g_MaterialDescBuffer,			3);
DESCRIPTOR_Texture2D(float,						g_GeometryDepth,				4);
DESCRIPTOR_Texture2D(float4,					g_GeometryImage0,				5);
DESCRIPTOR_Texture2D(float4,					g_GeometryImage1,				6);
DESCRIPTOR_Texture2D(float4,					g_GeometryImage2,				7);
DESCRIPTOR_Texture2D(float4,					g_ColorTexture,					8);
DESCRIPTOR_Texture2D(float4,					g_NormalTexture,				9);
DESCRIPTOR_Texture2D(float4,					g_MaskTexture,					10);
DESCRIPTOR_Texture2D(float4,					g_PrecomputedSky,				11);
DESCRIPTOR_Texture2D(uint4,						g_TracingInfo,					12);
DESCRIPTOR_Texture2D(float4,					g_ShadowResult,					13);
DESCRIPTOR_Texture2D(float4,					g_IndirectLight,				14);
DESCRIPTOR_Texture2D(float,						g_DepthHistory,					15);
DESCRIPTOR_Texture2D(uint4,						g_TracingHistory,				16);
DESCRIPTOR_Texture2D(float4,					g_ShadowHistory,				17);
DESCRIPTOR_Texture2D(float4,					g_IndirectLightHistory,			18);
DESCRIPTOR_Texture2D(float4,					g_ShadowMips,					19);
DESCRIPTOR_Texture2D(float4,					g_BlurSource,					20);
DESCRIPTOR_RWTexture2D(float4,					g_PrecomputedSkyTarget,			0);
DESCRIPTOR_RWTexture2D(float4,					g_LightingTarget,				1);
DESCRIPTOR_RWTexture2D(uint4,					g_TracingInfoTarget,			2);
DESCRIPTOR_RWTexture2D(float4,					g_ShadowTarget,					3);
DESCRIPTOR_RWTexture2D(float4,					g_IndirectLightTarget,			4);
DESCRIPTOR_RWTexture2D(float4,					g_BlurTarget,					5);
DESCRIPTOR_RWTexture2D(float4,					g_ShadowMip1,					6);
DESCRIPTOR_RWTexture2D(float4,					g_ShadowMip2,					7);
DESCRIPTOR_RWTexture2D(float4,					g_ShadowMip3,					8);
DESCRIPTOR_RWTexture2D(float4,					g_ShadowMip4,					9);

// descriptor arrays

static const CUINT(g_TextureArraySize) = 512;
static const CUINT(g_BufferArraySize) = 256;
DESCRIPTOR_ARRAY_Texture2D(g_TextureArraySize,			g_Texture2DArray,	128);
DESCRIPTOR_ARRAY_ByteAddressBuffer(g_BufferArraySize,	g_BufferArray,		640);

#endif