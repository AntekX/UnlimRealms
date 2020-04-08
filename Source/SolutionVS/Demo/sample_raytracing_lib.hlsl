
struct SceneConstants
{
	float4x4 viewProjInv;
	float4 cameraPos;
	float4 viewportSize; // w, h, 1/w, 1/h
	float4 clearColor;
	float4 lightDirection;
};

ConstantBuffer<SceneConstants> g_SceneCB : register(b0);
RaytracingAccelerationStructure g_SceneStructure : register(t0);
RWTexture2D<float4> g_TargetTexture : register(u0);

typedef BuiltInTriangleIntersectionAttributes SampleHitAttributes;
struct SampleRayData
{
	float4 color;
	float2 clipPos;
};
struct SampleShadowData
{
	bool occluded;
};

[shader("raygeneration")]
void SampleRaygen()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint3 dispatchSize = DispatchRaysDimensions();
	if (dispatchIdx.x >= (uint)g_SceneCB.viewportSize.x || dispatchIdx.y >= (uint)g_SceneCB.viewportSize.y)
		return;
	
	RayDesc ray;
	ray.TMin = 0.001;
	ray.TMax = 1.0e+4;

	float2 rayPixelPos = (float2)dispatchIdx.xy + 0.5;
	float2 rayUV = rayPixelPos / (float2)dispatchSize.xy;
	float2 rayClip = float2(rayUV.x, 1.0 - rayUV.y) * 2.0 - 1.0;
	#if (0)
	// test: simple orthographic projection
	ray.Origin = float3(rayClip, 0.0);
	ray.Direction = float3(0, 0, 1);
	#else
	// calculate camera ray
	float4 rayWorld = mul(g_SceneCB.viewProjInv, float4(rayClip.xy, 0.0, 1.0));
	rayWorld.xyz /= rayWorld.w;
	ray.Origin = g_SceneCB.cameraPos.xyz;
	ray.Direction = normalize(rayWorld.xyz - ray.Origin);
	#endif

	SampleRayData rayData;
	rayData.color = 0.0;
	rayData.clipPos = rayClip;

	// ray trace
	uint instanceInclusionMask = 0xff;
	uint rayContributionToHitGroupIndex = 0;
	uint multiplierForGeometryContributionToShaderIndex = 1;
	uint missShaderIndex = 0;
	TraceRay(g_SceneStructure,
		RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		instanceInclusionMask,
		rayContributionToHitGroupIndex,
		multiplierForGeometryContributionToShaderIndex,
		missShaderIndex,
		ray, rayData);

	// write ray result
	float4 targetValue = g_SceneCB.clearColor;
	targetValue.xyz = lerp(targetValue.xyz, rayData.color.xyz, rayData.color.a);
	g_TargetTexture[dispatchIdx.xy] = targetValue;
}

[shader("miss")]
void SampleMiss(inout SampleRayData rayData)
{
	rayData.color.xyz = float3(max(rayData.clipPos.xy, 0.0), 0.0);
	rayData.color.w = saturate(max(rayData.color.x, rayData.color.y));
}

[shader("miss")]
void SampleMissShadow(inout SampleShadowData shadowData)
{
	shadowData.occluded = false;
}

[shader("closesthit")]
void SampleClosestHit(inout SampleRayData rayData, in SampleHitAttributes attribs)
{
	float3 baryCoords = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	rayData.color = float4(baryCoords.xyz, 1.0);

	// simple shadow test
	#if (1)

	SampleShadowData shadowData;
	shadowData.occluded = true;

	RayDesc ray;
	ray.TMin = 0.001;
	ray.TMax = 1.0e+4;
	ray.Origin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();;
	ray.Direction = g_SceneCB.lightDirection.xyz;

	uint instanceInclusionMask = 0xff;
	uint rayContributionToHitGroupIndex = 0;
	uint multiplierForGeometryContributionToShaderIndex = 1;
	uint missShaderIndex = 1;
	TraceRay(g_SceneStructure,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
		RAY_FLAG_FORCE_OPAQUE | // skip any hit shaders
		RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // skip closets hit shaders (only miss shader writes to payload)
		instanceInclusionMask,
		rayContributionToHitGroupIndex,
		multiplierForGeometryContributionToShaderIndex,
		missShaderIndex,
		ray, shadowData);

	if (shadowData.occluded)
	{
		rayData.color.xyz *= 0.5;
	}

	#endif
}