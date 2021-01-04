
#include "HybridRendering.hlsli"

// common bindings

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
sampler				g_SamplerBilinear		: register(s0);
Texture2D<float>	g_GeometryDepth			: register(t0);
Texture2D<float4>	g_GeometryImage0		: register(t1);
Texture2D<float4>	g_GeometryImage1		: register(t2);
Texture2D<float4>	g_GeometryImage2		: register(t3);
Texture2D<uint>		g_ShadowResult			: register(t4);
RWTexture2D<float4>	g_LightingTarget		: register(u0);

// lighting common

float4 CalculateSkyLight(const float3 position, const float3 direction)
{
	float height = lerp(g_SceneCB.Atmosphere.InnerRadius, g_SceneCB.Atmosphere.OuterRadius, 0.05);
	float4 color = 0.0;
	for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
	{
		LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
		if (LightType_Directional != light.Type)
			continue;
		float3 worldFrom = position + float3(0.0, height, 0.0);
		float3 worldTo = worldFrom + direction;
		color += AtmosphericScatteringSky(g_SceneCB.Atmosphere, light, worldTo, worldFrom);
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
	bool isSky = (clipDepth >= 1.0);
	float2 uvPos = (float2(imagePos)+0.5) * g_SceneCB.TargetSize.zw;
	float3 clipPos = float3(float2(uvPos.x, 1.0 - uvPos.y) * 2.0 - 1.0, clipDepth);
	float3 worldPos = ClipPosToWorldPos(clipPos, g_SceneCB.ViewProjInv);
	float3 worldRay = normalize(worldPos - g_SceneCB.CameraPos.xyz);

	float3 lightingResult = 0.0;

	[branch] if (isSky)
	{
		lightingResult = CalculateSkyLight(g_SceneCB.CameraPos.xyz, worldRay).xyz;
	}
	else
	{
		// read geometry buffer

		GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);

		// read lighting buffer
		
		uint shadowPerLightPacked = g_ShadowResult.Load(int3(imagePos.xy, 0));

		// material params

		MaterialInputs material = (MaterialInputs)0;
		initMaterial(material);
		material.normal = gbData.Normal;
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
			material.baseColor.xyz = 0.5;
			material.roughness = 0.5;
			material.metallic = 0.0;
			material.reflectance = 0.04;
		}

		// lighting params

		LightingParams lightingParams;
		getLightingParams(worldPos, g_SceneCB.CameraPos.xyz, material, lightingParams);

		// direct light

		float3 directLightColor = 0;
		for (uint ilight = 0; ilight < g_SceneCB.Lighting.LightSourceCount; ++ilight)
		{
			LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
			float shadowFactor = float((shadowPerLightPacked >> (ilight * 0x8)) & 0xff) / 255.0;
			float NoL = dot(-light.Direction, lightingParams.normal);
			shadowFactor *= saturate(NoL  * 10.0); // temp: simplified self shadowing
			float specularOcclusion = shadowFactor;
			directLightColor += EvaluateDirectLighting(lightingParams, light, shadowFactor, specularOcclusion).xyz;
		}

		// indirect light

		float3 envDir = float3(lightingParams.normal.x, max(lightingParams.normal.y, 0.0), lightingParams.normal.z);
		float3 envColor = CalculateSkyLight(worldPos, normalize(envDir * 0.5 + WorldUp)).xyz; // simplified sky light
		float3 indirectLightColor = lightingParams.diffuseColor.xyz * envColor; // no indirect spec

		// final

		lightingResult = directLightColor + indirectLightColor;
	}

	g_LightingTarget[imagePos] = float4(lightingResult, 1.0);
}