
#include "HybridRendering.hlsli"
#include "HDRRender/HDRRender.hlsli"

// common bindings

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
sampler				g_SamplerBilinear		: register(s0);
Texture2D<float>	g_GeometryDepth			: register(t0);
Texture2D<float4>	g_GeometryImage0		: register(t1);
Texture2D<float4>	g_GeometryImage1		: register(t2);
Texture2D<float4>	g_GeometryImage2		: register(t3);
Texture2D<uint>		g_ShadowResult			: register(t4);
Texture2D<uint>		g_TracingInfo			: register(t5);
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

// upsampling

static const int2 QuadSampleOfs[4] = {
	{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }
};

// compute mips

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeMips(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	// TODO
}

// compute lighting

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeLighting(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 imagePos = dispatchThreadId.xy;
	if (imagePos.x >= (uint)g_SceneCB.TargetSize.x || imagePos.y >= (uint)g_SceneCB.TargetSize.y)
		return;

	float clipDepth = g_GeometryDepth.Load(int3(imagePos.xy, 0));
	bool isSky = (clipDepth >= 1.0);
	float2 uvPos = (float2(imagePos) + 0.5) * g_SceneCB.TargetSize.zw;
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

		float debugWeight = 1.0; // TEMP
		float shadowPerLight[4] = { 0, 0, 0, 0 };
		float2 lightBufferPos = (float2(imagePos.xy) + 0.5) * g_SceneCB.LightBufferDownscale.y;
		uint tracingInfo = g_TracingInfo.Load(int3(lightBufferPos.xy, 0));
		uint2 tracingSamplePos = uint2(tracingInfo % (uint)g_SceneCB.LightBufferDownscale.x, tracingInfo / (uint)g_SceneCB.LightBufferDownscale.x); // sub sample used in ray tracing pass
		uint2 tracingImagePos = tracingSamplePos + floor(lightBufferPos) * g_SceneCB.LightBufferDownscale.x; // full res position used for tracing
		//debugWeight = (tracingImagePos.x == imagePos.x && tracingImagePos.y == imagePos.y ? 0.0 : 1.0);// TODO: debug tracing pos
		GBufferData tracingGBData = LoadGBufferData(tracingImagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
		float tracingConfidence = 1.0;
		tracingConfidence *= saturate(dot(tracingGBData.Normal, gbData.Normal));
		//tracingConfidence *= 1.0 - saturate(abs(tracingGBData.ClipDepth - clipDepth) / (clipDepth * 0.001));
		#if (0)
		// point upsampling
		uint shadowPerLightPacked = g_ShadowResult.Load(int3(lightBufferPos.xy, 0));
		[unroll] for (uint j = 0; j < 4; ++j)
		{
			shadowPerLight[j] = float((shadowPerLightPacked >> (j * 0x8)) & 0xff) / 255.0;
		}
		#else
		// filtered upsampling
		/*uint frameHashOfs = g_SceneCB.FrameNumber * (uint)g_SceneCB.TargetSize.x * (uint)g_SceneCB.TargetSize.y * g_SceneCB.PerFrameJitter;
		uint pixelHash = HashUInt(imagePos.x + imagePos.y * (uint)g_SceneCB.TargetSize.x + frameHashOfs);
		float2 jitter2d = BlueNoiseDiskLUT64[pixelHash % 64] * 0.5;*/
		lightBufferPos -= 0.5; // offset to neighbourhood for bilinear filtering
		float2 pf = frac(lightBufferPos);
		float sampleWeight[4] = {
			(1.0 - pf.x) * (1.0 - pf.y),
			pf.x * (1.0 - pf.y),
			(1.0 - pf.x) * pf.y,
			pf.x * pf.y
		};
		[unroll] for (uint i = 0; i < 4; ++i)
		{
			int2 lightSamplePos = int2(clamp(lightBufferPos + QuadSampleOfs[i], float2(0, 0), g_SceneCB.LightBufferSize.xy - 1));
			uint shadowPerLightPacked = g_ShadowResult.Load(int3(lightSamplePos.xy, 0));
			[unroll] for (uint j = 0; j < 4; ++j)
			{
				shadowPerLight[j] += float((shadowPerLightPacked >> (j * 0x8)) & 0xff) / 255.0 * sampleWeight[i];
			}
		}
		#endif

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
			float3 lightDir = (LightType_Directional == light.Type ? -light.Direction : normalize(light.Position.xyz - worldPos.xyz));
			float shadowFactor = shadowPerLight[ilight];
			float NoL = dot(lightDir, lightingParams.normal);
			shadowFactor *= saturate(NoL  * 10.0); // approximate self shadowing at grazing angles
			float specularOcclusion = shadowFactor;
			directLightColor += EvaluateDirectLighting(lightingParams, light, shadowFactor, specularOcclusion).xyz;
		}

		// indirect light

		float3 envDir = float3(lightingParams.normal.x, max(lightingParams.normal.y, 0.0), lightingParams.normal.z);
		float3 envColor = CalculateSkyLight(worldPos, normalize(envDir * 0.5 + WorldUp)).xyz; // simplified sky light
		float3 indirectLightColor = lightingParams.diffuseColor.xyz * envColor; // no indirect spec

		// final

		lightingResult = directLightColor + indirectLightColor;

		if (g_SceneCB.DebugVec0[0] >= 1)
		{
			lightingResult = lerp(float3(1,0,0), float3(0,1,0), debugWeight) * ComputeLuminance(indirectLightColor);
		}
	}

	g_LightingTarget[imagePos] = float4(lightingResult, 1.0);
}