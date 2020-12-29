
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants>	g_SceneCB			: register(b0);
Texture2D<float>				g_GeometryDepth		: register(t0);
Texture2D<float4>				g_GeometryImage0	: register(t1);
Texture2D<float4>				g_GeometryImage1	: register(t2);
Texture2D<float4>				g_GeometryImage2	: register(t3);
RaytracingAccelerationStructure	g_SceneStructure	: register(t4);
RWTexture2D<float>				g_ShadowTarget		: register(u0);

// ray generation: direct light

[shader("raygeneration")]
void RayGenDirect()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint2 imagePos = dispatchIdx.xy; // TODO: support downscaled dispatch
	float clipDepth = g_GeometryDepth.Load(int3(imagePos.xy, 0));
	bool isSky = (clipDepth >= 1.0);

	RayDataDirect rayData = (RayDataDirect)0;

	[branch] if (!isSky)
	{
		float2 uvPos = (float2(imagePos)+0.5) * g_SceneCB.TargetSize.zw;
		float3 clipPos = float3(float2(uvPos.x, 1.0 - uvPos.y) * 2.0 - 1.0, clipDepth);
		float3 worldPos = ClipPosToWorldPos(clipPos, g_SceneCB.ViewProjInv);
		float distBasedEps = length(worldPos - g_SceneCB.CameraPos.xyz) * 0.01;

		// TODO: use surface normal for Origin offset
		RayDesc ray;
		ray.Origin = worldPos - g_SceneCB.CameraDir.xyz * distBasedEps;
		ray.Direction = -g_SceneCB.Lighting.LightSources[0].Direction.xyz;
		ray.TMin = distBasedEps;
		ray.TMax = 1.0e+3;
		
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
	}

	// write result
	g_ShadowTarget[dispatchIdx.xy] = (rayData.occluded ? 0.0 : 1.0);
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