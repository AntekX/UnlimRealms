
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants>	g_SceneCB			: register(b0);
Texture2D<float>				g_GeometryDepth		: register(t0);
Texture2D<float4>				g_GeometryImage0	: register(t1);
Texture2D<float4>				g_GeometryImage1	: register(t2);
Texture2D<float4>				g_GeometryImage2	: register(t3);
RaytracingAccelerationStructure	g_SceneStructure	: register(t4);
RWTexture2D<uint>				g_ShadowTarget		: register(u0);

// common functions

static const uint SampleCount = 32; // TODO: move to CB

float3 GetDiskSampleDirection(uint sampleId)
{
	#if (1)
	float2 dir2d = BlueNoiseDiskLUT64[sampleId % 64]; // [-1, 1]
	return float3(dir2d.xy, 1.0);
	#else
	float2 p2d = Hammersley(sampleId % SampleCount, SampleCount) * 2.0 - 1.0;
	float3 dir = HemisphereSampleCosine(p2d.x, p2d.y);
	return float3(p2d.xy, 1.0);
	#endif
}

float3x3 ComputeDirectLightTBN(const LightDesc lightDesc)
{
	float3x3 lightDirTBN;
	lightDirTBN[2] = -lightDesc.Direction.xyz;
	lightDirTBN[0] = float3(1, 0, 0);
	lightDirTBN[1] = normalize(cross(lightDirTBN[2], lightDirTBN[0]));
	lightDirTBN[0] = cross(lightDirTBN[1], lightDirTBN[2]);
	lightDirTBN[0] *= lightDesc.Size; // scale by tangent of the visible disk half angle (to transform [-1,1] disk samples)
	lightDirTBN[1] *= lightDesc.Size;
	return lightDirTBN;
}

// ray generation: direct light

[shader("raygeneration")]
void RayGenDirect()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint2 imagePos = dispatchIdx.xy; // TODO: support downscaled dispatch
	
	// read geometry buffer
	GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	bool isSky = (gbData.ClipDepth >= 1.0);

	uint occlusionPerLightPacked = 0xffffffff; // 4 lights x 8 bit shadow factor
	[branch] if (!isSky)
	{
		float3 worldPos = ImagePosToWorldPos(imagePos, g_SceneCB.TargetSize.zw, gbData.ClipDepth, g_SceneCB.ViewProjInv);
		float distBasedEps = length(worldPos - g_SceneCB.CameraPos.xyz) * 5.0e-3;
		
		uint dispatchHash = HashUInt(dispatchIdx.x + dispatchIdx.y * (uint)g_SceneCB.TargetSize.x + g_SceneCB.FrameNumber);
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
		occlusionPerLightPacked = 0x0;
		for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
		{
			LightDesc lightDesc = g_SceneCB.Lighting.LightSources[ilight];
			float3x3 lightDirTBN = ComputeDirectLightTBN(lightDesc);
			float shadowFactor = 0.0;

			for (uint isample = 0; isample < SampleCount; ++isample)
			{
				if (SampleCount > 1)
				{
					float3 sampleDir = GetDiskSampleDirection(isample);
					ray.Direction = mul(mul(sampleDir, dispatchSamplingFrame), lightDirTBN);
				}
				else
				{
					ray.Direction = -g_SceneCB.Lighting.LightSources[ilight].Direction.xyz;
				}

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
			shadowFactor = 1.0 - shadowFactor / SampleCount;
			occlusionPerLightPacked |= uint(shadowFactor * 0xff) << (ilight * 0x8);
		}
	}

	// write result
	g_ShadowTarget[dispatchIdx.xy] = occlusionPerLightPacked;
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