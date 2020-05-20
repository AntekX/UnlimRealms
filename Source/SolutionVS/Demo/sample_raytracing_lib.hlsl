
#include "Lighting.hlsli"
#include "HDRRender/HDRRender.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

#pragma pack_matrix(row_major)

struct SceneConstants
{
	float4x4 viewProjInv;
	float4x4 viewProjPrev;
	float4 cameraPos;
	float4 cameraDir;
	float4 viewportSize; // w, h, 1/w, 1/h
	float4 clearColor;
	uint occlusionPassSeparate;
	uint occlusionSampleCount;
	uint denoisingEnabled;
	uint accumulationFrameCount;
	uint accumulationFrameNumber;
	uint3 __pad0;
	float4 debugVec0;
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

struct AORayData
{
	float occlusion;
	float hitDist;
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

float3x3 GetCurrentInstanceFrame()
{
	#if (0)
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
	#else
	float3x3 instanceFrame = instanceFrame = (float3x3)ObjectToWorld4x3();
	#endif

	// normalize (uniform scale is expected)
	float scaleInv = 1.0 / length(instanceFrame[0]);
	instanceFrame[0] = instanceFrame[0] * scaleInv;
	instanceFrame[1] = instanceFrame[1] * scaleInv;
	instanceFrame[2] = instanceFrame[2] * scaleInv;

	return instanceFrame;
}

MeshDesc GetCurrentMeshDesc()
{
	const uint meshStride = 8; // sizeof MeshDesc
	const uint meshID = InstanceID();

	MeshDesc meshDesc;
	meshDesc.vertexBufferOfs = g_MeshBuffer.Load(meshID * meshStride + 0);
	meshDesc.indexBufferOfs = g_MeshBuffer.Load(meshID * meshStride + 4);
	
	return meshDesc;
}

float3x3 GetMeshSurfaceTBN(const MeshDesc meshDesc, const float3x3 instanceFrame, const float3 baryCoords)
{
	const uint vertexStride = 24; // arbitrary vertex format can be read from mesh buffer
	const uint vertexNormOfs = 12;

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
	if (normal.z + Eps >= 1.0) bitangent = float3(0.0, -1.0, 0.0);
	if (normal.z - Eps <= -1.0) bitangent = float3(0.0, 1.0, 0.0);
	float3 tangent = normalize(cross(bitangent, normal));
	bitangent = cross(normal, tangent);
	float3x3 surfaceTBN = {
		tangent,
		bitangent,
		normal
	};
	surfaceTBN = mul(surfaceTBN, instanceFrame);
	if (HIT_KIND_TRIANGLE_BACK_FACE == HitKind()) surfaceTBN *= -1; // back face TBN

	return surfaceTBN;
}

[shader("raygeneration")]
void SampleRaygen()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint3 dispatchSize = DispatchRaysDimensions();

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
void ShadowMiss(inout SampleShadowData shadowData)
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

	// read mesh data

	const uint meshID = InstanceID();
	const MeshDesc meshDesc = GetCurrentMeshDesc();
	const float3x3 instanceFrame = GetCurrentInstanceFrame();
	const float3x3 surfaceTBN = GetMeshSurfaceTBN(meshDesc, instanceFrame, baryCoords);
	const float3 normal = surfaceTBN[2].xyz;

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
	if (g_SceneCB.occlusionSampleCount > 0 && rayData.recursionDepth <= 0)
	{
		if (!g_SceneCB.occlusionPassSeparate)
		{
			uint instanceInclusionMask = 0xff;
			uint rayContributionToHitGroupIndex = 0;
			uint multiplierForGeometryContributionToShaderIndex = 1;
			uint missShaderIndex = 1; // ShadowMiss

			SampleShadowData occlusionData;
			RayDesc ray;
			ray.TMin = 0.001;
			ray.TMax = 1.0e+4;
			ray.Origin = hitWorldPos;

			float visibility = 0.0;
			for (uint isample = 0; isample < g_SceneCB.occlusionSampleCount; ++isample)
			{
				// trace occlusion data

				uint accumulationFrame = g_SceneCB.accumulationFrameNumber % g_SceneCB.accumulationFrameCount;
				uint sampleSeed = isample + ((g_SceneCB.occlusionSampleCount * accumulationFrame) & 0xffff);
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
					accumulationConfidence = (accumulationConfidence > 0.25 ? 1.0 : 0.0);
					packedData = g_OcclusionBufferPrev[dispatchPosPrev];
					accumulatedOcclusion = float(packedData.x) / 0xffff;
					counter = min(uint(float(packedData.y) * accumulationConfidence) + 1, g_SceneCB.accumulationFrameCount);
					accumulatedOcclusion = (accumulatedOcclusion * (counter - 1) / counter) + (occlusionFactor / counter);
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
		else
		{
			// read precomputed occlusion result

			#if (0)

			static const uint resolutionDenom = 4;
			#if (0)
			uint2 samplePos = dispatchPos / resolutionDenom;
			occlusionFactor = float(g_OcclusionBuffer[samplePos].x) / 0xffff;
			#else
			#if (0)
			static const uint kernelSampleCount = 25;
			static const int2 kernelOfs[kernelSampleCount] = {
				int2(-2,-2), int2(-1,-2), int2( 0,-2), int2( 1,-2), int2( 2,-2),
				int2(-2,-1), int2(-1,-1), int2( 0,-1), int2( 1,-1), int2( 2,-1),
				int2(-2, 0), int2(-1, 0), int2( 0, 0), int2( 1, 0), int2( 2, 0),
				int2(-2, 1), int2(-1, 1), int2( 0, 1), int2( 1, 1), int2( 2, 1),
				int2(-2, 2), int2(-1, 2), int2( 0, 2), int2( 1, 2), int2( 2, 2)
			};
			static const float kernelWeight[kernelSampleCount] = {
				0.023528, 0.033969, 0.038393, 0.033969, 0.023528,
				0.033969, 0.049045, 0.055432, 0.049045, 0.033969,
				0.038393, 0.055432, 0.062651, 0.055432, 0.038393,
				0.033969, 0.049045, 0.055432, 0.049045, 0.033969,
				0.023528, 0.033969, 0.038393, 0.033969, 0.023528
			};
			#else
			static const uint kernelSampleCount = 9;
			static const int2 kernelOfs[kernelSampleCount] = {
				int2(-1,-1), int2(0,-1), int2(1,-1),
				int2(-1, 0), int2(0, 0), int2(1, 0),
				int2(-1, 1), int2(0, 1), int2(1, 1),
			};
			static const float kernelWeight[kernelSampleCount] = {
				0.095332, 0.118095, 0.095332,
				0.118095, 0.146293, 0.118095,
				0.095332, 0.118095, 0.095332
			};
			#endif
			occlusionFactor = 0.0;
			int2 bufferSize = int2(DispatchRaysDimensions().xy / resolutionDenom);
			int2 samplePosCenter = int2(dispatchPos / resolutionDenom);
			float weightSum = 0.0;
			for (uint k = 0; k < kernelSampleCount; ++k)
			{
				int2 samplePos = samplePosCenter + kernelOfs[k];
				if (samplePos.x < 0 || samplePos.y < 0 || samplePos.x >= bufferSize.x || samplePos.y >= bufferSize.y)
					continue;
				occlusionFactor += float(g_OcclusionBuffer[samplePos + kernelOfs[k]].x) / 0xffff * kernelWeight[k];
				weightSum += kernelWeight[k];
			}
			occlusionFactor /= weightSum;
			#endif

			#else

			// dinterleave precomputed data

			int2 samplePos = int2(dispatchPos.x / 2, dispatchPos.y); // chekerboard data sample pos
			uint samplePrecomputed = (dispatchPos.x + dispatchPos.y + 1) % 2; // check whether sample has precomputed data or requires deinterleaving
			if (samplePrecomputed > 0)
			{
				occlusionFactor = float(g_OcclusionBuffer[samplePos].x) / 0xffff;
			}
			else
			{
				static const int2 ofs[4] = { int2(0,-1), int2(0, 0), int2(1, 0), int2(0, 1) };
				int2 srcBufferSize = int2(DispatchRaysDimensions().x / 2, DispatchRaysDimensions().y);
				float depthCrnt = RayTCurrent();
				occlusionFactor = 0.0;
				float blendWeightSum = 0.0;
				[unroll] for (uint i = 0; i < 4; ++i)
				{
					int2 samplePosCrnt = samplePos + ofs[i];
					if (samplePosCrnt.x < 0 || samplePosCrnt.y < 0 || samplePosCrnt.x >= srcBufferSize.x || samplePosCrnt.y >= srcBufferSize.y)
						continue;
					float sampleDepth = g_DepthBuffer[samplePosCrnt];
					if (abs(sampleDepth - depthCrnt) > depthCrnt * 0.02)
						continue;
					float sampleOcclusion = float(g_OcclusionBuffer[samplePosCrnt].x) / 0xffff;
					float sampleWeight = 1.0;
					occlusionFactor += sampleOcclusion * sampleWeight;
					blendWeightSum += sampleWeight;
				}
				occlusionFactor = saturate(occlusionFactor / (blendWeightSum + Eps));
			}

			#endif
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
	float3 reflectedLightColor = reflectionColor * FresnelReflectance(lightingParams) * saturate(occlusionFactor + 0.2); // x occlusionFactor because AO evaluation is disabled in reflection tracing
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

[shader("raygeneration")]
void AORaygen()
{
	uint2 dispatchPos = DispatchRaysIndex().xy;
	uint2 dispatchSize = DispatchRaysDimensions().xy;

	// calculate camera space ray

	float2 rayPixelPos = float2(dispatchPos.x * 2 + (dispatchPos.y % 2), dispatchPos.y) + 0.5; // checkerboard position
	float2 rayUV = rayPixelPos * g_SceneCB.viewportSize.zw;
	float2 rayClip = float2(rayUV.x, 1.0 - rayUV.y) * 2.0 - 1.0;
	float4 rayWorld = mul(float4(rayClip.xy, 0.0, 1.0), g_SceneCB.viewProjInv);
	rayWorld.xyz /= rayWorld.w;
	RayDesc ray;
	ray.TMin = 0.001;
	ray.TMax = 1.0e+4;
	ray.Origin = g_SceneCB.cameraPos.xyz;
	ray.Direction = normalize(rayWorld.xyz - ray.Origin);

	// ray trace

	AORayData rayData;
	rayData.occlusion = 1.0;
	rayData.hitDist = ray.TMax;

	uint instanceInclusionMask = 0xff;
	uint rayContributionToHitGroupIndex = 0;
	uint multiplierForGeometryContributionToShaderIndex = 1;
	uint missShaderIndex = 0; // AOMiss
	TraceRay(g_SceneStructure,
		RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		instanceInclusionMask,
		rayContributionToHitGroupIndex,
		multiplierForGeometryContributionToShaderIndex,
		missShaderIndex,
		ray, rayData);

	// denosing filter
	
	float occlusionFactor = rayData.occlusion;
	uint occlusionCounter = 1;
	if (g_SceneCB.denoisingEnabled)
	{
		float depthCrnt = rayData.hitDist;
		float3 hitWorldPos = ray.Origin + ray.Direction * depthCrnt;
		float accumulatedOcclusion = occlusionFactor;
		uint counter = 1;
		float4 clipPosPrev = mul(float4(hitWorldPos, 1.0), g_SceneCB.viewProjPrev);
		clipPosPrev.xy /= clipPosPrev.w;
		if (all(abs(clipPosPrev.xy) < 1.0) && g_SceneCB.accumulationFrameNumber > 0)
		{
			uint2 dispatchPosPrev = uint2((clipPosPrev.xy * float2(1.0, -1.0) + 1.0) * 0.5 * g_SceneCB.viewportSize.xy);
			dispatchPosPrev.x /= 2;
			float depthPrev = g_DepthBufferPrev[dispatchPosPrev];
			float accumulationConfidence = 1.0 - saturate(abs(depthPrev - depthCrnt) / (min(depthCrnt, depthPrev) * 0.08));
			accumulationConfidence = (accumulationConfidence > 0.25 ? 1.0 : 0.0);
			uint2 packedData = g_OcclusionBufferPrev[dispatchPosPrev];
			accumulatedOcclusion = float(packedData.x) / 0xffff;
			counter = min(uint(float(packedData.y) * accumulationConfidence) + 1, g_SceneCB.accumulationFrameCount);
			accumulatedOcclusion = (accumulatedOcclusion * (counter - 1) / counter) + (occlusionFactor / counter);
			occlusionFactor = accumulatedOcclusion;
			occlusionCounter = counter;
		}
		if (0 == g_SceneCB.accumulationFrameNumber)
		{
			// first time clear
			g_OcclusionBufferPrev[dispatchPos] = uint2(0, 0);
			g_DepthBufferPrev[dispatchPos] = 0;
		}
	}

	// write result
	
	uint2 packedData;
	packedData.x = uint(occlusionFactor * 0xffff);
	packedData.y = occlusionCounter;
	g_OcclusionBuffer[dispatchPos] = packedData;
	g_DepthBuffer[dispatchPos] = rayData.hitDist;
}

[shader("miss")]
void AOMiss(inout AORayData rayData)
{
	rayData.occlusion = 1.0;
}

[shader("closesthit")]
void AOClosestHit(inout AORayData rayDataInout, in SampleHitAttributes attribs)
{
	uint2 dispatchPos = DispatchRaysIndex().xy;
	float2 dispatchPixelPos = float2(dispatchPos.x * 2 + (dispatchPos.y % 2), dispatchPos.y) + 0.5; // checkerboard position
	float3 hitWorldPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
	float3 baryCoords = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);

	// read mesh data

	const uint meshID = InstanceID();
	const MeshDesc meshDesc = GetCurrentMeshDesc();
	const float3x3 instanceFrame = GetCurrentInstanceFrame();
	const float3x3 surfaceTBN = GetMeshSurfaceTBN(meshDesc, instanceFrame, baryCoords);
	const float3 normal = surfaceTBN[2].xyz;

	// trace occlusion samples

	RayDesc ray;
	ray.TMin = 0.001;
	ray.TMax = 1.0e+4;
	ray.Origin = hitWorldPos;

	AORayData rayData;
	rayData.occlusion = 1.0;
	rayData.hitDist = ray.TMax;

	uint instanceInclusionMask = 0xff;
	uint rayContributionToHitGroupIndex = 0;
	uint multiplierForGeometryContributionToShaderIndex = 1;
	uint missShaderIndex = 0; // AOMiss

	float occlusion = 0.0;
	for (uint isample = 0; isample < g_SceneCB.occlusionSampleCount; ++isample)
	{
		uint accumulationFrame = g_SceneCB.accumulationFrameNumber % g_SceneCB.accumulationFrameCount;
		uint sampleSeed = isample + ((g_SceneCB.occlusionSampleCount * accumulationFrame) & 0xffff);
		ray.Direction = mul(GetSampleDirection(sampleSeed, /*hitWorldPos*/float3(dispatchPixelPos.xy, 0)), surfaceTBN);
		rayData.occlusion = 0.0;

		TraceRay(g_SceneStructure,
			RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
			RAY_FLAG_FORCE_OPAQUE | // skip any hit shaders
			RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // skip closets hit shaders (only miss shader writes to occlusion payload)
			instanceInclusionMask,
			rayContributionToHitGroupIndex,
			multiplierForGeometryContributionToShaderIndex,
			missShaderIndex,
			ray, rayData);

		occlusion += rayData.occlusion;
	}
	occlusion = occlusion / g_SceneCB.occlusionSampleCount;

	// output

	rayDataInout.occlusion = occlusion;
	rayDataInout.hitDist = RayTCurrent();
}

[shader("compute")]
[numthreads(8, 8, 1)]
void BlurOcclusion(const uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 bufferSize = uint2(g_SceneCB.viewportSize.x / 2, g_SceneCB.viewportSize.y);
	uint2 bufferPos = dispatchThreadId.xy;
	if (bufferPos.x >= bufferSize.x || bufferPos.y >= bufferSize.y)
		return;

	// normal distrubution, sigma = 1.5
	static const uint kernelSampleCount = 9;
	static const float kernelWeight[kernelSampleCount] = {
		0.095332, 0.118095, 0.095332,
		0.118095, 0.146293, 0.118095,
		0.095332, 0.118095, 0.095332
	};
	#if (0)
	static const int2 kernelOfs[kernelSampleCount] = {
		int2(-1,-1), int2(0,-1), int2(1,-1),
		int2(-1, 0), int2(0, 0), int2(1, 0),
		int2(-1, 1), int2(0, 1), int2(1, 1),
	};
	#else
	// checkerboard mode (gather "diamond" samples)
	// 0 - 1 - 2 - 3 - 4
	// - 0 - 1 - 2 - 3 -
	// 0 - 1 - 2 - 3 - 4
	// - 0 - 1 - 2 - 3 -
	// 0 - 1 - 2 - 3 - 4
	static const int2 kernelOfs[kernelSampleCount] = {
		int2( 0,-2), int2(-1,-1), int2( 0,-1),
		int2(-1, 0), int2( 0, 0), int2( 1, 0),
		int2(-1, 1), int2( 0, 1), int2( 0, 2)
	};
	#endif
	float occlusion = 0.0;
	float weightSum = 0.0;
	float bufferDepth = g_DepthBuffer[bufferPos];
	float depthThreshold = bufferDepth * 0.02;
	for (uint k = 0; k < kernelSampleCount; ++k)
	{
		int2 samplePos = bufferPos + kernelOfs[k];
		if (samplePos.x < 0 || samplePos.y < 0 || samplePos.x >= bufferSize.x || samplePos.y >= bufferSize.y)
			continue;
		float sampleDepth = g_DepthBuffer[samplePos];
		if (abs(sampleDepth - bufferDepth) > depthThreshold)
			continue;
		occlusion += float(g_OcclusionBuffer[samplePos].x) / 0xffff * kernelWeight[k];
		weightSum += kernelWeight[k];
	}
	occlusion /= weightSum;

	uint2 packedData = g_OcclusionBuffer[bufferPos];
	packedData.x = uint(occlusion * 0xffff);
	g_OcclusionBuffer[bufferPos] = packedData;
}