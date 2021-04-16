
#include "HybridRendering.hlsli"

// common functions

float3 GetDiskSampleDirection(uint sampleId, uint sampleCount)
{
	#if (1)
	float2 dir2d = BlueNoiseDiskLUT64[sampleId % 64]; // [-1, 1]
	return float3(dir2d.xy, 1.0);
	#else
	float2 p2d = Hammersley(sampleId % sampleCount, sampleCount);
	float3 dir = HemisphereSampleUniform(p2d.x, p2d.y);
	return dir.xyz;
	#endif
}

float3 GetHemisphereSampleDirection(uint sampleId, uint sampleCount)
{
	#if (1)
	float2 p2d = Hammersley(sampleId % sampleCount + 1, sampleCount + 1);
	#else
	float2 p2d = Halton(sampleId % sampleCount);
	#endif
	float3 dir = HemisphereSampleCosine(p2d.x, p2d.y);
	return dir.xyz;
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

float3x3 ComputeSamplingBasis(const float3 direction)
{
	float3x3 sampleTBN;
	sampleTBN[2] = direction;
	sampleTBN[0] = float3(1, 0, 0);
	sampleTBN[1] = normalize(cross(sampleTBN[2], sampleTBN[0]));
	sampleTBN[0] = cross(sampleTBN[1], sampleTBN[2]);
	return sampleTBN;
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
	uint4 tracingInfo = 0; // sub sample pos & counters
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
		float worldDist = length(worldPos - g_SceneCB.CameraPos.xyz);
		float distBasedEps = worldDist * 5.0e-3;

		uint dispatchConstHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x);
		uint dispatchCrntHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x * (1 + g_SceneCB.FrameNumber * g_SceneCB.PerFrameJitter));
		float2 dispathNoise2d = BlueNoiseDiskLUT64[dispatchCrntHash % 64];
		float3x3 dispatchSamplingFrame;
		dispatchSamplingFrame[2] = float3(0.0, 0.0, 1.0);
		dispatchSamplingFrame[0] = normalize(float3(dispathNoise2d.xy, 0.0));
		dispatchSamplingFrame[1] = cross(dispatchSamplingFrame[2], dispatchSamplingFrame[0]);

		// Direct Shadow

		if (g_SceneCB.ShadowSamplesPerLight > 0)
		{
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

				uint sampleCount = g_SceneCB.ShadowSamplesPerLight * max(1, g_SceneCB.ShadowAccumulationFrames);
				uint sampleIdOfs = g_SceneCB.FrameNumber * g_SceneCB.ShadowSamplesPerLight * g_SceneCB.PerFrameJitter + dispatchConstHash;
				for (uint isample = 0; isample < g_SceneCB.ShadowSamplesPerLight; ++isample)
				{
					float3 sampleDir = GetDiskSampleDirection(isample + sampleIdOfs, sampleCount);
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
				shadowFactor = 1.0 - shadowFactor / g_SceneCB.ShadowSamplesPerLight;
				occlusionPerLight[ilight] = shadowFactor;
			}

			#if (SHADOW_BUFFER_APPLY_HISTORY_IN_RAYGEN)
			// apply accumulatd history
			float4 clipPosPrev = mul(float4(worldPos, 1.0), g_SceneCB.ViewProjPrev);
			clipPosPrev.xy /= clipPosPrev.w;
			if (g_SceneCB.ShadowAccumulationFrames > 0 && all(abs(clipPosPrev.xy) < 1.0))
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
					counter = clamp(counter + 1, 0, g_SceneCB.ShadowAccumulationFrames);
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
		}

		// Indirect Light

		if (g_SceneCB.IndirectSamplesPerFrame > 0)
		{
			// sky light with ambient occlusion
			
			RayDesc ray;
			ray.Origin = worldPos + gbData.Normal * distBasedEps;
			ray.TMax = worldDist * g_SceneCB.DebugVec1[0];// 20.0;
			ray.TMin = 1.0e-3;// ray.TMax * 1.0e-4;

			float3x3 surfaceTBN = ComputeSamplingBasis(gbData.Normal);
			float ambientOcclusion = 0.0;
			float3 skyLight = 0.0;
			uint sampleCount = g_SceneCB.IndirectSamplesPerFrame * (g_SceneCB.IndirectAccumulationFrames + 1);
			uint sampleIdOfs = g_SceneCB.FrameNumber * g_SceneCB.IndirectSamplesPerFrame * g_SceneCB.PerFrameJitter + dispatchConstHash;
			for (uint isample = 0; isample < g_SceneCB.IndirectSamplesPerFrame; ++isample)
			{
				float3 sampleDir = GetHemisphereSampleDirection(isample + sampleIdOfs, sampleCount);
				ray.Direction = mul(mul(sampleDir, dispatchSamplingFrame), surfaceTBN);

				RayDataDirect rayData = (RayDataDirect)0;
				rayData.occluded = true;
				rayData.recusrionDepth = 0;
				rayData.hitDist = ray.TMax;

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

				float sampleOcclusion = float(rayData.occluded) * saturate(1.0 - rayData.hitDist / ray.TMax);
				ambientOcclusion += sampleOcclusion;

				float3 skyDir = ray.Direction;
				float NdotL = saturate(dot(gbData.Normal, ray.Direction));
				float3 absorption = 1.0;
				if (skyDir.z <= 0.0)
				{
					// bounced sky light approximation
					skyDir = reflect(skyDir, gbData.Normal);
					absorption = g_SceneCB.Material.BaseColor;
				}
				skyLight += GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerTrilinearWrap, worldPos, skyDir).xyz * absorption * NdotL * (1.0 - sampleOcclusion);
			}
			ambientOcclusion = 1.0 - ambientOcclusion / g_SceneCB.IndirectSamplesPerFrame;
			indirectLight = skyLight / g_SceneCB.IndirectSamplesPerFrame;
		}
		else
		{
			// simplified ambient
			float3 skyDir = float3(gbData.Normal.x, max(gbData.Normal.y, 0.0), gbData.Normal.z);
			skyDir = normalize(skyDir * 0.5 + WorldUp);
			float3 skyLight = GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerTrilinearWrap, worldPos, skyDir).xyz;
			indirectLight = skyLight * 0.05;
		}
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
	rayData.hitDist = RayTCurrent();
	rayData.recusrionDepth += 1;

	#if (0)
	// TODO

	const uint IndirectLightBounces = 1;
	if (rayData.recusrionDepth <= IndirectLightBounces)
	{
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
	}

	#endif
}