
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
Texture2D<float>	g_GeometryDepth			: register(t0);
Texture2D<float4>	g_GeometryImage0		: register(t1);
Texture2D<float4>	g_GeometryImage1		: register(t2);
Texture2D<float4>	g_GeometryImage2		: register(t3);
RWTexture2D<float4>	g_LightingTarget		: register(u0);

// compute lighting

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeLighting(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 imagePos = dispatchThreadId.xy;
	float geomtryClipDepth = g_GeometryDepth.Load(int3(imagePos.xy, 0));
	float4 geomtryImage0 = g_GeometryImage0.Load(int3(imagePos.xy, 0));
	float4 geomtryImage1 = g_GeometryImage1.Load(int3(imagePos.xy, 0));
	float4 geomtryImage2 = g_GeometryImage2.Load(int3(imagePos.xy, 0));

	// TEMP: some test shading
	const float FarClipDistance = 1.0e+4;
	const float NearClipDistance = 0.1;
	float projectionA = FarClipDistance / (FarClipDistance - NearClipDistance);
	float projectionB = (-FarClipDistance * NearClipDistance) / (FarClipDistance - NearClipDistance);
	float viewDepth = projectionB / (geomtryClipDepth - projectionA);
	float3 normal = geomtryImage1.xyz * 2.0 - 1.0;
	float3 skyColor = float3(135, 206, 235) / 255.0 * 6;
	float3 lightIntensity = 2.0;
	float3 lightDir = -g_SceneCB.cameraDir.xyz;
	float3 lightColor = (geomtryClipDepth == 1.0 ? skyColor : saturate(dot(normal, lightDir)) * lightIntensity);
	float3 transmittance = pow(1.0 - saturate(viewDepth / 64.0), 2);
	float3 inscatteredLight = skyColor * (1.0 - transmittance);
	lightColor = lightColor * transmittance + inscatteredLight;

	g_LightingTarget[imagePos] = float4(lightColor, 1.0);
}

// TEMP
[shader("compute")]
[numthreads(8, 8, 1)]
void dummyShaderToMakeFXCHappy(const uint3 dispatchThreadId : SV_DispatchThreadID)
{

}