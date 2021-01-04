
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants>	g_SceneCB			: register(b0);
Texture2D<float>				g_GeometryDepth		: register(t0);
Texture2D<float4>				g_GeometryImage0	: register(t1);
Texture2D<float4>				g_GeometryImage1	: register(t2);
Texture2D<float4>				g_GeometryImage2	: register(t3);
RaytracingAccelerationStructure	g_SceneStructure	: register(t4);
RWTexture2D<uint>				g_ShadowTarget		: register(u0);

// ray generation: direct light

[shader("raygeneration")]
void RayGenDirect()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint2 imagePos = dispatchIdx.xy; // TODO: support downscaled dispatch
	
	// read geometry buffer

	GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	bool isSky = (gbData.ClipDepth >= 1.0);

	uint occlusionPerLightPacked = 0xffffffff; // 8 lights maximum
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
			ray.Direction = -g_SceneCB.Lighting.LightSources[ilight].Direction.xyz;

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
			
			occlusionPerLightPacked |= uint(rayData.occluded) << ilight;
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