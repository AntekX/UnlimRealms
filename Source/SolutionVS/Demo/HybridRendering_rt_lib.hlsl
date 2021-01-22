
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants>	g_SceneCB			: register(b0);
Texture2D<float>				g_GeometryDepth		: register(t0);
Texture2D<float4>				g_GeometryImage0	: register(t1);
Texture2D<float4>				g_GeometryImage1	: register(t2);
Texture2D<float4>				g_GeometryImage2	: register(t3);
Texture2D<uint>					g_ShadowHistory		: register(t4);
Texture2D<uint2>				g_TracingHistory	: register(t5);
RaytracingAccelerationStructure	g_SceneStructure	: register(t6);
RWTexture2D<uint>				g_ShadowTarget		: register(u0);
RWTexture2D<uint2>				g_TracingInfoTarget	: register(u1);

// common functions

float3 GetDiskSampleDirection(uint sampleId)
{
	#if (1)
	float2 dir2d = BlueNoiseDiskLUT64[sampleId % 64]; // [-1, 1]
	return float3(dir2d.xy, 1.0);
	#else
	const uint SampleCount = g_SceneCB.SamplesPerLight * max(1, g_SceneCB.DebugVec0[3] + 1);
	float2 p2d = Hammersley(sampleId % SampleCount, SampleCount) * 2.0 - 1.0;
	float3 dir = HemisphereSampleCosine(p2d.x, p2d.y);
	return float3(p2d.xy, 1.0);
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
	uint occlusionPerLightPacked = 0xffffffff; // 4 lights x 8 bit shadow factor
	uint2 tracingInfo = 0; // sub sample pos & counter

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

		uint frameHashOfs = g_SceneCB.FrameNumber * dispatchSize.x * dispatchSize.y * g_SceneCB.PerFrameJitter;
		uint dispatchHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x + frameHashOfs);
		float2 dispathNoise2d = BlueNoiseDiskLUT64[dispatchHash % 64];
		float3x3 dispatchSamplingFrame;
		dispatchSamplingFrame[2] = float3(0.0, 0.0, 1.0);
		dispatchSamplingFrame[0] = normalize(float3(dispathNoise2d.xy, 0.0));
		dispatchSamplingFrame[1] = cross(dispatchSamplingFrame[2], dispatchSamplingFrame[0]);

		RayDesc ray;
		ray.Origin = worldPos + gbData.Normal * distBasedEps;
		ray.TMin = 1.0e-3;
		ray.TMax = 1.0e+4;

		// per light

		float4 occlusionPerLight = 0;
		for (uint ilight = 0; ilight < min(g_SceneCB.Lighting.LightSourceCount, ShadowBufferEntriesPerPixel); ++ilight)
		{
			LightDesc lightDesc = g_SceneCB.Lighting.LightSources[ilight];
			float3x3 lightDirTBN = ComputeLightSamplingBasis(worldPos, lightDesc);
			float shadowFactor = 0.0;

			for (uint isample = 0; isample < g_SceneCB.SamplesPerLight; ++isample)
			{
				float3 sampleDir = GetDiskSampleDirection(isample + g_SceneCB.FrameNumber * g_SceneCB.SamplesPerLight * g_SceneCB.PerFrameJitter);
				ray.Direction = mul(mul(sampleDir, dispatchSamplingFrame), lightDirTBN);
				//ray.Direction = -g_SceneCB.Lighting.LightSources[ilight].Direction.xyz;
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

		// apply accumulatd history
		float4 clipPosPrev = mul(float4(worldPos, 1.0), g_SceneCB.ViewProjPrev);
		clipPosPrev.xy /= clipPosPrev.w;
		if (all(abs(clipPosPrev.xy) < 1.0) && g_SceneCB.DebugVec0[3] > 0)
		{
			float2 imagePosPrev = (clipPosPrev.xy * float2(1.0, -1.0) + 1.0) * 0.5 * g_SceneCB.TargetSize.xy;
			uint2 dispatchPosPrev = clamp(floor(imagePosPrev) * g_SceneCB.LightBufferDownscale.y, float2(0, 0), g_SceneCB.LightBufferSize.xy - 1);
			//dispatchPosPrev = dispatchIdx.xy; // TEMP: exclude preprojection
			uint counter = g_TracingHistory.Load(int3(dispatchPosPrev.xy, 0)).y;
			uint shadowHistoryPacked = g_ShadowHistory.Load(int3(dispatchPosPrev.xy, 0));
			[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
			{
				float occlusionPackedPrev = float((shadowHistoryPacked >> (j * 0x8)) & 0xff);
				float occlusionAccumulated = occlusionPackedPrev / 0xff;
				if (g_SceneCB.DebugVec0[2] > 0) occlusionPerLight[j] = g_SceneCB.DebugVec0[2]; // TEMP: override current data
				occlusionAccumulated = lerp(occlusionPerLight[j], occlusionAccumulated, /*float(counter) / (counter + 1)*/g_SceneCB.DebugVec0[3] / (g_SceneCB.DebugVec0[3] + 1));
				// note: following code guarantees minimal difference (if any) is applied to quantized result to overcome lack of precision at the end of the accumulation curve
				float diff = occlusionPerLight[j] - occlusionAccumulated; // calculate difference before quantizing
				diff = (diff > 0 ? 1.0f : (diff < 0 ? -1.0f : 0.0f));
				occlusionAccumulated = floor(occlusionAccumulated * 0xff + 0.5); // quantize
				occlusionAccumulated = occlusionPackedPrev + max(abs(occlusionAccumulated - occlusionPackedPrev), abs(diff)) * diff;
				occlusionPerLight[j] = occlusionAccumulated / 0xff;
			}
			counter = clamp(counter + 1, 0, /*AccumulatedSamplesCount*/g_SceneCB.DebugVec0[3]);
			tracingInfo[1] = counter;
		}

		// pack result
		occlusionPerLightPacked = 0x0;
		[unroll] for (uint j = 0; j < 4; ++j)
		{
			occlusionPerLightPacked |= uint(floor(occlusionPerLight[j] * 0xff + 0.5)) << (j * 0x8);
		}
	}

	// write result
	g_ShadowTarget[dispatchIdx.xy] = occlusionPerLightPacked;
	g_TracingInfoTarget[dispatchIdx.xy] = tracingInfo;
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