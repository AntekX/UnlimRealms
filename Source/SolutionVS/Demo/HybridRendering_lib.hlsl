
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
Texture2D<float>	g_GeometryDepth			: register(t0);
Texture2D<float4>	g_GeometryImage0		: register(t1);
Texture2D<float4>	g_GeometryImage1		: register(t2);
RWTexture2D<float4>	g_LightingTarget		: register(u0);

// compute lighting

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeLighting(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 imagePos = dispatchThreadId.xy;
	float geomtryDepth = g_GeometryDepth.Load(int3(imagePos.xy, 0));
	float4 geomtryImage0 = g_GeometryImage0.Load(int3(imagePos.xy, 0));
	float4 geomtryImage1 = g_GeometryImage1.Load(int3(imagePos.xy, 0));
	//float3 normal = float3(geomtryImage1.xy * 2.0 - 1.0, 0);
	//normal.z = sqrt(1.0 - dot(geomtryImage1.xy, geomtryImage1.xy));
	float3 normal = geomtryImage1.xyz * 2.0 - 1.0;

	float3 lightColor = (geomtryDepth == 1.0 ? float3(0.8, 0.9, 1.0) : saturate(dot(normal,-g_SceneCB.cameraDir.xyz)));

	g_LightingTarget[imagePos] = float4(lightColor, 1.0);
}

// TEMP
[shader("compute")]
[numthreads(8, 8, 1)]
void dummyShaderToMakeFXCHappy(const uint3 dispatchThreadId : SV_DispatchThreadID)
{

}