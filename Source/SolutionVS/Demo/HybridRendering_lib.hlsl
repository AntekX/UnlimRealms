
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
Texture2D<float>	g_GeometryDepth			: register(t0);
Texture2D<float4>	g_GeometryImage0		: register(t1);
Texture2D<float4>	g_GeometryImage1		: register(t2);
Texture2D<float4>	g_GeometryImage2		: register(t3);
RWTexture2D<float4>	g_LightingTarget		: register(u0);

// lighting common

float4 CalculateSkyLight(const float3 position, const float3 direction)
{
	const float SkyIrradianceFactor = Pi;
	float height = lerp(g_SceneCB.Atmosphere.InnerRadius, g_SceneCB.Atmosphere.OuterRadius, 0.05);
	float4 color = 0.0;
	for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
	{
		LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
		float3 worldFrom = position + float3(0.0, height, 0.0);
		float3 worldTo = worldFrom + direction;
		color += AtmosphericScatteringSky(g_SceneCB.Atmosphere, light, worldTo, worldFrom) * SkyIrradianceFactor;
	}
	color.w = min(1.0, color.w);
	return color;
}

// compute lighting

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeLighting(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 imagePos = dispatchThreadId.xy;
	float clipDepth = g_GeometryDepth.Load(int3(imagePos.xy, 0));
	float4 geomtryData0 = g_GeometryImage0.Load(int3(imagePos.xy, 0));
	float4 geomtryData1 = g_GeometryImage1.Load(int3(imagePos.xy, 0));
	float4 geomtryData2 = g_GeometryImage2.Load(int3(imagePos.xy, 0));

	bool isSky = (clipDepth >= 1.0);
	float3 normal = geomtryData1.xyz * 2.0 - 1.0;
	float2 uvPos = (float2(imagePos) + 0.5) * g_SceneCB.TargetSize.zw;
	float3 clipPos = float3(float2(uvPos.x, 1.0 - uvPos.y) * 2.0 - 1.0, clipDepth);
	float viewDepth = ClipDepthToViewDepth(clipDepth, g_SceneCB.Proj);
	float3 worldPos = ClipPosToWorldPos(clipPos, g_SceneCB.ViewProjInv);
	//float3 worldRay = normalize(ClipPosToWorldPos(float3(clipPos.xy, 0), g_SceneCB.ViewProjInv) - g_SceneCB.CameraPos.xyz);
	float3 worldRay = normalize(worldPos - g_SceneCB.CameraPos.xyz);

	// material params

	MaterialInputs material = (MaterialInputs)0;
	initMaterial(material);
	material.normal = normal;
	if (g_SceneCB.OverrideMaterial)
	{
		material.baseColor.xyz = g_SceneCB.Material.BaseColor;
		material.roughness = g_SceneCB.Material.Roughness;
		material.metallic = g_SceneCB.Material.Metallic;
		material.reflectance = g_SceneCB.Material.Reflectance;
	}
	else
	{
		// TODO: read mesh material
		material.baseColor.xyz = 1.0;
		material.roughness = 1.0;
		material.metallic = 0.0;
		material.reflectance = 0.04;
	}

	// lighting params

	LightingParams lightingParams;
	getLightingParams(worldPos, g_SceneCB.CameraPos.xyz, material, lightingParams);

	// calculate direct light

	float3 directLightColor = 0;
	for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
	{
		LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
		float shadowFactor = 1.0;
		float specularOcclusion = 1.0;
		directLightColor += EvaluateDirectLighting(lightingParams, light, shadowFactor, specularOcclusion).xyz;
	}

	// final result

	float4 skyLight = 0.0;
	[branch] if (isSky)
	{
		skyLight = CalculateSkyLight(g_SceneCB.CameraPos.xyz, worldRay);
	}
	
	float3 lightingResult = directLightColor * !isSky + skyLight.xyz * isSky;

	g_LightingTarget[imagePos] = float4(lightingResult, 1.0);
}

// TEMP
[shader("compute")]
[numthreads(8, 8, 1)]
void dummyShaderToMakeFXCHappy(const uint3 dispatchThreadId : SV_DispatchThreadID)
{

}