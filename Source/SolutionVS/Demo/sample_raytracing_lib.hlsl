
struct SceneConstants
{
	float4x4 viewProjInv;
	float4 cameraPos;
	float4 viewportSize; // w, h, 1/w, 1/h
};

ConstantBuffer<SceneConstants> g_SceneCB : register(b0);
RaytracingAccelerationStructure g_SceneStructure : register(t0);
RWTexture2D<float4> g_TargetTexture : register(u0);

typedef BuiltInTriangleIntersectionAttributes SampleHitAttributes;
struct SampleRayData
{
	float4 color;
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

	// test: simple orthographic projection
	float2 rayPixelPos = (float2)dispatchIdx.xy + 0.5;
	float2 rayUV = rayPixelPos / (float2)dispatchSize.xy;
	float2 rayClip = float2(rayUV.x, 1.0 - rayUV.y) * 2.0 - 1.0;
	ray.Origin = float3(rayClip, 0.0);
	ray.Direction = float3(0, 0, 1);

	SampleRayData rayData;
	rayData.color = 0.0;

	// ray trace
	uint instanceInclusionMask = 0xff;
	uint rayContributionToHitGroupIndex = 0;
	uint multiplierForGeometryContributionToShaderIndex = 1;
	uint missShaderIndex = 0;
	TraceRay(g_SceneStructure,
		RAY_FLAG_FORCE_OPAQUE /*| RAY_FLAG_CULL_BACK_FACING_TRIANGLES*/,
		instanceInclusionMask,
		rayContributionToHitGroupIndex,
		multiplierForGeometryContributionToShaderIndex,
		missShaderIndex,
		ray, rayData);

	if (rayData.color.a > 0)
	{
		// write ray result
		g_TargetTexture[dispatchIdx.xy] = rayData.color;
	}
	else
	{
		// TEMP: debug
		float4 targetValue = g_TargetTexture[dispatchIdx.xy];
		targetValue.xyz = lerp(targetValue.xyz, pow(float3(max(0.0, ray.Origin.xy), 0.0), 2) * 5, 0.5);
		g_TargetTexture[dispatchIdx.xy] = targetValue;
	}
}

[shader("closesthit")]
void SampleClosestHit(inout SampleRayData rayData, in SampleHitAttributes attribs)
{
	rayData.color = float4(0.0f, 0.0f, 1.0f, 1.0f);
}

[shader("miss")]
void SampleMiss(inout SampleRayData rayData)
{
	//rayData.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
}