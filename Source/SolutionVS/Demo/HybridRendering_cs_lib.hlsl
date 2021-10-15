
#include "HybridRendering.hlsli"

// sky precompute

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeSky(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 imagePos = dispatchThreadId.xy;
	if (imagePos.x >= (uint)g_SceneCB.PrecomputedSkySize.x || imagePos.y >= (uint)g_SceneCB.PrecomputedSkySize.y)
		return;

	float3 groundPos = 0.0;
	float3 direction = SkyImagePosToDirection(g_SceneCB, imagePos);
	float4 skyLight = CalculateSkyLight(g_SceneCB, groundPos, direction);

	g_PrecomputedSkyTarget[imagePos] = skyLight;
}

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
		lightingResult = GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerBilinearWrap, g_SceneCB.CameraPos.xyz, worldRay).xyz;
	}
	else
	{
		// read geometry buffer

		GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);

		// read lighting buffer

		float4 shadowPerLight = 0.0;
		float3 indirectLight = 0.0;
		float2 lightBufferPos = (float2(imagePos.xy) + 0.5) * g_SceneCB.LightBufferDownscale.y;

		#if (0)
		
		// point upsampling
		shadowPerLight = g_ShadowResult.Load(int3(lightBufferPos.xy, 0));
		indirectLight = g_IndirectLight.Load(int3(lightBufferPos.xy, 0)).xyz;
		
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
			float4 shadowResultData = g_ShadowResult.Load(int3(lightSamplePos.xy, 0));
			[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
			{
				shadowPerLight[j] += shadowResultData[j] * sampleWeight[i];
			}
			indirectLight.xyz += g_IndirectLight.Load(int3(lightSamplePos.xy, 0)).xyz * sampleWeight[i];
		}
		//float debugValue = shadowPerLight[3];
		#endif

		// direct light

		LightingParams lightingParams = GetMaterialLightingParams(g_SceneCB, gbData, worldPos);

		float3 directLight = 0;
		for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
		{
			LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
			float3 lightDir = (LightType_Directional == light.Type ? -light.Direction : normalize(light.Position.xyz - worldPos.xyz));
			float shadowFactor = (ilight < ShadowBufferEntriesPerPixel ? shadowPerLight[ilight] : 1.0);
			float NoL = dot(lightDir, lightingParams.normal);
			shadowFactor *= saturate(NoL  * 10.0); // approximate self shadowing at grazing angles
			float specularOcclusion = shadowFactor;
			directLight += EvaluateDirectLighting(lightingParams, light, shadowFactor, specularOcclusion).xyz;
		}

		// indirect light

		indirectLight = indirectLight * lightingParams.diffuseColor.xyz;

		// final

		lightingResult = directLight * g_SceneCB.DirectLightFactor + indirectLight * g_SceneCB.IndirectLightFactor;

		// TEMP:
		if (g_SceneCB.DebugVec0[0] > 0)
		{
			lightingResult = lerp(float3(1, 0, 0), float3(0, 1, 0), debugValue);
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

// blur filter

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

static const uint g_BlurGroupSize = 8;
static const uint g_BlurGroupBorder = BlurKernelSize / 2;
static const uint g_BlurGroupSizeWithBorders = g_BlurGroupSize + g_BlurGroupBorder * 2;
groupshared float4 g_BlurGroupData[g_BlurGroupSizeWithBorders * g_BlurGroupSizeWithBorders];
groupshared float g_BlurGroupDepth[g_BlurGroupSizeWithBorders * g_BlurGroupSizeWithBorders];
groupshared float3 g_BlurGroupNormal[g_BlurGroupSizeWithBorders * g_BlurGroupSizeWithBorders];
#define BLUR_PREFETCH 1 // -40% time
#define BLUR_PREFETCH_GBDATA 1 // additional -20% time (-50% total)
#define BLUR_COMPUTE_VARIANCE 0

void swap(inout float a, inout float b)
{
	float c = a;
	a = b;
	b = c;
}

void swap(inout float4 a, inout float4 b)
{
	float4 c = a;
	a = b;
	b = c;
}

[shader("compute")]
[numthreads(g_BlurGroupSize, g_BlurGroupSize, 1)]
void BlurLightingResult(const uint3 dispatchThreadId : SV_DispatchThreadID, const uint3 groupId : SV_GroupID, const uint3 groupThreadId : SV_GroupThreadID, const uint groupThreadIdx : SV_GroupIndex)
{
	uint2 bufferPos = dispatchThreadId.xy;
	if (bufferPos.x >= (uint)g_SceneCB.LightBufferSize.x || bufferPos.y >= (uint)g_SceneCB.LightBufferSize.y)
		return;

	uint2 subSamplePos = 0; // sample sub pos from tracing info ommited here because currently always 0

	// prefetch data in shared memory
	#if (BLUR_PREFETCH)
	int2 groupFrom = max(int2(groupId.xy) * g_BlurGroupSize.xx - g_BlurGroupBorder.xx, int2(0, 0));
	int2 groupTo = min(int2(groupId.xy + 1) * g_BlurGroupSize.xx - 1 + g_BlurGroupBorder.xx, int2(g_SceneCB.LightBufferSize.xy - 1));
	int2 groupSize = (groupTo - groupFrom + 1);
	int2 prefetchFrom = int2(bufferPos.xy) - int2(groupThreadId.x == 0, groupThreadId.y == 0) * g_BlurGroupBorder;
	int2 prefetchTo = int2(bufferPos.xy) + int2(groupThreadId.x + 1 == g_BlurGroupSize, groupThreadId.y + 1 == g_BlurGroupSize) * g_BlurGroupBorder;
	prefetchFrom = clamp(prefetchFrom, int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
	prefetchTo = clamp(prefetchTo, int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
	for (int py = prefetchFrom.y; py <= prefetchTo.y; ++py)
	{
		for (int px = prefetchFrom.x; px <= prefetchTo.x; ++px)
		{
			int groupDataOfs = (px - groupFrom.x) + (py - groupFrom.y) * groupSize.x;
			g_BlurGroupData[groupDataOfs] = g_BlurSource[int2(px, py)];
			#if (BLUR_PREFETCH_GBDATA)
			g_BlurGroupDepth[groupDataOfs] = g_GeometryDepth[int2(px, py) * g_SceneCB.LightBufferDownscale.x + subSamplePos];
			g_BlurGroupNormal[groupDataOfs] = g_GeometryImage1[int2(px, py) * g_SceneCB.LightBufferDownscale.x + subSamplePos].xyz;
			#endif
		}
	}
	GroupMemoryBarrierWithGroupSync();
	#endif

	uint2 gbufferPos = bufferPos * g_SceneCB.LightBufferDownscale.x + subSamplePos;
	#if (BLUR_PREFETCH_GBDATA)
	uint groupDataPos = (bufferPos.x - groupFrom.x) + (bufferPos.y - groupFrom.y) * groupSize.x;
	float4 sourceData = g_BlurGroupData[groupDataPos];
	GBufferData gbData = (GBufferData)0;
	gbData.ClipDepth = g_BlurGroupDepth[groupDataPos];
	gbData.Normal = g_BlurGroupNormal[groupDataPos];
	#else
	float4 sourceData = g_BlurSource[bufferPos];
	GBufferData gbData = LoadGBufferData(gbufferPos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	#endif
	bool isSky = (gbData.ClipDepth >= 1.0);
	if (isSky)
		return;

	int2 blurStartPos = int2(bufferPos.xy) - int(BlurKernelSize / 2);

	#if (BLUR_COMPUTE_VARIANCE)
	float4 averageValue = 0.0;
	for (py = 0; py < BlurKernelSize; ++py)
	{
		for (int px = 0; px < BlurKernelSize; ++px)
		{
			int2 blurSamplePos = clamp(blurStartPos.xy + int2(px, py), int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
			int groupDataOfs = (blurSamplePos.x - groupFrom.x) + (blurSamplePos.y - groupFrom.y) * groupSize.x;
			averageValue += g_BlurGroupData[groupDataOfs];
		}
	}
	averageValue /= BlurKernelSize * BlurKernelSize;
	float4 crntValue = g_BlurGroupData[(bufferPos.x - groupFrom.x) + (bufferPos.y - groupFrom.y) * groupSize.x];
	float4 variance = (crntValue - averageValue);
	variance = saturate(variance * variance * g_SceneCB.DebugVec0[3]);
	float varianceMax = max(max(variance.x, variance.y), variance.z);
	#endif

	//float blurDepthDeltaTolerance = gbData.ClipDepth * 1.0e-4;
	float3 worldPos = ImagePosToWorldPos(int2(gbufferPos.xy), g_SceneCB.TargetSize.zw, gbData.ClipDepth, g_SceneCB.ViewProjInv);
	float3 worldToEyeDir = normalize(g_SceneCB.CameraPos.xyz - worldPos.xyz);
	float viewDepth = ClipDepthToViewDepth(gbData.ClipDepth, g_SceneCB.Proj);
	float blurDepthDeltaTolerance = viewDepth * lerp(0.02, 0.008, dot(gbData.Normal.xyz, worldToEyeDir));
	float blurWeightSum = 0.0;
	float4 bluredResult = 0.0;
	#if (BLUR_PREFETCH)
	// note: kernel size scaling is not supported in prefetch mode
	int kernelSizeFactor = 1;
	#else
	int kernelSizeFactor = clamp(g_BlurPassCB.KernelSizeFactor, 1, 16);
	#endif
	for (int iy = 0; iy < BlurKernelSize; ++iy)
	{
		for (int ix = 0; ix < BlurKernelSize; ++ix)
		{
			#if (BLUR_PREFETCH)
			int2 blurSamplePos = clamp(blurStartPos.xy + int2(ix, iy), int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
			#else
			// with kernelSizeFactor
			int2 blurSamplePos = clamp(int2(bufferPos.xy) + (int2(ix, iy) - int(BlurKernelSize / 2)) * kernelSizeFactor, int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
			#endif
			int blurKernelPos = ix + iy * BlurKernelSize;
			float blurSampleWeight = BlurKernelWeight[blurKernelPos];
			#if (BLUR_COMPUTE_VARIANCE)
			blurSampleWeight = lerp(BlurKernelWeightLow[blurKernelPos], blurSampleWeight, varianceMax);
			#endif
			int2 blurSampleGBPos = clamp(blurSamplePos * g_SceneCB.LightBufferDownscale.x + subSamplePos, int2(0, 0), int2(g_SceneCB.TargetSize.xy - 1));

			// gbuffer based discontinuities rejection
			#if (BLUR_PREFETCH_GBDATA)
			groupDataPos = (blurSamplePos.x - groupFrom.x) + (blurSamplePos.y - groupFrom.y) * groupSize.x;
			float4 blurSampleData = g_BlurGroupData[groupDataPos];
			GBufferData blurSampleGBData = (GBufferData)0;
			blurSampleGBData.ClipDepth = g_BlurGroupDepth[groupDataPos];
			blurSampleGBData.Normal = g_BlurGroupNormal[groupDataPos];
			#else
			float4 blurSampleData = g_BlurSource[blurSamplePos];
			GBufferData blurSampleGBData = LoadGBufferData(blurSampleGBPos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
			#endif
			//blurSampleWeight *= 1.0 - saturate(abs(gbData.ClipDepth - blurSampleGBData.ClipDepth) / blurDepthDeltaTolerance);
			float blurSampleViewDepth = ClipDepthToViewDepth(blurSampleGBData.ClipDepth, g_SceneCB.Proj);
			#if (0)
			blurSampleWeight *= 1.0 - saturate(abs(viewDepth - blurSampleViewDepth) / blurDepthDeltaTolerance);
			blurSampleWeight *= pow(saturate(dot(blurSampleGBData.Normal, gbData.Normal)), g_SceneCB.DebugVec1[3]);
			#else
			float depthDist = (viewDepth - blurSampleViewDepth);
			float depthDist2 = depthDist * depthDist;
			blurSampleWeight *= saturate(exp(-depthDist2 / g_SceneCB.DebugVec2[0]));
			float3 normalDist = blurSampleGBData.Normal - gbData.Normal;
			float normalDist2 = dot(normalDist, normalDist);
			blurSampleWeight *= saturate(exp(-normalDist2 / g_SceneCB.DebugVec2[1]));
			//float3 colorDist = sourceData.xyz - blurSampleData.xyz;
			//float colorDist2 = dot(colorDist, colorDist);
			//blurSampleWeight *= saturate(exp(-colorDist2 / g_SceneCB.DebugVec2[2]));
			#endif

			#if (BLUR_PREFETCH)
			int groupDataOfs = (blurSamplePos.x - groupFrom.x) + (blurSamplePos.y - groupFrom.y) * groupSize.x;
			bluredResult += g_BlurGroupData[groupDataOfs] * blurSampleWeight;
			#else
			bluredResult += g_BlurSource[blurSamplePos.xy] * blurSampleWeight;
			#endif
			blurWeightSum += blurSampleWeight;
		}
	}
	bluredResult /= max(blurWeightSum, 1.0e-5);
	#if (BLUR_COMPUTE_VARIANCE)
	bluredResult[3] = varianceMax; // TEMP: used as debug output
	#endif

	#if (0)
	const int filterOfs[3] = { -1, 0, 1 };
	float4 values[9];
	float valuesLum[9];
	for (iy = 0; iy < 3; ++iy)
	{
		for (int ix = 0; ix < 3; ++ix)
		{
			int2 filterPos = clamp(bufferPos.xy + int2(filterOfs[ix], filterOfs[iy]), int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
			int groupDataOfs = (filterPos.x - groupFrom.x) + (filterPos.y - groupFrom.y) * groupSize.x;
			values[ix + iy * 3] = g_BlurGroupData[groupDataOfs];
			valuesLum[ix + iy * 3] = ComputeLuminance(values[ix + iy * 3].xyz);
		}
	}
	float t;
	for (int i = 0; i < 9; ++i)
	{
		for (int j = i + 1; j < 9; ++j)
		{
			#if (1)
			if (values[i].x > values[j].x) swap(values[i].x, values[j].x);
			if (values[i].y > values[j].y) swap(values[i].y, values[j].y);
			if (values[i].z > values[j].z) swap(values[i].z, values[j].z);
			if (values[i].w > values[j].w) swap(values[i].w, values[j].w);
			#else
			if (valuesLum[i] > valuesLum[j]) swap(values[i], values[j]);
			#endif
		}
	}
	float4 medianValue = values[4];
	bluredResult = medianValue;
	#endif

	g_BlurTarget[bufferPos.xy] = bluredResult;
}

// lighting result temporal accumulation filter

#define ACCUMULATE_NEIGHBOURHOOD_CLIP 0
static const uint AccumulateNeighbourhoodSize = 9;
static const float2 AccumulateNeighbourhoodOfs[AccumulateNeighbourhoodSize] = {
	float2( 0, 0), float2(-1,-1), float2( 0,-1),
	float2( 1,-1), float2( 1, 0), float2( 1, 1),
	float2( 0, 1), float2(-1, 1), float2(-1, 0)
};

// note: clips towards aabb center + p.w
float4 ClipAABB(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
{
	float3 p_clip = 0.5 * (aabb_max + aabb_min);
	float3 e_clip = 0.5 * (aabb_max - aabb_min);
	float4 v_clip = q - float4(p_clip, p.w);
	float3 v_unit = v_clip.xyz / e_clip;
	float3 a_unit = abs(v_unit);
	float ma_unit = max3(a_unit);
	if (ma_unit > 1.0)
		return float4(p_clip, p.w) + v_clip / ma_unit;
	else
		return q;// point inside aabb
}

[shader("compute")]
[numthreads(8, 8, 1)]
void AccumulateLightingResult(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 lightBufferPos = dispatchThreadId.xy;
	if (lightBufferPos.x >= (uint)g_SceneCB.LightBufferSize.x || lightBufferPos.y >= (uint)g_SceneCB.LightBufferSize.y)
		return;

	// tracing info (sub sample pos & counter)

	uint4 tracingInfo = g_TracingInfoTarget[lightBufferPos.xy];
	uint tracingSampleId = tracingInfo[0];
	uint2 tracingSamplePos = uint2(tracingSampleId % (uint)g_SceneCB.LightBufferDownscale.x, tracingSampleId / (uint)g_SceneCB.LightBufferDownscale.x);

	// read geometry buffer

	uint2 imagePos = lightBufferPos * g_SceneCB.LightBufferDownscale.x + tracingSamplePos;
	GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	bool isSky = (gbData.ClipDepth >= 1.0);
	if (isSky)
		return;

	// read lighting result

	float4 shadowPerLight = g_ShadowTarget[lightBufferPos.xy];
	float4 indirectLight = g_IndirectLightTarget[lightBufferPos.xy];
	
	#if (ACCUMULATE_NEIGHBOURHOOD_CLIP)
	float4 indirectMu1 = 0;
	float4 indirectMu2 = 0;
	for (uint isample = 0; isample < AccumulateNeighbourhoodSize; ++isample)
	{
		int2 samplePos = clamp(lightBufferPos.xy + AccumulateNeighbourhoodOfs[isample], int2(0, 0), int2(g_SceneCB.LightBufferSize.xy - 1));
		float4 sampleValue = g_IndirectLightTarget[samplePos];
		indirectMu1 += sampleValue;
		indirectMu2 += sampleValue * sampleValue;
	}
	float4 indirectMu = indirectMu1 / AccumulateNeighbourhoodSize;
	float4 indirectSigma = sqrt(indirectMu2 / AccumulateNeighbourhoodSize - indirectMu * indirectMu);
	const float indirectGamma = g_SceneCB.DebugVec0[3];
	float4 indirectMin = indirectMu - indirectGamma * indirectSigma;
	float4 indirectMax = indirectMu + indirectGamma * indirectSigma;
	#endif

	// apply filter

	float2 lightBufferUV = (float2(lightBufferPos.xy) + 0.5) * g_SceneCB.LightBufferSize.zw;
	float2 uvPos = (float2(imagePos) + 0.5) * g_SceneCB.TargetSize.zw;
	float3 clipPos = float3(float2(uvPos.x, 1.0 - uvPos.y) * 2.0 - 1.0, gbData.ClipDepth);
	float3 worldPos = ClipPosToWorldPos(clipPos, g_SceneCB.ViewProjInv);
	float4 clipPosPrev = mul(float4(worldPos, 1.0), g_SceneCB.ViewProjPrev);
	clipPosPrev.xy /= clipPosPrev.w;
	// TODO: experimental, always reproject, even on borders, prefer ghosting over disocclusion
	//clipPosPrev.xy = clamp(clipPosPrev.xy, float2(-1.0 + g_SceneCB.TargetSize.zw * 0.5), 1.0 - float2(g_SceneCB.TargetSize.zw * 0.5));
	if (all(abs(clipPosPrev.xy) < 1.0))
	{
		float2 imagePosPrev = clamp((clipPosPrev.xy * float2(1.0, -1.0) + 1.0) * 0.5 * g_SceneCB.TargetSize.xy, float2(0, 0), g_SceneCB.TargetSize.xy - 1);
		uint2 dispatchPosPrev = clamp(floor(imagePosPrev * g_SceneCB.LightBufferDownscale.y), float2(0, 0), g_SceneCB.LightBufferSize.xy - 1);
		imagePosPrev = dispatchPosPrev * g_SceneCB.LightBufferDownscale.x;

		uint4 tracingInfoHistory = g_TracingHistory.Load(int3(dispatchPosPrev.xy, 0));
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
		float worldPosDist = length(worldPos - worldPosPrev);
		float worldPosTolerance = max(viewDepth, viewDepthPrev) * /*3.0e-3*/max(1.0e-6, g_SceneCB.DebugVec0[2]);
		float shadowHistoryConfidence = 1.0 - saturate(worldPosDist / worldPosTolerance);
		//float shadowHistoryConfidence = saturate(exp(-(worldPosDist * worldPosDist) / g_SceneCB.DebugVec0[2]));
		shadowHistoryConfidence = saturate((shadowHistoryConfidence - g_SceneCB.DebugVec0[1]) / (1.0 - g_SceneCB.DebugVec0[1]));
		//float depthDist = abs(viewDepth - viewDepthPrev);
		//float depthDist2 = depthDist * depthDist;
		//float shadowHistoryConfidence = saturate(exp(-depthDist2 / g_SceneCB.DebugVec0[1]));
		//shadowPerLight[3] = shadowHistoryConfidence; // TEMP: for debug output
		worldPosTolerance = max(viewDepth, viewDepthPrev) * /*3.0e-3*/max(1.0e-6, g_SceneCB.DebugVec1[2]);
		float indirectLightHistoryConfidence = 1.0 - saturate(worldPosDist / worldPosTolerance);
		indirectLightHistoryConfidence = saturate((indirectLightHistoryConfidence - g_SceneCB.DebugVec1[1]) / (1.0 - g_SceneCB.DebugVec1[1]));
		#endif

		uint shadowCounter = (uint)floor(float(tracingInfoHistory[1]) * shadowHistoryConfidence + 0.5);
		float shadowHistoryWeight = float(shadowCounter) / (shadowCounter + 1);
		uint indirectLightCounter = (uint)floor(float(tracingInfoHistory[2]) * indirectLightHistoryConfidence + 0.5);
		float indirectLightHistoryWeight = float(indirectLightCounter) / (indirectLightCounter + 1);

		// direct shadow

		#if (0)
		// fallback to higher mip at low history counter
		float shadowMip = g_SceneCB.DebugVec0[3] * pow(1.0 - float(shadowCounter) / g_SceneCB.ShadowAccumulationFrames, 1.0);
		float4 shadowMipData = g_ShadowMips.SampleLevel(g_SamplerTrilinear, lightBufferUV, shadowMip - 1);
		shadowPerLight = (shadowMip < 1 ? lerp(shadowPerLight, shadowMipData, shadowMip) : shadowMipData);
		#endif

		float4 shadowHistoryData = g_ShadowHistory.Load(int3(dispatchPosPrev.xy, 0));
		[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
		{
			#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
			shadowPerLight[j] = lerp(shadowPerLight[j], shadowHistoryData[j], (ilight == j ? shadowHistoryWeight : 1.0));
			#else
			shadowPerLight[j] = lerp(shadowPerLight[j], shadowHistoryData[j], shadowHistoryWeight);
			#endif
		}

		// indirect light

		float4 indirectLightHistory = g_IndirectLightHistory.Load(int3(dispatchPosPrev.xy, 0));
		#if (ACCUMULATE_NEIGHBOURHOOD_CLIP)
		indirectLightHistory = ClipAABB(indirectMin.xyz, indirectMax.xyz, float4(indirectLightHistory.xyz, 1.0), float4(indirectLightHistory.xyz, 1.0));
		//indirectLightHistory.xyz = max(indirectMin.xyz, indirectLightHistory.xyz);
		//indirectLightHistory.xyz = min(indirectMax.xyz, indirectLightHistory.xyz);
		#endif
		indirectLight = lerp(indirectLight, indirectLightHistory, indirectLightHistoryWeight);

		// update counters

		#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
		if (ilight == lightCount - 1)
		#endif
		shadowCounter = clamp(shadowCounter + 1, 0, g_SceneCB.ShadowAccumulationFrames);
		indirectLightCounter = clamp(indirectLightCounter + 1, 0, g_SceneCB.IndirectAccumulationFrames);
		tracingInfo[1] = shadowCounter;
		tracingInfo[2] = indirectLightCounter;
	}
	else
	{
		// no history available, fallback to higher mip
		//shadowPerLight = g_ShadowMips.SampleLevel(g_SamplerTrilinear, lightBufferUV, 3);
	}

	// write result
	g_ShadowTarget[lightBufferPos.xy] = shadowPerLight;
	g_TracingInfoTarget[lightBufferPos.xy] = tracingInfo;
	g_IndirectLightTarget[lightBufferPos.xy] = indirectLight;
}