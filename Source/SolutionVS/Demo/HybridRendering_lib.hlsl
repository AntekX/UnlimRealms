
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
Texture2D<float4> g_GeometryDepth			: register(t0);
Texture2D<float4> g_GeometryImage0			: register(t1);
Texture2D<float4> g_GeometryImage1			: register(t2);
RWTexture2D<float4> g_LightingTarget		: register(u0);

// compute lighting

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeLighting(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 imagePos = dispatchThreadId.xy;
	float4 geomtryImage0 = g_GeometryImage1.Load(int3(imagePos.xy, 0));
	float3 normal = float3(geomtryImage0.xy, 0);
	normal.z = sqrt(1.0 - dot(geomtryImage0.xy, geomtryImage0.xy));

	g_LightingTarget[imagePos] = dot(normal, normalize(float3(1,1,-1)));
}

// TEMP
[shader("compute")]
[numthreads(8, 8, 1)]
void dummyShaderToMakeFXCHappy(const uint3 dispatchThreadId : SV_DispatchThreadID)
{

}