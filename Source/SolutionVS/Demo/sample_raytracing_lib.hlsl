
struct SceneConstants
{
	float4x4 projToWorld;
	float4 cameraPos;
	float4 viewportSize; // w, h, 1/w, 1/h
};

RaytracingAccelerationStructure g_SceneStructure : register(t0);
RWTexture2D<float4> g_TargetTexture : register(u0);
ConstantBuffer<SceneConstants> g_SceneCB : register(b0);

typedef BuiltInTriangleIntersectionAttributes SampleHitAttributes;
struct SampleRayData
{
	float4 color;
};

[shader("raygeneration")]
void SampleRaygenShader()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint3 dispatchSize = DispatchRaysDimensions();
	if (dispatchIdx.x >= (uint)g_SceneCB.viewportSize.x || dispatchIdx.y >= (uint)g_SceneCB.viewportSize.y)
		return;
	
	RayDesc ray;
	ray.TMin = 0.001;
	ray.TMax = 1.0e+4;

	// test: simaple orthographic projection
	float2 rayScreenPos = (float2)dispatchIdx.xy / (float2)dispatchSize.xy;
	ray.Origin = float3(rayScreenPos.xy * g_SceneCB.viewportSize.zw, 0.0);
	ray.Direction = float3(0, 0, 1);

	SampleRayData rayData;
	rayData.color = 0.0;

	// ray trace
	uint instanceInclusionMask = 0xff;
	uint rayContributionToHitGroupIndex = 0;
	uint multiplierForGeometryContributionToShaderIndex = 1;
	uint missShaderIndex = 0;
	TraceRay(g_SceneStructure,
		RAY_FLAG_NONE /*| RAY_FLAG_CULL_BACK_FACING_TRIANGLES*/,
		instanceInclusionMask,
		rayContributionToHitGroupIndex,
		multiplierForGeometryContributionToShaderIndex,
		missShaderIndex,
		ray, rayData);

	// write ray result
	g_TargetTexture[dispatchIdx.xy] = rayData.color;
}