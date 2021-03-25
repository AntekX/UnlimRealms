
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants>	g_SceneCB				: register(b0);
sampler							g_SamplerTrilinearWrap	: register(s0);
Texture2D<float>				g_GeometryDepth			: register(t0);
Texture2D<float4>				g_GeometryImage0		: register(t1);
Texture2D<float4>				g_GeometryImage1		: register(t2);
Texture2D<float4>				g_GeometryImage2		: register(t3);
Texture2D<float>				g_DepthHistory			: register(t4);
Texture2D<float4>				g_ShadowHistory			: register(t5);
Texture2D<uint2>				g_TracingHistory		: register(t6);
Texture2D<float4>				g_PrecomputedSky		: register(t7);
RaytracingAccelerationStructure	g_SceneStructure		: register(t8);
RWTexture2D<float4>				g_ShadowTarget			: register(u0);
RWTexture2D<uint2>				g_TracingInfoTarget		: register(u1);
RWTexture2D<float4>				g_IndirectLightTarget	: register(u2);

// common functions

float3 GetDiskSampleDirection(uint sampleId)
{
	#if (1)
	float2 dir2d = BlueNoiseDiskLUT64[sampleId % 64]; // [-1, 1]
	return float3(dir2d.xy, 1.0);
	#else
	const uint SampleCount = g_SceneCB.SamplesPerLight * max(1, g_SceneCB.AccumulationFrameCount + 1);
	float2 p2d = Hammersley(sampleId % SampleCount, SampleCount);
	//float3 dir = HemisphereSampleCosine(p2d.x, p2d.y);
	float3 dir = normalize(float3(p2d.xy * 2.0 - 1.0, 1.0));
	return dir.xyz;
	#endif
}

float3x3 ComputeLightSamplingBasis(const float3 worldPos, const LightDesc lightDesc)
{
	float3 dirToLight = -lightDesc.Direction.xyz;
	float halfAngleTangent = lightDesc.Size; // tangent of the visible disk half angle (to transform [-1,1] disk samples)
	[flatten] if (LightType_Spherical == lightDesc.Type)
	{
		dirToLight = lightDesc.Position.xyz - worldPos.xyz;
		float dist = length(dirToLight);
		dirToLight /= dist;
		halfAngleTangent = lightDesc.Size / dist;
	}
	float3x3 lightTBN;
	lightTBN[2] = dirToLight;
	lightTBN[0] = float3(1, 0, 0);
	lightTBN[1] = normalize(cross(lightTBN[2], lightTBN[0]));
	lightTBN[0] = cross(lightTBN[1], lightTBN[2]);
	lightTBN[0] *= halfAngleTangent; 
	lightTBN[1] *= halfAngleTangent;

	return lightTBN;
}

// ray generation: direct light

[shader("raygeneration")]
void RayGenDirect()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint2 dispatchSize = (uint2)g_SceneCB.LightBufferSize.xy;
	#if (SHADOW_BUFFER_UINT32)
	uint occlusionPerLightPacked = 0xffffffff;
	#else
	float4 occlusionPerLight = 1.0;
	#endif
	uint2 tracingInfo = 0; // sub sample pos & counter
	float3 indirectLight = 0.0;

	// fetch at first sub sample
	uint2 imagePos = dispatchIdx.xy * g_SceneCB.LightBufferDownscale.x;
	#if (0)
	// find best fitting sub sample
	uint dispatchDownscale = (uint)g_SceneCB.LightBufferDownscale.x;
	[branch] if (dispatchDownscale > 1)
	{
		uint2 imageSubPos = 0;
		float clipDepthCrnt = 1.0;
		for (uint iy = 0; iy < dispatchDownscale; ++iy)
		{
			for (uint ix = 0; ix < dispatchDownscale; ++ix)
			{
				float clipDepth = g_GeometryDepth.Load(int3(imagePos.x + ix, imagePos.y + iy, 0));
				[flatten] if (clipDepth < clipDepthCrnt)
				{
					clipDepthCrnt = clipDepth;
					imageSubPos = uint2(ix, iy);
				}
			}
		}
		imagePos += imageSubPos;
		tracingInfo[0] = imageSubPos.x + imageSubPos.y * dispatchDownscale;
	}
	#endif

	// read geometry buffer
	GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	bool isSky = (gbData.ClipDepth >= 1.0);

	[branch] if (!isSky)
	{
		float3 worldPos = ImagePosToWorldPos(imagePos, g_SceneCB.TargetSize.zw, gbData.ClipDepth, g_SceneCB.ViewProjInv);
		float distBasedEps = length(worldPos - g_SceneCB.CameraPos.xyz) * 5.0e-3;

		uint dispatchConstHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x);
		uint dispatchCrntHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x * (1 + g_SceneCB.FrameNumber * g_SceneCB.PerFrameJitter));
		float2 dispathNoise2d = BlueNoiseDiskLUT64[dispatchCrntHash % 64];
		float3x3 dispatchSamplingFrame;
		dispatchSamplingFrame[2] = float3(0.0, 0.0, 1.0);
		dispatchSamplingFrame[0] = normalize(float3(dispathNoise2d.xy, 0.0));
		dispatchSamplingFrame[1] = cross(dispatchSamplingFrame[2], dispatchSamplingFrame[0]);

		// Direct Shadow

		RayDesc ray;
		ray.Origin = worldPos + gbData.Normal * distBasedEps;
		ray.TMin = 1.0e-3;
		ray.TMax = 1.0e+4;

		#if (SHADOW_BUFFER_UINT32)
		occlusionPerLightPacked = 0x0;
		float occlusionPerLight[ShadowBufferEntriesPerPixel];
		[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j) occlusionPerLight[j] = 1.0;
		#endif

		// per light
		uint lightCount = min(g_SceneCB.Lighting.LightSourceCount, ShadowBufferEntriesPerPixel);
		#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
		uint ilight = (g_SceneCB.FrameNumber % lightCount);
		#else
		for (uint ilight = 0; ilight < lightCount; ++ilight)
		#endif
		{
			LightDesc lightDesc = g_SceneCB.Lighting.LightSources[ilight];
			float3x3 lightDirTBN = ComputeLightSamplingBasis(worldPos, lightDesc);
			float shadowFactor = 0.0;

			for (uint isample = 0; isample < g_SceneCB.SamplesPerLight; ++isample)
			{
				float3 sampleDir = GetDiskSampleDirection(isample + g_SceneCB.FrameNumber * g_SceneCB.SamplesPerLight * g_SceneCB.PerFrameJitter + dispatchConstHash);
				ray.Direction = mul(mul(sampleDir, dispatchSamplingFrame), lightDirTBN);
				ray.TMax = (LightType_Directional == lightDesc.Type ? 1.0e+4 : length(lightDesc.Position - ray.Origin) - lightDesc.Size);

				RayDataDirect rayData = (RayDataDirect)0;
				rayData.occluded = true;

				// ray trace
				uint instanceInclusionMask = 0xff;
				uint rayContributionToHitGroupIndex = 0;
				uint multiplierForGeometryContributionToShaderIndex = 1;
				uint missShaderIndex = RTShaderId_MissDirect;
				TraceRay(g_SceneStructure,
					RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
					instanceInclusionMask,
					rayContributionToHitGroupIndex,
					multiplierForGeometryContributionToShaderIndex,
					missShaderIndex,
					ray, rayData);

				shadowFactor += float(rayData.occluded);
			}
			shadowFactor = 1.0 - shadowFactor / g_SceneCB.SamplesPerLight;
			occlusionPerLight[ilight] = shadowFactor;
		}

		#if (SHADOW_BUFFER_APPLY_HISTORY_IN_RAYGEN)
		// apply accumulatd history
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
			float depthDeltaTolerance = gbData.ClipDepth * /*1.0e-4*/max(1.0e-6, g_SceneCB.DebugVec0[2]);
			float surfaceHistoryWeight = 1.0 - saturate(abs(gbData.ClipDepth - clipDepthPrev) / depthDeltaTolerance);
			#elif (0)
			// history rejection from Ray Tracing Gems "Ray traced Shadows"
			float3 normalVS = mul(float4(gbData.Normal, 0.0), g_SceneCB.View).xyz;
			//float depthDeltaTolerance = 0.003 + 0.017 * abs(normalVS.z);
			float depthDeltaTolerance = g_SceneCB.DebugVec0[1] + g_SceneCB.DebugVec0[2] * abs(normalVS.z);
			float surfaceHistoryWeight = (abs(1.0 - clipDepthPrev / gbData.ClipDepth) < depthDeltaTolerance ? 1.0 : 0.0);
			#else
			float3 worldPosPrev = ClipPosToWorldPos(float3(clipPosPrev.xy, clipDepthPrev), g_SceneCB.ViewProjInvPrev);
			float viewDepth = ClipDepthToViewDepth(gbData.ClipDepth, g_SceneCB.Proj);
			float worldPosTolerance = viewDepth * /*1.0e-3*/max(1.0e-6, g_SceneCB.DebugVec0[2]);
			float surfaceHistoryWeight = 1.0 - saturate(length(worldPos - worldPosPrev) / worldPosTolerance);
			surfaceHistoryWeight = saturate((surfaceHistoryWeight - g_SceneCB.DebugVec0[1]) / (1.0 - g_SceneCB.DebugVec0[1]));
			#endif

			uint counter = g_TracingHistory.Load(int3(dispatchPosPrev.xy, 0)).y;
			counter = (uint)floor(float(counter) * surfaceHistoryWeight + 0.5);
			float historyWeight = float(counter) / (counter + 1);

			#if (SHADOW_BUFFER_UINT32)
			uint shadowHistoryPacked = g_ShadowHistory.Load(int3(dispatchPosPrev.xy, 0));
			#else
			float4 shadowHistoryData = g_ShadowHistory.Load(int3(dispatchPosPrev.xy, 0));
			#endif
			[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
			{
				#if (SHADOW_BUFFER_UINT32)
				uint occlusionPackedPrev = ((shadowHistoryPacked >> (j * ShadowBufferBitsPerEntry)) & ShadowBufferEntryMask);
				float occlusionAccumulated = ShadowBufferEntryUnpack(occlusionPackedPrev);
				occlusionAccumulated = lerp(occlusionPerLight[j], occlusionAccumulated, historyWeight);
				#if (1)
				// note: following code guarantees minimal difference (if any) is applied to quantized result to overcome lack of precision at the end of the accumulation curve
				const float diffEps = 1.0e-4;
				float diff = occlusionPerLight[j] - occlusionAccumulated; // calculate difference before quantizing
				diff = (diff > diffEps ? 1.0f : (diff < -diffEps ? -1.0f : 0.0f));
				occlusionAccumulated = (float)ShadowBufferEntryPack(occlusionAccumulated); // quantize
				occlusionAccumulated = occlusionPackedPrev + max(abs(occlusionAccumulated - occlusionPackedPrev), abs(diff)) * diff;
				occlusionPerLight[j] = ShadowBufferEntryUnpack(occlusionAccumulated);
				#else
				occlusionPerLight[j] = occlusionAccumulated;
				#endif
				#else

				#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
				occlusionPerLight[j] = lerp(occlusionPerLight[j], shadowHistoryData[j], (ilight == j ? historyWeight : 1.0));
				#else
				occlusionPerLight[j] = lerp(occlusionPerLight[j], shadowHistoryData[j], historyWeight);
				#endif
				#endif
			}

			#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
			if (ilight == lightCount - 1)
			#endif
			counter = clamp(counter + 1, 0, g_SceneCB.AccumulationFrameCount);
			tracingInfo[1] = counter;
		}
		#endif

		#if (SHADOW_BUFFER_UINT32)
		// pack result
		[unroll] for (/*uint */j = 0; j < ShadowBufferEntriesPerPixel; ++j)
		{
			occlusionPerLightPacked |= ShadowBufferEntryPack(occlusionPerLight[j]) << (j * ShadowBufferBitsPerEntry);
		}
		#endif

		// Indirect Light

		// TODO
		// TEST: sample some sky light from upper hemisphere
		float3 skyDir = float3(gbData.Normal.x, max(gbData.Normal.y, 0.0), gbData.Normal.z);
		skyDir = normalize(skyDir * 0.5 + WorldUp);
		float3 skyLight = GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerTrilinearWrap, worldPos, skyDir).xyz;
		indirectLight = skyLight * 0.02;
	}

	// write result
	#if (SHADOW_BUFFER_UINT32)
	g_ShadowTarget[dispatchIdx.xy] = occlusionPerLightPacked;
	#else
	g_ShadowTarget[dispatchIdx.xy] = occlusionPerLight;
	#endif
	g_TracingInfoTarget[dispatchIdx.xy] = tracingInfo;
	g_IndirectLightTarget[dispatchIdx.xy] = float4(indirectLight.xyz, 0.0);
}

// miss: direct light

[shader("miss")]
void MissDirect(inout RayDataDirect rayData)
{
	rayData.occluded = false;
}

// closest: direct light

[shader("closesthit")]
void ClosestHitDirect(inout RayDataDirect rayData, in BuiltInTriangleIntersectionAttributes attribs)
{
	// noting to do
}