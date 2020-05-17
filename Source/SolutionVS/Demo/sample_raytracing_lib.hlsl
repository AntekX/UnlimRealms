
#include "Lighting.hlsli"
#include "HDRRender/HDRRender.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

#pragma pack_matrix(row_major)

struct SceneConstants
{
	float4x4 viewProjInv;
	float4x4 viewProjPrev;
	float4 cameraPos;
	float4 viewportSize; // w, h, 1/w, 1/h
	float4 clearColor;
	uint occlusionSampleCount;
	uint denoisingEnabled;
	uint accumulationFrameCount;
	uint accumulationFrameNumber;
	AtmosphereDesc atmoDesc;
	LightingDesc lightingDesc;
};

struct Vertex
{
	float3 pos;
	float3 norm;
};

struct MeshDesc
{
	uint vertexBufferOfs;
	uint indexBufferOfs;
};

static const uint RecursionDepthMax = 4;

#define DEBUG_VIEW_DISABLED 0
#define DEBUG_VIEW_AMBIENTOCCLUSION 1
#define DEBUG_VIEW_AMBIENTOCCLUSION_SAMPLINGDIR 2
#define DEBUG_VIEW_AMBIENTOCCLUSION_CONFIDENCE 3
#define DEBUG_VIEW_MODE DEBUG_VIEW_DISABLED

ConstantBuffer<SceneConstants> g_SceneCB			: register(b0);
RaytracingAccelerationStructure g_SceneStructure	: register(t0);
//StructuredBuffer<Vertex> g_VertexBuffer: register(t1); // todo: load does not work as expected
ByteAddressBuffer g_VertexBuffer					: register(t1);
ByteAddressBuffer g_IndexBuffer						: register(t2);
ByteAddressBuffer g_MeshBuffer						: register(t3);
ByteAddressBuffer g_InstanceBuffer					: register(t4);
RWTexture2D<float4> g_TargetTexture					: register(u0);
RWTexture2D<uint2> g_OcclusionBuffer				: register(u1);
RWTexture2D<uint2> g_OcclusionBufferPrev			: register(u2);
RWTexture2D<float> g_DepthBuffer					: register(u3);
RWTexture2D<float> g_DepthBufferPrev				: register(u4);

typedef BuiltInTriangleIntersectionAttributes SampleHitAttributes;
struct SampleRayData
{
	float4 color;
	uint recursionDepth;
};
struct SampleShadowData
{
	bool occluded;
};

#define SAMPLE_LIGHTING 0

float CalculateDiffuseCoefficient(in float3 worldPos, in float3 incidentLightRay, in float3 normal)
{
	float fNDotL = saturate(dot(-incidentLightRay, normal));
	return fNDotL;
}

float CalculateSpecularCoefficient(in float3 worldPos, in float3 incidentLightRay, in float3 normal, in float specularPower)
{
	float3 reflectedLightRay = normalize(reflect(incidentLightRay, normal));
	return pow(saturate(dot(reflectedLightRay, normalize(-WorldRayDirection()))), specularPower);
}

float3 FresnelReflectanceSchlick(in float3 I, in float3 N, in float3 f0)
{
	float cosi = saturate(dot(-I, N));
	return f0 + (1 - f0)*pow(1 - cosi, 5);
}

float3 CalculatePhongLightingDirect(LightDesc light, in float3 worldPos, in float3 normal, in float3 albedo, in float shadowFactor, in float diffuseCoef = 1.0, in float specularCoef = 1.0, in float specularPower = 50)
{
	float3 incidentLightRay = light.Direction.xyz;

	// diffuse
	float3 lightDiffuseColor = (light.Color * light.Intensity) * 0.05;
	float Kd = CalculateDiffuseCoefficient(worldPos, incidentLightRay, normal);
	float3 diffuseColor = diffuseCoef * Kd * lightDiffuseColor * albedo * shadowFactor;
	
	// atmospheric scattering
	float3 atmoSurfOfs = float3(0.0, g_SceneCB.atmoDesc.InnerRadius, 0.0);
	float3 atmoWorldPos = worldPos + atmoSurfOfs;
	float3 atmoCameraPos = g_SceneCB.cameraPos.xyz + atmoSurfOfs;
	diffuseColor = AtmosphericScatteringSurface(g_SceneCB.atmoDesc, light, diffuseColor, atmoWorldPos, atmoCameraPos).xyz;

	// specular
	float3 lightSpecularColor = lightDiffuseColor;
	float Ks = CalculateSpecularCoefficient(worldPos, incidentLightRay, normal, specularPower);
	float3 specularColor = specularCoef * Ks * lightSpecularColor;// *shadowFactor;

	return diffuseColor + specularColor;
}

float3 CalculatePhongLightingIndirect(in float3 worldPos, in float3 normal, in float3 albedo, in float diffuseCoef = 1.0)
{
	// simple ambient
	float3 ambientColor = g_SceneCB.clearColor.xyz * g_SceneCB.lightingDesc.LightSources[0].Intensity * 0.005;
	ambientColor = albedo * ambientColor;

	return ambientColor;
}

float4 CalculateSkyLight(const float3 position, const float3 direction)
{
	const float SkyLightIntensity = 20;
	float height = lerp(g_SceneCB.atmoDesc.InnerRadius, g_SceneCB.atmoDesc.OuterRadius, 0.05);
	float4 color = 0.0;
	for (uint ilight = 0; ilight < g_SceneCB.lightingDesc.LightSourceCount; ++ilight)
	{
		LightDesc light = g_SceneCB.lightingDesc.LightSources[ilight];
		float3 worldFrom = position + float3(0.0, height, 0.0);
		float3 worldTo = worldFrom + direction;
		color += AtmosphericScatteringSky(g_SceneCB.atmoDesc, light, worldTo, worldFrom) * SkyLightIntensity;
	}
	color.w = min(1.0, color.w);
	return color;
}

float3 GetSampleDirection(uint seed, float3 samplePos)
{
	float xi0 = HashFloat3(Rand3D(samplePos.xyz) + seed);
	float xi1 = HashFloat3(Rand3D(samplePos.zxy) + seed);
	float3 dir = float3(
		sqrt(xi0) * cos(TwoPi * xi1),
		sqrt(xi0) * sin(TwoPi * xi1),
		sqrt(1 - xi0));
	return dir;
}

float3 InterpolatedVertexAttribute(float3 attributes[3], float3 barycentrics)
{
	return (attributes[0] * barycentrics.x + attributes[1] * barycentrics.y + attributes[2] * barycentrics.z);
}

float2 RayDispatchClipPos()
{
	uint3 dispatchSize = DispatchRaysDimensions();
	uint3 rayDispatchIdx = DispatchRaysIndex();
	float2 rayPixelPos = (float2)rayDispatchIdx.xy + 0.5;
	float2 rayUV = rayPixelPos / (float2)dispatchSize.xy;
	float2 rayClip = float2(rayUV.x, 1.0 - rayUV.y) * 2.0 - 1.0;
	return rayClip;
}

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

	float2 rayClip = RayDispatchClipPos();
	#if (0)
	// test: simple orthographic projection
	ray.Origin = float3(rayClip, 0.0);
	ray.Direction = float3(0, 0, 1);
	#else
	// calculate camera ray
	float4 rayWorld = mul(float4(rayClip.xy, 0.0, 1.0), g_SceneCB.viewProjInv);
	rayWorld.xyz /= rayWorld.w;
	ray.Origin = g_SceneCB.cameraPos.xyz;
	ray.Direction = normalize(rayWorld.xyz - ray.Origin);
	#endif

	SampleRayData rayData;
	rayData.color = 0.0;
	rayData.recursionDepth = 0;

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
	float4 targetValue = float4(0.0, 0.0, 0.0, 1.0);
	targetValue.xyz = lerp(targetValue.xyz, rayData.color.xyz, rayData.color.w);
	targetValue.w = saturate(targetValue.w + rayData.color.w);
	g_TargetTexture[dispatchIdx.xy] = targetValue;
}

[shader("miss")]
void SampleMiss(inout SampleRayData rayData)
{
	//rayData.color.xyz = g_SceneCB.clearColor.xyz;
	//rayData.color.w = 1.0;

	// calculate atmosphere light

	rayData.color = CalculateSkyLight(WorldRayOrigin(), WorldRayDirection());
}

[shader("miss")]
void SampleMissShadow(inout SampleShadowData shadowData)
{
	shadowData.occluded = false;
}

[shader("closesthit")]
void SampleClosestHit(inout SampleRayData rayData, in SampleHitAttributes attribs)
{
	#if (DEBUG_VIEW_MODE)
	float4 debugVec0 = 0.0;
	#endif

	uint2 dispatchPos = DispatchRaysIndex().xy;
	float3 baryCoords = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float4 debugColor = float4(baryCoords.xyz, 1.0);
	float3 hitWorldPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

	// read instance transformation

	static const uint InstanceStride = 64; // sizeof GrafAccelerationStructureInstance
	uint instanceIdx = InstanceIndex();
	uint instanceOfs = instanceIdx * InstanceStride;
	float3x4 imx;
	imx[0] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 0));
	imx[1] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 16));
	imx[2] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 32));
	// transpose to unscale
	float3x3 instanceFrame = float3x3(
		float3(imx[0][0], imx[1][0], imx[2][0]),
		float3(imx[0][1], imx[1][1], imx[2][1]),
		float3(imx[0][2], imx[1][2], imx[2][2])
	);
	instanceFrame[0] = normalize(instanceFrame[0]);
	instanceFrame[1] = normalize(instanceFrame[1]);
	instanceFrame[2] = normalize(instanceFrame[2]);

	// read vertex attributes

	const uint meshStride = 8; // sizeof MeshDesc
	const uint vertexStride = 24; // arbitrary vertex format can be read from mesh buffer
	const uint vertexNormOfs = 12;

	uint meshID = InstanceID();
	MeshDesc meshDesc;
	meshDesc.vertexBufferOfs = g_MeshBuffer.Load(meshID * meshStride + 0);
	meshDesc.indexBufferOfs = g_MeshBuffer.Load(meshID * meshStride + 4);
	uint indexOffset = meshDesc.indexBufferOfs + PrimitiveIndex() * 3 * 4;
	uint3 indices = g_IndexBuffer.Load3(indexOffset);
	float3 vnormal[3];
	[unroll] for (int i = 0; i < 3; ++i)
	{
		uint vertexOfs = meshDesc.vertexBufferOfs + indices[i] * vertexStride + vertexNormOfs;
		vnormal[i] = asfloat(g_VertexBuffer.Load3(vertexOfs));
	}
	float3 normal = /*normalize*/(InterpolatedVertexAttribute(vnormal, baryCoords));
	float3 bitangent = float3(0.0, 0.0, 1.0);
	if (normal.z + Eps >= 1.0) bitangent = float3(0.0,-1.0, 0.0);
	if (normal.z - Eps <=-1.0) bitangent = float3(0.0, 1.0, 0.0);
	float3 tangent = normalize(cross(bitangent, normal));
	bitangent = cross(normal, tangent);
	float3x3 surfaceTBN = {
		tangent,
		bitangent,
		normal
	};
	surfaceTBN = mul(surfaceTBN, instanceFrame);
	surfaceTBN *= -sign(dot(surfaceTBN[2], WorldRayDirection())); // back face TBN
	normal = surfaceTBN[2].xyz;

	// material params

	float3 albedo = 0.75;// debugColor.xyz;
	float diffuseCoef = 1.0;
	float specularCoef = 0.5;
	float specularPower = 50.0;
	float reflectanceCoef = 0.5;

	MaterialInputs material = (MaterialInputs)0;
	initMaterial(material);
	material.baseColor.xyz = 1.0;
	material.normal = normal;
	material.roughness = 1.0;
	material.metallic = 0.0;
	material.reflectance = 0.0;

	// mirror
	#if (0)
	if (2 == meshID)
	{
		//material.baseColor.xyz = float3(0.25, 0.24, 0.22);
		material.baseColor.xyz = float3(0.98, 0.84, 0.72);
		material.metallic = 1.0;
		material.roughness = 0.2;
		material.reflectance = 1.0;
	}
	#endif

	// coated plastic
	#if (1)
	if (2 == meshID)
	{
		material.baseColor.xyz = float3(0.98, 0.84, 0.72);
		material.metallic = 0.0;
		material.roughness = 0.0;
		material.reflectance = 1.0;
		material.hasClearCoat = false;
		material.clearCoat = 1.0;
		material.clearCoatRoughness = 0.1;
		material.clearCoatNormal = normal;
	}
	#endif

	// copper
	#if (0)
	material.baseColor.xyz = float3(0.98, 0.84, 0.72);
	material.metallic = 1.0;
	material.roughness = 0.4;
	material.reflectance = 0.0;
	#endif

	// wood
	#if (0)
	material.baseColor.xyz = float3(0.53, 0.36, 0.24);
	material.metallic = 0.0;
	material.roughness = 1.0;
	material.reflectance = 0.0;
	#endif

	// evaluate ambient occlusion

	float occlusionFactor = 1.0;
	uint dispatchOcclusionEnabled = 1;// (dispatchPos.x + dispatchPos.y) % 2 | !g_SceneCB.denoisingEnabled; // calculate occlusion in checkerboard
	if (g_SceneCB.occlusionSampleCount > 0 && dispatchOcclusionEnabled && rayData.recursionDepth <= 0)
	{
		uint instanceInclusionMask = 0xff;
		uint rayContributionToHitGroupIndex = 0;
		uint multiplierForGeometryContributionToShaderIndex = 1;
		uint missShaderIndex = 1; // SampleMissShadow

		SampleShadowData occlusionData;
		RayDesc ray;
		ray.TMin = 0.001;
		ray.TMax = 1.0e+4;
		ray.Origin = hitWorldPos;

		float visibility = 0.0;
		for (uint isample = 0; isample < g_SceneCB.occlusionSampleCount; ++isample)
		{
			// trace occlusion data

			uint sampleSeed = isample + ((g_SceneCB.occlusionSampleCount * g_SceneCB.accumulationFrameNumber) & 0xffff);
			ray.Direction = mul(GetSampleDirection(sampleSeed, /*hitWorldPos*/float3(dispatchPos.xy, 0)), surfaceTBN);
			occlusionData.occluded = true;
			#if (DEBUG_VIEW_AMBIENTOCCLUSION_SAMPLINGDIR == DEBUG_VIEW_MODE)
			debugVec0.xyz += ray.Direction;
			#endif

			TraceRay(g_SceneStructure,
				RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
				RAY_FLAG_FORCE_OPAQUE | // skip any hit shaders
				RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // skip closets hit shaders (only miss shader writes to shadow payload)
				instanceInclusionMask,
				rayContributionToHitGroupIndex,
				multiplierForGeometryContributionToShaderIndex,
				missShaderIndex,
				ray, occlusionData);

			visibility += (occlusionData.occluded ? 0.0 : 1.0);
		}
		occlusionFactor = visibility / g_SceneCB.occlusionSampleCount;
		#if (DEBUG_VIEW_AMBIENTOCCLUSION_SAMPLINGDIR == DEBUG_VIEW_MODE)
		debugVec0.xyz = normalize(debugVec0.xyz);
		#endif

		// denosing filter

		if (g_SceneCB.denoisingEnabled && rayData.recursionDepth <= 0)
		{
			float depthCrnt = RayTCurrent();
			float accumulatedOcclusion = occlusionFactor;
			uint counter = 1;
			uint2 packedData = 0;
			float4 clipPosPrev = mul(float4(hitWorldPos, 1.0), g_SceneCB.viewProjPrev);
			clipPosPrev.xy /= clipPosPrev.w;
			if (all(abs(clipPosPrev.xy) < 1.0) && g_SceneCB.accumulationFrameNumber > 0)
			{
				uint2 dispatchPosPrev = uint2((clipPosPrev.xy * float2(1.0, -1.0) + 1.0) * 0.5 * DispatchRaysDimensions().xy);
				float depthPrev = g_DepthBufferPrev[dispatchPosPrev];
				float accumulationConfidence = 1.0 - saturate(abs(depthPrev - depthCrnt) / (min(depthCrnt, depthPrev) * 0.08));
				packedData = g_OcclusionBufferPrev[dispatchPosPrev];
				accumulatedOcclusion = float(packedData.x) / 0xffff;
				counter = min(uint(float(packedData.y) * (accumulationConfidence > 0.33 ? 1.0 : accumulationConfidence / 0.33 * 0)) + 1, g_SceneCB.accumulationFrameCount);
				accumulatedOcclusion = (accumulatedOcclusion * (counter - 1) / counter) + (occlusionFactor / counter);
				//accumulatedOcclusion = lerp(1.0, accumulatedOcclusion, saturate(float(counter) / 2));
				occlusionFactor = accumulatedOcclusion;
				#if (DEBUG_VIEW_AMBIENTOCCLUSION_CONFIDENCE == DEBUG_VIEW_MODE)
				debugVec0.x = accumulationConfidence;
				#endif
			}
			packedData.x = uint(accumulatedOcclusion * 0xffff);
			packedData.y = counter;
			g_OcclusionBuffer[dispatchPos] = packedData;
			g_DepthBuffer[dispatchPos] = depthCrnt;
			if (0 == g_SceneCB.accumulationFrameNumber)
			{
				// first time clear
				g_OcclusionBufferPrev[dispatchPos] = uint2(0, 0);
				g_DepthBufferPrev[dispatchPos] = 0;
			}
		}
	}
	else if (g_SceneCB.occlusionSampleCount > 0 && !dispatchOcclusionEnabled && rayData.recursionDepth == 0)
	{
		// deinterleave occlusion data from previous buffer

		float4 clipPosPrev = mul(float4(hitWorldPos, 1.0), g_SceneCB.viewProjPrev);
		clipPosPrev.xy /= clipPosPrev.w;
		if (all(abs(clipPosPrev.xy) < 1.0) && g_SceneCB.accumulationFrameNumber > 0)
		{
			uint2 dispatchMax = DispatchRaysDimensions().xy - 1;
			uint2 dispatchPosPrev = uint2((clipPosPrev.xy * float2(1.0, -1.0) + 1.0) * 0.5 * DispatchRaysDimensions().xy);
			static const int2 ofs[4] = { int2(-1,0), int2(0,-1), int2(1,0), int2(0,1) };
			occlusionFactor = 0.0;
			for (uint i = 0; i < 4; ++i)
			{
				uint2 samplePos = uint2(clamp(int2(dispatchPos)+ofs[i], int2(0, 0), int2(dispatchMax)));
				occlusionFactor += float(g_OcclusionBufferPrev[samplePos].x) / 0xffff;
			}
			occlusionFactor *= 0.25;
		}
	}

	// trace reflection data

	float3 reflectionColor = 0.0;
	float3 reflectionDir = reflect(WorldRayDirection(), normal);
	bool reflective = (material.reflectance > Eps || material.metallic > Eps);
	if (rayData.recursionDepth < RecursionDepthMax && reflective)
	{
		SampleRayData reflectionData;
		reflectionData.color = 0;
		reflectionData.recursionDepth = rayData.recursionDepth + 1;

		RayDesc ray;
		ray.TMin = 0.001;
		ray.TMax = 1.0e+4;
		ray.Origin = hitWorldPos;
		ray.Direction = reflectionDir;

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
			ray, reflectionData);

		reflectionColor = reflectionData.color.xyz;
	}
	else
	{
		// fallback to env color reflection
		reflectionColor = CalculateSkyLight(hitWorldPos, float3(reflectionDir.x, abs(reflectionDir.y), reflectionDir.z)).xyz;
	}

	// pre-calculate lighting paramns for given material and ray

	LightingParams lightingParams;
	getLightingParams(hitWorldPos, WorldRayOrigin(), material, lightingParams);

	// calculate direct light

	float3 directLightColor = 0;
	for (uint ilight = 0; ilight < g_SceneCB.lightingDesc.LightSourceCount; ++ilight)
	{
		// trace shadow data

		LightDesc light = g_SceneCB.lightingDesc.LightSources[ilight];

		SampleShadowData shadowData;
		shadowData.occluded = true;

		RayDesc ray;
		ray.TMin = 0.001;
		ray.TMax = 1.0e+4;
		ray.Origin = hitWorldPos;
		ray.Direction = -light.Direction;

		uint instanceInclusionMask = 0xff;
		uint rayContributionToHitGroupIndex = 0;
		uint multiplierForGeometryContributionToShaderIndex = 1;
		uint missShaderIndex = 1; // SampleMissShadow
		TraceRay(g_SceneStructure,
			RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
			RAY_FLAG_FORCE_OPAQUE | // skip any hit shaders
			RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // skip closets hit shaders (only miss shader writes to shadow payload)
			instanceInclusionMask,
			rayContributionToHitGroupIndex,
			multiplierForGeometryContributionToShaderIndex,
			missShaderIndex,
			ray, shadowData);

		float shadowFactor = (shadowData.occluded ? 0.0 : 1.0);
		float specularOcclusion = shadowFactor * occlusionFactor;

		#if (SAMPLE_LIGHTING)
		directLightColor += CalculatePhongLightingDirect(light, hitWorldPos, normal, albedo, shadowFactor, diffuseCoef, specularCoef, specularPower);
		#else
		#if (1)
		// calculate incoming sun light with respect to atmospheric scattering
		light.Color.xyz = CalculateSkyLight(hitWorldPos, -light.Direction).xyz;
		light.Intensity = min(1.0, light.Intensity / ComputeLuminance(light.Color.xyz));
		#endif
		directLightColor += EvaluateDirectLighting(lightingParams, light, shadowFactor, specularOcclusion).xyz;
		#endif
	}

	// final radiance

	#if (SAMPLE_LIGHTING)
	float3 lightColor = directLightColor + CalculatePhongLightingIndirect(hitWorldPos, normal, albedo, diffuseCoef);
	float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), normal, albedo.xyz);
	lightColor.xyz += reflectionColor * fresnelR * reflectanceCoef;
	#else
	float3 envColor = CalculateSkyLight(hitWorldPos, normalize(lightingParams.normal * 0.5 + WorldUp)).xyz;
	float3 indirectLightColor = lightingParams.diffuseColor.xyz * envColor * occlusionFactor; // temp ambient, no indirect spec
	float3 reflectedLightColor = reflectionColor * FresnelReflectance(lightingParams);
	float3 lightColor = directLightColor + indirectLightColor + reflectedLightColor;
	#endif

	rayData.color = float4(lightColor.xyz, 1.0);

	// debug output overrides
	#if (DEBUG_VIEW_AMBIENTOCCLUSION == DEBUG_VIEW_MODE)
	rayData.color = float4(occlusionFactor.xxx * g_SceneCB.lightingDesc.LightSources[0].Intensity, 1.0);
	#elif (DEBUG_VIEW_AMBIENTOCCLUSION_SAMPLINGDIR == DEBUG_VIEW_MODE)
	rayData.color = float4(saturate(debugVec0.xyz) * g_SceneCB.lightingDesc.LightSources[0].Intensity, 1.0);
	#elif (DEBUG_VIEW_AMBIENTOCCLUSION_CONFIDENCE == DEBUG_VIEW_MODE)
	rayData.color = float4(lerp(float3(1, 0, 0), float3(0, 1, 0), debugVec0.x) * g_SceneCB.lightingDesc.LightSources[0].Intensity, 1.0);
	#endif
}