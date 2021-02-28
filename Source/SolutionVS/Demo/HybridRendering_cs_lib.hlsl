
#include "HybridRendering.hlsli"
#include "HDRRender/HDRRender.hlsli"

// common bindings

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
sampler				g_SamplerBilinear		: register(s0);
sampler				g_SamplerTrilinear		: register(s1);
Texture2D<float>	g_GeometryDepth			: register(t0);
Texture2D<float4>	g_GeometryImage0		: register(t1);
Texture2D<float4>	g_GeometryImage1		: register(t2);
Texture2D<float4>	g_GeometryImage2		: register(t3);
Texture2D<float4>	g_ShadowResult			: register(t4);
Texture2D<uint2>	g_TracingInfo			: register(t5);
Texture2D<float>	g_DepthHistory			: register(t6);
Texture2D<float4>	g_ShadowHistory			: register(t7);
Texture2D<uint2>	g_TracingHistory		: register(t8);
Texture2D<float4>	g_ShadowMips			: register(t9);
RWTexture2D<float4>	g_LightingTarget		: register(u0);
RWTexture2D<float4>	g_ShadowMip1			: register(u1);
RWTexture2D<float4>	g_ShadowMip2			: register(u2);
RWTexture2D<float4>	g_ShadowMip3			: register(u3);
RWTexture2D<float4>	g_ShadowMip4			: register(u4);
RWTexture2D<float4>	g_ShadowTarget			: register(u5);
RWTexture2D<uint2>	g_TracingInfoTarget		: register(u6);


// lighting common

float4 CalculateSkyLight(const float3 position, const float3 direction)
{
	float height = lerp(g_SceneCB.Atmosphere.InnerRadius, g_SceneCB.Atmosphere.OuterRadius, 0.05);
	float4 color = 0.0;
	for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
	{
		LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
		if (LightType_Directional != light.Type)
			continue;
		float3 worldFrom = position + float3(0.0, height, 0.0);
		float3 worldTo = worldFrom + direction;
		color += AtmosphericScatteringSky(g_SceneCB.Atmosphere, light, worldTo, worldFrom);
	}
	color.w = min(1.0, color.w);
	return color;
}

// upsampling

static const int2 QuadSampleOfs[4] = {
	{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }
};

// compute lighting

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeLighting(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 imagePos = dispatchThreadId.xy;
	if (imagePos.x >= (uint)g_SceneCB.TargetSize.x || imagePos.y >= (uint)g_SceneCB.TargetSize.y)
		return;

	float clipDepth = g_GeometryDepth.Load(int3(imagePos.xy, 0));
	bool isSky = (clipDepth >= 1.0);
	float2 uvPos = (float2(imagePos) + 0.5) * g_SceneCB.TargetSize.zw;
	float3 clipPos = float3(float2(uvPos.x, 1.0 - uvPos.y) * 2.0 - 1.0, clipDepth);
	float3 worldPos = ClipPosToWorldPos(clipPos, g_SceneCB.ViewProjInv);
	float3 worldRay = worldPos - g_SceneCB.CameraPos.xyz;
	float worldDist = length(worldRay);
	worldRay /= worldDist;

	float3 lightingResult = 0.0;

	[branch] if (isSky)
	{
		lightingResult = CalculateSkyLight(g_SceneCB.CameraPos.xyz, worldRay).xyz;
	}
	else
	{
		// read geometry buffer

		GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);

		// read lighting buffer

		#if (SHADOW_BUFFER_UINT32)
		float shadowPerLight[ShadowBufferEntriesPerPixel];
		[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j) shadowPerLight[j] = 0.0;
		#else
		float4 shadowPerLight = 0.0;
		#endif

		float2 lightBufferPos = (float2(imagePos.xy) + 0.5) * g_SceneCB.LightBufferDownscale.y;
		#if (0)
		
		// point upsampling
		#if (SHADOW_BUFFER_UINT32)
		uint shadowPerLightPacked = g_ShadowResult.Load(int3(lightBufferPos.xy, 0));
		[unroll] for (/*uint */j = 0; j < ShadowBufferEntriesPerPixel; ++j)
		{
			shadowPerLight[j] = ShadowBufferGetLightOcclusion(shadowPerLightPacked, j);
		}
		#else
		shadowPerLight = g_ShadowResult.Load(int3(lightBufferPos.xy, 0));
		#endif
		
		#else
		
		// filtered upsampling
		/*uint frameHashOfs = g_SceneCB.FrameNumber * (uint)g_SceneCB.TargetSize.x * (uint)g_SceneCB.TargetSize.y * g_SceneCB.PerFrameJitter;
		uint pixelHash = HashUInt(imagePos.x + imagePos.y * (uint)g_SceneCB.TargetSize.x + frameHashOfs);
		float2 jitter2d = BlueNoiseDiskLUT64[pixelHash % 64] * 0.5;*/
		lightBufferPos -= 0.5; // offset to neighbourhood for bilinear filtering
		float2 pf = frac(lightBufferPos);
		float4 sampleWeight = {
			(1.0 - pf.x) * (1.0 - pf.y),
			pf.x * (1.0 - pf.y),
			(1.0 - pf.x) * pf.y,
			pf.x * pf.y
		};

		// per light buffer sample weights
		float4 tracingConfidence = 1.0;
		[unroll] for (uint i = 0; i < 4; ++i)
		{
			int2 lightSamplePos = int2(clamp(lightBufferPos + QuadSampleOfs[i], float2(0, 0), g_SceneCB.LightBufferSize.xy - 1));
			uint tracingSampleId = g_TracingInfo.Load(int3(lightSamplePos.xy, 0)).x;
			uint2 tracingSamplePos = uint2(tracingSampleId % (uint)g_SceneCB.LightBufferDownscale.x, tracingSampleId / (uint)g_SceneCB.LightBufferDownscale.x); // sub sample used in ray tracing pass
			uint2 tracingImagePos = tracingSamplePos + lightSamplePos * g_SceneCB.LightBufferDownscale.x; // full res position used for tracing
			GBufferData tracingGBData = LoadGBufferData(tracingImagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
			#if (0)
			float depthDeltaTolerance = clipDepth * 1.0e-4;
			tracingConfidence[i] *= 1.0 - saturate(abs(tracingGBData.ClipDepth - clipDepth) / depthDeltaTolerance);
			#else
			float surfaceDistTolerance = worldDist * 1.0e-2;
			float3 tracingOrigin = ImagePosToWorldPos(tracingImagePos, g_SceneCB.TargetSize.zw, tracingGBData.ClipDepth, g_SceneCB.ViewProjInv);
			tracingConfidence[i] *= 1.0 - saturate(abs(dot(tracingOrigin - worldPos, gbData.Normal)) / surfaceDistTolerance);
			#endif
			tracingConfidence[i] *= saturate(dot(tracingGBData.Normal, gbData.Normal));
		}
		float debugValue = max(max(tracingConfidence[0], tracingConfidence[1]), max(tracingConfidence[2], tracingConfidence[3])) > 0.0 ? 1 : 0;
		tracingConfidence *= sampleWeight;
		float tracingConfidenceSum = dot(tracingConfidence, 1.0);
		[flatten] if (tracingConfidenceSum > 0.0)
		{
			sampleWeight = tracingConfidence * rcp(tracingConfidenceSum);
		}

		// blend light buffer results
		[unroll] for (/*uint */i = 0; i < 4; ++i)
		{
			int2 lightSamplePos = int2(clamp(lightBufferPos + QuadSampleOfs[i], float2(0, 0), g_SceneCB.LightBufferSize.xy - 1));
			#if (SHADOW_BUFFER_UINT32)
			uint shadowPerLightPacked = g_ShadowResult.Load(int3(lightSamplePos.xy, 0));
			[unroll] for (/*uint */j = 0; j < ShadowBufferEntriesPerPixel; ++j)
			{
				shadowPerLight[j] += ShadowBufferGetLightOcclusion(shadowPerLightPacked, j) * sampleWeight[i];
			}
			#else
			float4 shadowResultData = g_ShadowResult.Load(int3(lightSamplePos.xy, 0));
			[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
			{
				shadowPerLight[j] += shadowResultData[j] * sampleWeight[i];
			}
			#endif
		}
		#endif

		// material params

		MaterialInputs material = (MaterialInputs)0;
		initMaterial(material);
		material.normal = gbData.Normal;
		if (g_SceneCB.OverrideMaterial)
		{
			material.baseColor.xyz = g_SceneCB.Material.BaseColor;
			material.roughness = g_SceneCB.Material.Roughness;
			material.metallic = g_SceneCB.Material.Metallic;
			material.reflectance = g_SceneCB.Material.Reflectance;
		}
		else
		{
			// TODO: read mesh material
			material.baseColor.xyz = 0.5;
			material.roughness = 0.5;
			material.metallic = 0.0;
			material.reflectance = 0.04;
		}

		// lighting params

		LightingParams lightingParams;
		getLightingParams(worldPos, g_SceneCB.CameraPos.xyz, material, lightingParams);

		// direct light

		float3 directLightColor = 0;
		for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
		{
			LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
			float3 lightDir = (LightType_Directional == light.Type ? -light.Direction : normalize(light.Position.xyz - worldPos.xyz));
			float shadowFactor = (ilight < ShadowBufferEntriesPerPixel ? shadowPerLight[ilight] : 1.0);
			float NoL = dot(lightDir, lightingParams.normal);
			shadowFactor *= saturate(NoL  * 10.0); // approximate self shadowing at grazing angles
			float specularOcclusion = shadowFactor;
			directLightColor += EvaluateDirectLighting(lightingParams, light, shadowFactor, specularOcclusion).xyz;
		}

		// indirect light

		float3 envDir = float3(lightingParams.normal.x, max(lightingParams.normal.y, 0.0), lightingParams.normal.z);
		float3 envColor = CalculateSkyLight(worldPos, normalize(envDir * 0.5 + WorldUp)).xyz; // simplified sky light
		float3 indirectLightColor = lightingParams.diffuseColor.xyz * envColor; // no indirect spec

		// final

		lightingResult = directLightColor + indirectLightColor;

		// TEMP:
		if (g_SceneCB.DebugVec0[0] > 0)
		{
			lightingResult = lerp(float3(1,0,0), float3(0,1,0), /*debugValue*/shadowPerLight[3]) * max(indirectLightColor, 1.0);
		}
	}

	g_LightingTarget[imagePos] = float4(lightingResult, 1.0);
}

// compute shadow mips

static const uint g_ShadowMipGroupSize = 16;
groupshared float4 g_ShadowMipGroupData[g_ShadowMipGroupSize * g_ShadowMipGroupSize];

[shader("compute")]
[numthreads(g_ShadowMipGroupSize, g_ShadowMipGroupSize, 1)]
void ComputeShadowMips(const uint3 dispatchThreadId : SV_DispatchThreadID, const uint3 groupThreadId : SV_GroupThreadID, const uint groupThreadIdx : SV_GroupIndex)
{
	uint2 imagePos = dispatchThreadId.xy;
	uint2 imageSize = uint2(g_SceneCB.LightBufferSize.xy);
	if (imagePos.x >= imageSize.x || imagePos.y >= imageSize.y)
		return;

	// fetch data into shared memeory

	float4 dataMip0 = g_ShadowResult.Load(int3(imagePos.xy, 0));
	g_ShadowMipGroupData[groupThreadIdx] = dataMip0;
	GroupMemoryBarrierWithGroupSync();

	// mip 1

	if (imagePos.x % 2 != 0 || imagePos.y % 2 != 0)
		return;

	uint2 dstSize = imageSize / 2;
	uint2 dstPos = imagePos.xy / 2;
	if (dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
		return;

	uint2 srcSize = imageSize;
	uint2 srcPos = imagePos.xy;
	float4 mipData = 0;
	[unroll] for (uint i = 0; i < 4; ++i)
	{
		uint2 subPos = (min(srcPos.xy + uint2(QuadSampleOfs[i].xy), srcSize.xy)) % g_ShadowMipGroupSize;
		mipData += g_ShadowMipGroupData[subPos.x + subPos.y * g_ShadowMipGroupSize];
	}
	mipData *= 0.25;
	g_ShadowMip1[dstPos] = mipData;
	g_ShadowMipGroupData[groupThreadIdx] = mipData;
	GroupMemoryBarrierWithGroupSync();

	// mip 2

	if (imagePos.x % 4 != 0 || imagePos.y % 4 != 0)
		return;

	dstSize = imageSize / 4;
	dstPos = imagePos.xy / 4;
	if (dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
		return;

	srcSize = imageSize / 2;
	srcPos = imagePos.xy / 2;
	mipData = 0;
	[unroll] for (i = 0; i < 4; ++i)
	{
		uint2 subPos = (min(srcPos.xy + uint2(QuadSampleOfs[i].xy), srcSize.xy) * 2) % g_ShadowMipGroupSize;
		mipData += g_ShadowMipGroupData[subPos.x + subPos.y * g_ShadowMipGroupSize];
	}
	mipData *= 0.25;
	g_ShadowMip2[dstPos] = mipData;
	g_ShadowMipGroupData[groupThreadIdx] = mipData;
	GroupMemoryBarrierWithGroupSync();

	// mip 3

	if (imagePos.x % 8 != 0 || imagePos.y % 8 != 0)
		return;

	dstSize = imageSize / 8;
	dstPos = imagePos.xy / 8;
	if (dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
		return;

	srcSize = imageSize / 4;
	srcPos = imagePos.xy / 4;
	mipData = 0;
	[unroll] for (i = 0; i < 4; ++i)
	{
		uint2 subPos = (min(srcPos.xy + uint2(QuadSampleOfs[i].xy), srcSize.xy) * 4) % g_ShadowMipGroupSize;
		mipData += g_ShadowMipGroupData[subPos.x + subPos.y * g_ShadowMipGroupSize];
	}
	mipData *= 0.25;
	g_ShadowMip3[dstPos] = mipData;
	g_ShadowMipGroupData[groupThreadIdx] = mipData;
	GroupMemoryBarrierWithGroupSync();

	// mip 4

	if (imagePos.x % 16 != 0 || imagePos.y % 16 != 0)
		return;

	dstSize = imageSize / 16;
	dstPos = imagePos.xy / 16;
	if (dstPos.x >= dstSize.x || dstPos.y >= dstSize.y)
		return;

	srcSize = imageSize / 8;
	srcPos = imagePos.xy / 8;
	mipData = 0;
	[unroll] for (i = 0; i < 4; ++i)
	{
		uint2 subPos = (min(srcPos.xy + uint2(QuadSampleOfs[i].xy), srcSize.xy) * 8) % g_ShadowMipGroupSize;
		mipData += g_ShadowMipGroupData[subPos.x + subPos.y * g_ShadowMipGroupSize];
	}
	mipData *= 0.25;
	g_ShadowMip4[dstPos] = mipData;
	g_ShadowMipGroupData[groupThreadIdx] = mipData;
	GroupMemoryBarrierWithGroupSync();
}

// shadow result blur filter

#if (0)
// kernel size = 3
// sigma = 1.0
static const uint BlurKernelSize = 3;
static const float BlurKernelWeight[BlurKernelSize * BlurKernelSize] = {
	0.077847,	0.123317,	0.077847,
	0.123317,	0.195346,	0.123317,
	0.077847,	0.123317,	0.077847,
};
#elif (1)
// kernel size = 5
static const uint BlurKernelSize = 5;
#if (0)
// sigma = 1.0
static const float BlurKernelWeight[BlurKernelSize * BlurKernelSize] = {
	0.003765,	0.015019,	0.023792,	0.015019,	0.003765,
	0.015019,	0.059912,	0.094907,	0.059912,	0.015019,
	0.023792,	0.094907,	0.150342,	0.094907,	0.023792,
	0.015019,	0.059912,	0.094907,	0.059912,	0.015019,
	0.003765,	0.015019,	0.023792,	0.015019,	0.003765,
};
#else
// sigma = 2.0
static const float BlurKernelWeight[BlurKernelSize * BlurKernelSize] = {
	0.023528,	0.033969,	0.038393,	0.033969,	0.023528,
	0.033969,	0.049045,	0.055432,	0.049045,	0.033969,
	0.038393,	0.055432,	0.062651,	0.055432,	0.038393,
	0.033969,	0.049045,	0.055432,	0.049045,	0.033969,
	0.023528,	0.033969,	0.038393,	0.033969,	0.023528,
};
#endif
// sigma = 0.5
static const float BlurKernelWeightLow[BlurKernelSize * BlurKernelSize] = {
	0.000002,	0.000212,	0.000922,	0.000212,	0.000002,
	0.000212,	0.024745,	0.107391,	0.024745,	0.000212,
	0.000922,	0.107391,	0.466066,	0.107391,	0.000922,
	0.000212,	0.024745,	0.107391,	0.024745,	0.000212,
	0.000002,	0.000212,	0.000922,	0.000212,	0.000002,
};
#elif (0)
// kernel size = 7
// sigma = 2.0
static const uint BlurKernelSize = 7;
static const float BlurKernelWeight[BlurKernelSize * BlurKernelSize] = {
	0.005084,	0.009377,	0.013539,	0.015302,	0.013539,	0.009377,	0.005084,
	0.009377,	0.017296,	0.024972,	0.028224,	0.024972,	0.017296,	0.009377,
	0.013539,	0.024972,	0.036054,	0.040749,	0.036054,	0.024972,	0.013539,
	0.015302,	0.028224,	0.040749,	0.046056,	0.040749,	0.028224,	0.015302,
	0.013539,	0.024972,	0.036054,	0.040749,	0.036054,	0.024972,	0.013539,
	0.009377,	0.017296,	0.024972,	0.028224,	0.024972,	0.017296,	0.009377,
	0.005084,	0.009377,	0.013539,	0.015302,	0.013539,	0.009377,	0.005084,
};
#endif

static const uint g_ShadowBlurGroupSize = 8;
static const uint g_ShadowBlurGroupBorder = BlurKernelSize / 2;
static const uint g_ShadowBlurGroupSizeWithBorders = g_ShadowBlurGroupSize + g_ShadowBlurGroupBorder * 2;
groupshared float4 g_ShadowBlurGroupData[g_ShadowBlurGroupSizeWithBorders * g_ShadowBlurGroupSizeWithBorders];
groupshared float g_ShadowBlurGroupDepth[g_ShadowBlurGroupSizeWithBorders * g_ShadowBlurGroupSizeWithBorders];
#define SHADOW_BLUR_PREFETCH 1 // -40% time
#define SHADOW_BLUR_PREFETCH_DEPTH 1 // additional -20% time (-50% total)
#define SHADOW_BLUR_COMPUTE_VARIANCE 0

[shader("compute")]
[numthreads(g_ShadowBlurGroupSize, g_ShadowBlurGroupSize, 1)]
void BlurShadowResult(const uint3 dispatchThreadId : SV_DispatchThreadID, const uint3 groupId : SV_GroupID, const uint3 groupThreadId : SV_GroupThreadID, const uint groupThreadIdx : SV_GroupIndex)
{
	uint2 lightBufferPos = dispatchThreadId.xy;
	if (lightBufferPos.x >= (uint)g_SceneCB.LightBufferSize.x || lightBufferPos.y >= (uint)g_SceneCB.LightBufferSize.y)
		return;

	uint2 subSamplePos = 0; // sample sub pos from tracing info ommited here because currently always 0

	// prefetch shadow data in shared memory
	#if (SHADOW_BLUR_PREFETCH)
	int2 groupFrom = max(int2(groupId.xy) * g_ShadowBlurGroupSize.xx - g_ShadowBlurGroupBorder.xx, int2(0, 0));
	int2 groupTo = min(int2(groupId.xy + 1) * g_ShadowBlurGroupSize.xx - 1 + g_ShadowBlurGroupBorder.xx, int2(g_SceneCB.LightBufferSize.xy - 1));
	int2 groupSize = (groupTo - groupFrom + 1);
	int2 prefetchFrom = int2(lightBufferPos.xy) - int2(groupThreadId.x == 0, groupThreadId.y == 0) * g_ShadowBlurGroupBorder;
	int2 prefetchTo = int2(lightBufferPos.xy) + int2(groupThreadId.x + 1 == g_ShadowBlurGroupSize, groupThreadId.y + 1 == g_ShadowBlurGroupSize) * g_ShadowBlurGroupBorder;
	prefetchFrom = clamp(prefetchFrom, int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
	prefetchTo = clamp(prefetchTo, int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
	for (int py = prefetchFrom.y; py <= prefetchTo.y; ++py)
	{
		for (int px = prefetchFrom.x; px <= prefetchTo.x; ++px)
		{
			int groupDataOfs = (px - groupFrom.x) + (py - groupFrom.y) * groupSize.x;
			g_ShadowBlurGroupData[groupDataOfs] = g_ShadowResult[int2(px, py)];
			#if (SHADOW_BLUR_PREFETCH_DEPTH)
			g_ShadowBlurGroupDepth[groupDataOfs] = g_GeometryDepth[int2(px, py) * g_SceneCB.LightBufferDownscale.x + subSamplePos];
			#endif
		}
	}
	GroupMemoryBarrierWithGroupSync();
	#endif

	uint2 gbufferPos = lightBufferPos * g_SceneCB.LightBufferDownscale.x + subSamplePos;
	#if (SHADOW_BLUR_PREFETCH_DEPTH)
	GBufferData gbData = (GBufferData)0;
	gbData.ClipDepth = g_ShadowBlurGroupDepth[(lightBufferPos.x - groupFrom.x) + (lightBufferPos.y - groupFrom.y) * groupSize.x];
	#else
	GBufferData gbData = LoadGBufferData(gbufferPos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	#endif
	bool isSky = (gbData.ClipDepth >= 1.0);
	if (isSky)
		return;

	int2 blurStartPos = int2(lightBufferPos.xy) - int(BlurKernelSize / 2);

	#if (SHADOW_BLUR_COMPUTE_VARIANCE)
	float4 shadowAverage = 0.0;
	for (py = 0; py < BlurKernelSize; ++py)
	{
		for (int px = 0; px < BlurKernelSize; ++px)
		{
			int2 blurSamplePos = clamp(blurStartPos.xy + int2(px, py), int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
			int groupDataOfs = (blurSamplePos.x - groupFrom.x) + (blurSamplePos.y - groupFrom.y) * groupSize.x;
			shadowAverage += g_ShadowBlurGroupData[groupDataOfs];
		}
	}
	shadowAverage /= BlurKernelSize * BlurKernelSize;
	float4 shadow = g_ShadowBlurGroupData[(lightBufferPos.x - groupFrom.x) + (lightBufferPos.y - groupFrom.y) * groupSize.x];
	float4 variance = (shadow - shadowAverage);
	variance = saturate(variance * variance * g_SceneCB.DebugVec0[3]);
	float varianceMax = max(max(variance.x, variance.y), variance.z);
	#endif

	//float blurDepthDeltaTolerance = gbData.ClipDepth * 1.0e-4;
	float3 worldPos = ImagePosToWorldPos(int2(gbufferPos.xy), g_SceneCB.TargetSize.zw, gbData.ClipDepth, g_SceneCB.ViewProjInv);
	float3 worldToEyeDir = normalize(g_SceneCB.CameraPos.xyz - worldPos.xyz);
	float viewDepth = ClipDepthToViewDepth(gbData.ClipDepth, g_SceneCB.Proj);
	float blurDepthDeltaTolerance = viewDepth * lerp(0.02, 0.008, dot(gbData.Normal.xyz, worldToEyeDir));
	float4 shadowBlured = 0.0;
	float blurWeightSum = 0.0;
	for (int iy = 0; iy < BlurKernelSize; ++iy)
	{
		for (int ix = 0; ix < BlurKernelSize; ++ix)
		{
			int2 blurSamplePos = clamp(blurStartPos.xy + int2(ix, iy), int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
			int blurKernelPos = ix + iy * BlurKernelSize;
			float blurSampleWeight = BlurKernelWeight[blurKernelPos];
			#if (SHADOW_BLUR_COMPUTE_VARIANCE)
			blurSampleWeight = lerp(BlurKernelWeightLow[blurKernelPos], blurSampleWeight, varianceMax);
			#endif
			int2 blurSampleGBPos = blurSamplePos * g_SceneCB.LightBufferDownscale.x + subSamplePos;
			
			// gbuffer based discontinuities rejection
			#if (SHADOW_BLUR_PREFETCH_DEPTH)
			GBufferData blurSampleGBData = (GBufferData)0;
			blurSampleGBData.ClipDepth = g_ShadowBlurGroupDepth[(blurSamplePos.x - groupFrom.x) + (blurSamplePos.y - groupFrom.y) * groupSize.x];
			#else
			GBufferData blurSampleGBData = LoadGBufferData(blurSampleGBPos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
			#endif
			//blurSampleWeight *= 1.0 - saturate(abs(gbData.ClipDepth - blurSampleGBData.ClipDepth) / blurDepthDeltaTolerance);
			float blurSampleViewDepth = ClipDepthToViewDepth(blurSampleGBData.ClipDepth, g_SceneCB.Proj);
			blurSampleWeight *= 1.0 - saturate(abs(viewDepth - blurSampleViewDepth) / blurDepthDeltaTolerance);

			#if (SHADOW_BLUR_PREFETCH)
			int groupDataOfs = (blurSamplePos.x - groupFrom.x) + (blurSamplePos.y - groupFrom.y) * groupSize.x;
			shadowBlured += g_ShadowBlurGroupData[groupDataOfs] * blurSampleWeight;
			#else
			shadowBlured += g_ShadowResult[blurSamplePos.xy] * blurSampleWeight;
			#endif
			blurWeightSum += blurSampleWeight;
		}
	}
	shadowBlured /= max(blurWeightSum, 1.0e-5);
	#if (SHADOW_BLUR_COMPUTE_VARIANCE)
	shadowBlured[3] = varianceMax;
	#endif
	g_ShadowTarget[lightBufferPos.xy] = shadowBlured;
}

// shadow result combined denoising filter

[shader("compute")]
[numthreads(8, 8, 1)]
void AccumulateShadowResult(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 lightBufferPos = dispatchThreadId.xy;
	if (lightBufferPos.x >= (uint)g_SceneCB.LightBufferSize.x || lightBufferPos.y >= (uint)g_SceneCB.LightBufferSize.y)
		return;

	// tracing info (sub sample pos & counter)

	uint2 tracingInfo = g_TracingInfoTarget[lightBufferPos.xy];
	uint tracingSampleId = tracingInfo.x;
	uint2 tracingSamplePos = uint2(tracingSampleId % (uint)g_SceneCB.LightBufferDownscale.x, tracingSampleId / (uint)g_SceneCB.LightBufferDownscale.x);

	// read geometry buffer

	uint2 imagePos = lightBufferPos * g_SceneCB.LightBufferDownscale.x + tracingSamplePos;
	GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	bool isSky = (gbData.ClipDepth >= 1.0);
	if (isSky)
		return;

	// read lighting result

	float4 shadowPerLight = 0.0;
	#if (SHADOW_BUFFER_UINT32)
	uint shadowPerLightPacked = g_ShadowTarget[lightBufferPos.xy];
	[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
	{
		shadowPerLight[j] = ShadowBufferGetLightOcclusion(shadowPerLightPacked, j);
	}
	#else
	shadowPerLight = g_ShadowTarget[lightBufferPos.xy];
	#endif

	// apply filter

	float2 lightBufferUV = (float2(lightBufferPos.xy) + 0.5) * g_SceneCB.LightBufferSize.zw;
	float2 uvPos = (float2(imagePos) + 0.5) * g_SceneCB.TargetSize.zw;
	float3 clipPos = float3(float2(uvPos.x, 1.0 - uvPos.y) * 2.0 - 1.0, gbData.ClipDepth);
	float3 worldPos = ClipPosToWorldPos(clipPos, g_SceneCB.ViewProjInv);
	float4 clipPosPrev = mul(float4(worldPos, 1.0), g_SceneCB.ViewProjPrev);
	clipPosPrev.xy /= clipPosPrev.w;
	if (g_SceneCB.AccumulationFrameCount > 0 && all(abs(clipPosPrev.xy) < 1.0))
	{
		float2 imagePosPrev = clamp((clipPosPrev.xy * float2(1.0, -1.0) + 1.0) * 0.5 * g_SceneCB.TargetSize.xy, float2(0, 0), g_SceneCB.TargetSize.xy - 1);
		uint2 dispatchPosPrev = clamp(floor(imagePosPrev * g_SceneCB.LightBufferDownscale.y), float2(0, 0), g_SceneCB.LightBufferSize.xy - 1);
		imagePosPrev = dispatchPosPrev * g_SceneCB.LightBufferDownscale.x;

		float clipDepthPrev = g_DepthHistory.Load(int3(imagePosPrev.xy, 0));
		bool isSkyPrev = (clipDepthPrev >= 1.0);
		// TODO
		#if (0)
		float viewDepth = ClipDepthToViewDepth(gbData.ClipDepth, g_SceneCB.Proj);
		float viewDepthPrev = ClipDepthToViewDepth(clipDepthPrev, g_SceneCB.ProjPrev);
		float depthDeltaTolerance = viewDepth * /*0.01*/max(1.0e-6, g_SceneCB.DebugVec0[2]);
		float surfaceHistoryWeight = 1.0 - saturate(abs(viewDepth - viewDepthPrev) / depthDeltaTolerance);
		#elif (0)
		// history rejection from Ray Tracing Gems "Ray traced Shadows"
		float3 normalVS = mul(float4(gbData.Normal, 0.0), g_SceneCB.View).xyz;
		//float depthDeltaTolerance = 0.003 + 0.017 * abs(normalVS.z);
		float depthDeltaTolerance = g_SceneCB.DebugVec0[1] + g_SceneCB.DebugVec0[2] * abs(normalVS.z);
		float surfaceHistoryWeight = (abs(1.0 - clipDepthPrev / gbData.ClipDepth) < depthDeltaTolerance ? 1.0 : 0.0);
		#else
		float3 worldPosPrev = ClipPosToWorldPos(float3(clipPosPrev.xy, clipDepthPrev), g_SceneCB.ViewProjInvPrev);
		float viewDepth = ClipDepthToViewDepth(gbData.ClipDepth, g_SceneCB.Proj);
		float viewDepthPrev = ClipDepthToViewDepth(clipDepthPrev, g_SceneCB.ProjPrev);
		float worldPosTolerance = max(viewDepth, viewDepthPrev) * /*3.0e-3*/max(1.0e-6, g_SceneCB.DebugVec0[2]);
		float surfaceHistoryWeight = 1.0 - saturate(length(worldPos - worldPosPrev) / worldPosTolerance);
		surfaceHistoryWeight = saturate((surfaceHistoryWeight - g_SceneCB.DebugVec0[1]) / (1.0 - g_SceneCB.DebugVec0[1]));
		//shadowPerLight[3] = surfaceHistoryWeight; // TEMP: for debug output
		#endif

		uint counter = g_TracingHistory.Load(int3(dispatchPosPrev.xy, 0)).y;
		counter = (uint)floor(float(counter) * surfaceHistoryWeight + 0.5);
		float historyWeight = float(counter) / (counter + 1);

		#if (SHADOW_BUFFER_UINT32)
		// not supported
		#elif (0)
		// fallback to higher mip at low history counter
		float shadowMip = g_SceneCB.DebugVec0[3] * pow(1.0 - float(counter) / g_SceneCB.AccumulationFrameCount, 1.0);
		float4 shadowMipData = g_ShadowMips.SampleLevel(g_SamplerTrilinear, lightBufferUV, shadowMip - 1);
		shadowPerLight = (shadowMip < 1 ? lerp(shadowPerLight, shadowMipData, shadowMip) : shadowMipData);
		#endif

		#if (SHADOW_BUFFER_UINT32)
		uint shadowHistoryPacked = g_ShadowHistory.Load(int3(dispatchPosPrev.xy, 0));
		#else
		float4 shadowHistoryData = g_ShadowHistory.Load(int3(dispatchPosPrev.xy, 0));
		#endif
		[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
		{
			#if (SHADOW_BUFFER_UINT32)
			uint shadowPackedPrev = ((shadowHistoryPacked >> (j * ShadowBufferBitsPerEntry)) & ShadowBufferEntryMask);
			float shadowAccumulated = ShadowBufferEntryUnpack(shadowPackedPrev);
			shadowAccumulated = lerp(shadowPerLight[j], shadowAccumulated, historyWeight);
			#if (1)
			// note: following code guarantees minimal difference (if any) is applied to quantized result to overcome lack of precision at the end of the accumulation curve
			const float diffEps = 1.0e-4;
			float diff = shadowPerLight[j] - shadowAccumulated; // calculate difference before quantizing
			diff = (diff > diffEps ? 1.0f : (diff < -diffEps ? -1.0f : 0.0f));
			shadowAccumulated = (float)ShadowBufferEntryPack(shadowAccumulated); // quantize
			shadowAccumulated = shadowPackedPrev + max(abs(shadowAccumulated - shadowPackedPrev), abs(diff)) * diff;
			shadowPerLight[j] = ShadowBufferEntryUnpack(shadowAccumulated);
			#else
			shadowPerLight[j] = shadowAccumulated;
			#endif
			#else

			#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
			shadowPerLight[j] = lerp(shadowPerLight[j], shadowHistoryData[j], (ilight == j ? historyWeight : 1.0));
			#else
			shadowPerLight[j] = lerp(shadowPerLight[j], shadowHistoryData[j], historyWeight);
			#endif
			#endif
		}

		#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
		if (ilight == lightCount - 1)
		#endif
		counter = clamp(counter + 1, 0, g_SceneCB.AccumulationFrameCount);
		tracingInfo[1] = counter;
	}
	else
	{
		// no history available, fallback to higher mip
		#if (SHADOW_BUFFER_UINT32)
		// not supported
		#else
		//shadowPerLight = g_ShadowMips.SampleLevel(g_SamplerTrilinear, lightBufferUV, 3);
		#endif
	}

	#if (SHADOW_BUFFER_UINT32)
	// pack result
	shadowPerLightPacked = 0x0;
	[unroll] for (/*uint */j = 0; j < ShadowBufferEntriesPerPixel; ++j)
	{
		shadowPerLightPacked |= ShadowBufferEntryPack(shadowPerLight[j]) << (j * ShadowBufferBitsPerEntry);
	}
	#endif

	// write result
	#if (SHADOW_BUFFER_UINT32)
	g_ShadowTarget[lightBufferPos.xy] = shadowPerLightPacked;
	#else
	g_ShadowTarget[lightBufferPos.xy] = shadowPerLight;
	#endif
	g_TracingInfoTarget[lightBufferPos.xy] = tracingInfo;
}