
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

static const uint SampleCount = 64; // TODO: move to CB

float3 GetSampleDirection(uint seed)
{
	float diskSize = tan(SolarDiskHalfAngle * 4); // todo: light constant, move outside
	float2 dir2d = diskSize * BlueNoiseDiskLUT64[seed % 64]; // [-1, 1]
	return float3(dir2d.xy, 1.0);
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

		RayDesc ray;
		ray.Origin = worldPos + gbData.Normal * distBasedEps;
		ray.TMin = 1.0e-3;
		ray.TMax = 1.0e+4;

		// per light
		occlusionPerLightPacked = 0x0;
		for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
		{
			float shadowFactor = 0.0;
			for (uint isample = 0; isample < SampleCount; ++isample)
			{
				if (SampleCount > 1)
				{
					float3 sampleDir = GetSampleDirection(isample + g_SceneCB.FrameNumber);
					float3x3 sampleTBN;
					sampleTBN[2] = -g_SceneCB.Lighting.LightSources[ilight].Direction.xyz;
					sampleTBN[0] = float3(1, 0, 0);
					sampleTBN[1] = normalize(cross(sampleTBN[2], sampleTBN[0]));
					sampleTBN[0] = cross(sampleTBN[1], sampleTBN[2]);
					ray.Direction = mul(sampleDir, sampleTBN);
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