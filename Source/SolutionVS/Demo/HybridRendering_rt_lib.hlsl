
#include "HybridRendering.hlsli"

// common functions

float3 GetDiskSampleDirection(uint sampleId, uint sampleCount)
{
	#if (1)
	float2 dir2d = BlueNoiseDiskLUT64[sampleId % 64]; // [-1, 1]
	return float3(dir2d.xy, 1.0);
	#else
	float2 p2d = Hammersley(sampleId % sampleCount, sampleCount);
	float3 dir = HemisphereSampleUniform(p2d.x, p2d.y);
	return dir.xyz;
	#endif
}

float3 GetHemisphereSampleDirection(uint sampleId, uint sampleCount)
{
	#if (0)
	float2 p2d = Hammersley(sampleId % sampleCount + 1, sampleCount + 1);
	float3 dir = HemisphereSampleCosine(p2d.x, p2d.y);
	#elif (0)
	float2 p2d = Halton(sampleId % sampleCount);
	float3 dir = HemisphereSampleCosine(p2d.x, p2d.y);
	#elif (1)
	float2 p2d = BlueNoiseDiskLUT64[sampleId % 64]; // [-1, 1]
	float3 dir = float3(p2d.x, p2d.y, sqrt(1.0 - dot(p2d.xy, p2d.xy)));
	//p2d.xy = (p2d.xy + 1.0) * 0.5;
	//float3 dir = HemisphereSampleCosine(p2d.x, p2d.y);
	#endif
	return dir.xyz;
}

float3x3 ComputeLightSamplingBasis(const float3 worldPos, const LightDesc lightDesc)
{
	float3 dirToLight = -lightDesc.Direction.xyz;
	float halfAngleTangent = lightDesc.Size; // tangent of the visible disk half angle (to transform [-1,1] disk samples)
	[flatten] if (LightType_Spherical == lightDesc.Type)
	{
		dirToLight = lightDesc.Position.xyz - worldPos.xyz;
		float dist = length(dirToLight);
		dirToLight /= dist;
		halfAngleTangent = lightDesc.Size / dist;
	}
	float3x3 lightTBN;
	lightTBN[2] = dirToLight;
	lightTBN[0] = float3(1, 0, 0);
	lightTBN[1] = normalize(cross(lightTBN[2], lightTBN[0]));
	lightTBN[0] = cross(lightTBN[1], lightTBN[2]);
	lightTBN[0] *= halfAngleTangent; 
	lightTBN[1] *= halfAngleTangent;

	return lightTBN;
}

float3x3 ComputeSamplingBasis(const float3 direction)
{
	float3x3 sampleTBN;
	sampleTBN[2] = direction;
	sampleTBN[0] = float3(1, 0, 0);
	sampleTBN[1] = normalize(cross(sampleTBN[2], sampleTBN[0]));
	sampleTBN[0] = cross(sampleTBN[1], sampleTBN[2]);
	return sampleTBN;
}

float2 InterpolatedVertexAttribute2(float2 attributes[3], float3 barycentrics)
{
	return (attributes[0] * barycentrics.x + attributes[1] * barycentrics.y + attributes[2] * barycentrics.z);
}

float3 InterpolatedVertexAttribute3(float3 attributes[3], float3 barycentrics)
{
	return (attributes[0] * barycentrics.x + attributes[1] * barycentrics.y + attributes[2] * barycentrics.z);
	//return attributes[0] + (attributes[1] - attributes[0]) * barycentrics[1] + (attributes[2] - attributes[0]) * barycentrics[2];
}

float3 WorldRayHitPoint()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

MaterialInputs GetMeshMaterialAtRayHitPoint(const BuiltInTriangleIntersectionAttributes attribs)
{
	MaterialInputs material = (MaterialInputs)0;

	// get hit mesh desc
	const uint subMeshIdx = InstanceID(); // sub mesh index
	const uint subMeshBufferOfs = subMeshIdx * SubMeshDescSize;
	SubMeshDesc subMeshDesc = (SubMeshDesc)0;
	subMeshDesc.VertexBufferDescriptor = g_MeshDescBuffer.Load(subMeshBufferOfs + 0);
	subMeshDesc.IndexBufferDescriptor = g_MeshDescBuffer.Load(subMeshBufferOfs + 4);
	subMeshDesc.PrimitivesOffset = g_MeshDescBuffer.Load(subMeshBufferOfs + 8);
	subMeshDesc.ColorMapDescriptor = g_MeshDescBuffer.Load(subMeshBufferOfs + 12);
	subMeshDesc.NormalMapDescriptor = g_MeshDescBuffer.Load(subMeshBufferOfs + 16);
	subMeshDesc.MaskMapDescriptor = g_MeshDescBuffer.Load(subMeshBufferOfs + 20);
	subMeshDesc.MaterialBufferIndex = g_MeshDescBuffer.Load(subMeshBufferOfs + 24);

	// instance transformation
	float3x3 instanceFrame = (float3x3)ObjectToWorld4x3();
	#if (0)
	float scaleInv = 1.0 / length(instanceFrame[0]); // unscale
	instanceFrame[0] = instanceFrame[0] * scaleInv;
	instanceFrame[1] = instanceFrame[1] * scaleInv;
	instanceFrame[2] = instanceFrame[2] * scaleInv;
	#endif

	// load hit mesh material desc
	const uint materialBufferOfs = subMeshDesc.MaterialBufferIndex * MeshMaterialDescSize;
	MeshMaterialDesc materialDesc = (MeshMaterialDesc)0;
	materialDesc.BaseColor = asfloat(g_MaterialDescBuffer.Load3(materialBufferOfs + 0));

	// fetch hit primitive data
	ByteAddressBuffer vertexBuffer = g_BufferArray[NonUniformResourceIndex(subMeshDesc.VertexBufferDescriptor)];
	ByteAddressBuffer indexBuffer = g_BufferArray[NonUniformResourceIndex(subMeshDesc.IndexBufferDescriptor)];
	
	const uint hitPrimitiveIdx = PrimitiveIndex();
	const uint indexBufferOfs = (subMeshDesc.PrimitivesOffset + hitPrimitiveIdx * 3) * IndexSize;
	const uint3 indices = indexBuffer.Load3(indexBufferOfs);
	float3 vertexNormal[3];
	float2 vertexTexcoord[3];
	[unroll] for (int i = 0; i < 3; ++i)
	{
		uint vertexBufferOfs = indices[i] * VertexSize;
		vertexNormal[i] = asfloat(vertexBuffer.Load3(vertexBufferOfs + 12));
		vertexTexcoord[i] = asfloat(vertexBuffer.Load2(vertexBufferOfs + 36));
	}
	const float3 baryCoords = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float3 normal = normalize(mul(InterpolatedVertexAttribute3(vertexNormal, baryCoords), instanceFrame));
	if (HIT_KIND_TRIANGLE_BACK_FACE == HitKind()) normal *= -1; // back face
	float2 texCoord = InterpolatedVertexAttribute2(vertexTexcoord, baryCoords);
	#if (1)
	texCoord = float2(texCoord.x, 1.0 - texCoord.y); // TODO: investigate why texCoord.y is inverted...
	#endif
	
	// fetch textures
	Texture2D colorMap = g_Texture2DArray[NonUniformResourceIndex(subMeshDesc.ColorMapDescriptor)];
	float colorMapMip = 0;
	float4 baseColor = colorMap.SampleLevel(g_SamplerBilinearWrap, texCoord, colorMapMip).xyzw;

	// fill material
	material.baseColor.xyz = baseColor.xyz * materialDesc.BaseColor.xyz;
	material.baseColor.w = baseColor.w;
	material.normal = normal;
	ApplyMaterialOverride(g_SceneCB, material);

	return material;
}

// ray generation

[shader("raygeneration")]
void RayGenMain()
{
	uint3 dispatchIdx = DispatchRaysIndex();
	uint2 dispatchSize = (uint2)g_SceneCB.LightBufferSize.xy;
	float4 shadowPerLight = 1.0;
	uint4 tracingInfo = 0; // sub sample pos & counters
	float3 indirectLight = 0.0;

	// fetch at first sub sample
	uint2 imagePos = dispatchIdx.xy * g_SceneCB.LightBufferDownscale.x;
	#if (0)
	// find best fitting sub sample
	uint dispatchDownscale = (uint)g_SceneCB.LightBufferDownscale.x;
	[branch] if (dispatchDownscale > 1)
	{
		uint2 imageSubPos = 0;
		float clipDepthCrnt = 1.0;
		for (uint iy = 0; iy < dispatchDownscale; ++iy)
		{
			for (uint ix = 0; ix < dispatchDownscale; ++ix)
			{
				float clipDepth = g_GeometryDepth.Load(int3(imagePos.x + ix, imagePos.y + iy, 0));
				[flatten] if (clipDepth < clipDepthCrnt)
				{
					clipDepthCrnt = clipDepth;
					imageSubPos = uint2(ix, iy);
				}
			}
		}
		imagePos += imageSubPos;
		tracingInfo[0] = imageSubPos.x + imageSubPos.y * dispatchDownscale;
	}
	#endif

	// read geometry buffer
	GBufferData gbData = LoadGBufferData(imagePos, g_GeometryDepth, g_GeometryImage0, g_GeometryImage1, g_GeometryImage2);
	bool isSky = (gbData.ClipDepth >= 1.0);

	[branch] if (!isSky)
	{
		float3 worldPos = ImagePosToWorldPos(imagePos, g_SceneCB.TargetSize.zw, gbData.ClipDepth, g_SceneCB.ViewProjInv);
		float worldDist = length(worldPos - g_SceneCB.CameraPos.xyz);
		float distBasedEps = worldDist * 5.0e-3;

		uint dispatchConstHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x);
		uint dispatchCrntHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x * (1 + g_SceneCB.FrameNumber * g_SceneCB.PerFrameJitter));
		float2 dispathNoise2d = BlueNoiseDiskLUT64[dispatchCrntHash % 64];
		float3x3 dispatchSamplingFrame;
		dispatchSamplingFrame[2] = float3(0.0, 0.0, 1.0);
		dispatchSamplingFrame[0] = normalize(float3(dispathNoise2d.xy, 0.0));
		dispatchSamplingFrame[1] = cross(dispatchSamplingFrame[2], dispatchSamplingFrame[0]);

		// Direct Shadow

		if (g_SceneCB.ShadowSamplesPerLight > 0)
		{
			RayDesc ray;
			ray.Origin = worldPos + gbData.Normal * distBasedEps;
			ray.TMin = 1.0e-3;
			ray.TMax = 1.0e+4;

			// per light
			uint lightCount = min(g_SceneCB.Lighting.LightSourceCount, ShadowBufferEntriesPerPixel);
			#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
			uint ilight = (g_SceneCB.FrameNumber % lightCount);
			#else
			for (uint ilight = 0; ilight < lightCount; ++ilight)
			#endif
			{
				LightDesc lightDesc = g_SceneCB.Lighting.LightSources[ilight];
				float3x3 lightDirTBN = ComputeLightSamplingBasis(worldPos, lightDesc);
				float shadowFactor = 0.0;

				uint sampleCount = g_SceneCB.ShadowSamplesPerLight * max(1, g_SceneCB.ShadowAccumulationFrames);
				uint sampleIdOfs = g_SceneCB.FrameNumber * g_SceneCB.ShadowSamplesPerLight * g_SceneCB.PerFrameJitter + dispatchConstHash;
				for (uint isample = 0; isample < g_SceneCB.ShadowSamplesPerLight; ++isample)
				{
					float3 sampleDir = GetDiskSampleDirection(isample + sampleIdOfs, sampleCount);
					ray.Direction = mul(mul(sampleDir, dispatchSamplingFrame), lightDirTBN);
					ray.TMax = (LightType_Directional == lightDesc.Type ? 1.0e+4 : length(lightDesc.Position - ray.Origin) - lightDesc.Size);

					RayDataDirect rayData = (RayDataDirect)0;
					rayData.occluded = true;

					// ray trace
					uint instanceInclusionMask = 0xff;
					uint rayContributionToHitGroupIndex = ShaderGroupIdx_ClosestHitDirect;
					uint multiplierForGeometryContributionToShaderIndex = 1;
					uint missShaderIndex = ShaderGroupIdx_MissDirect;
					TraceRay(g_SceneStructure,
						RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
						instanceInclusionMask,
						rayContributionToHitGroupIndex,
						multiplierForGeometryContributionToShaderIndex,
						missShaderIndex,
						ray, rayData);

					shadowFactor += float(rayData.occluded);
				}
				shadowFactor = 1.0 - shadowFactor / g_SceneCB.ShadowSamplesPerLight;
				shadowPerLight[ilight] = shadowFactor;
			}

			#if (SHADOW_BUFFER_APPLY_HISTORY_IN_RAYGEN)
			// apply accumulatd history
			float4 clipPosPrev = mul(float4(worldPos, 1.0), g_SceneCB.ViewProjPrev);
			clipPosPrev.xy /= clipPosPrev.w;
			if (g_SceneCB.ShadowAccumulationFrames > 0 && all(abs(clipPosPrev.xy) < 1.0))
			{
				float2 imagePosPrev = clamp((clipPosPrev.xy * float2(1.0, -1.0) + 1.0) * 0.5 * g_SceneCB.TargetSize.xy, float2(0, 0), g_SceneCB.TargetSize.xy - 1);
				uint2 dispatchPosPrev = clamp(floor(imagePosPrev * g_SceneCB.LightBufferDownscale.y), float2(0, 0), g_SceneCB.LightBufferSize.xy - 1);
				imagePosPrev = dispatchPosPrev * g_SceneCB.LightBufferDownscale.x;

				float clipDepthPrev = g_DepthHistory.Load(int3(imagePosPrev.xy, 0));
				bool isSkyPrev = (clipDepthPrev >= 1.0);
				// TODO
				#if (0)
				float depthDeltaTolerance = gbData.ClipDepth * /*1.0e-4*/max(1.0e-6, g_SceneCB.DebugVec0[2]);
				float surfaceHistoryWeight = 1.0 - saturate(abs(gbData.ClipDepth - clipDepthPrev) / depthDeltaTolerance);
				#elif (0)
				// history rejection from Ray Tracing Gems "Ray traced Shadows"
				float3 normalVS = mul(float4(gbData.Normal, 0.0), g_SceneCB.View).xyz;
				//float depthDeltaTolerance = 0.003 + 0.017 * abs(normalVS.z);
				float depthDeltaTolerance = g_SceneCB.DebugVec0[1] + g_SceneCB.DebugVec0[2] * abs(normalVS.z);
				float surfaceHistoryWeight = (abs(1.0 - clipDepthPrev / gbData.ClipDepth) < depthDeltaTolerance ? 1.0 : 0.0);
				#else
				float3 worldPosPrev = ClipPosToWorldPos(float3(clipPosPrev.xy, clipDepthPrev), g_SceneCB.ViewProjInvPrev);
				float viewDepth = ClipDepthToViewDepth(gbData.ClipDepth, g_SceneCB.Proj);
				float worldPosTolerance = viewDepth * /*1.0e-3*/max(1.0e-6, g_SceneCB.DebugVec0[2]);
				float surfaceHistoryWeight = 1.0 - saturate(length(worldPos - worldPosPrev) / worldPosTolerance);
				surfaceHistoryWeight = saturate((surfaceHistoryWeight - g_SceneCB.DebugVec0[1]) / (1.0 - g_SceneCB.DebugVec0[1]));
				#endif

				uint counter = g_TracingHistory.Load(int3(dispatchPosPrev.xy, 0)).y;
				counter = (uint)floor(float(counter) * surfaceHistoryWeight + 0.5);
				float historyWeight = float(counter) / (counter + 1);

				float4 shadowHistoryData = g_ShadowHistory.Load(int3(dispatchPosPrev.xy, 0));
				[unroll] for (uint j = 0; j < ShadowBufferEntriesPerPixel; ++j)
				{
					#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
					shadowPerLight[j] = lerp(shadowPerLight[j], shadowHistoryData[j], (ilight == j ? historyWeight : 1.0));
					#else
					shadowPerLight[j] = lerp(shadowPerLight[j], shadowHistoryData[j], historyWeight);
					#endif
				}

				#if (SHADOW_BUFFER_ONE_LIGHT_PER_FRAME)
				if (ilight == lightCount - 1)
				#endif
				counter = clamp(counter + 1, 0, g_SceneCB.ShadowAccumulationFrames);
				tracingInfo[1] = counter;
			}
			#endif
		}

		// Indirect Light

		if (g_SceneCB.IndirectSamplesPerFrame > 0)
		{
			#if (RT_REFLECTION_TEST) || (RT_GI_TEST)
			float rayTraceDist = 100.0;
			#else
			float rayTraceDist = 20.0;
			#endif
			RayDesc ray;
			ray.Origin = worldPos + gbData.Normal * distBasedEps;
			ray.TMax = worldDist * rayTraceDist;
			ray.TMin = 1.0e-3;// ray.TMax * 1.0e-4;

			float3x3 surfaceTBN = ComputeSamplingBasis(gbData.Normal);
			float3 skyLight = 0.0;
			uint sampleCount = g_SceneCB.IndirectSamplesPerFrame * (g_SceneCB.IndirectAccumulationFrames + 1);
			uint sampleIdOfs = g_SceneCB.FrameNumber * g_SceneCB.IndirectSamplesPerFrame * g_SceneCB.PerFrameJitter + dispatchConstHash;
			for (uint isample = 0; isample < g_SceneCB.IndirectSamplesPerFrame; ++isample)
			{
				float3 sampleDir = GetHemisphereSampleDirection(isample + sampleIdOfs, sampleCount);
				ray.Direction = mul(mul(sampleDir, dispatchSamplingFrame), surfaceTBN);
				#if (RT_REFLECTION_TEST)
				// TEST: specular lobe importance sampling
				if (gbData.Normal.y > 0.9)
				{
					float3 viewVec = normalize(g_SceneCB.CameraPos.xyz - worldPos);
					float2 xi = Hammersley((isample + sampleIdOfs) % sampleCount, sampleCount);
					xi = mul(float3(xi.xy, 1.0), dispatchSamplingFrame).xy;
					ray.Direction = ImportanceSampleGGX(xi, g_SceneCB.Material.Roughness, gbData.Normal, viewVec);
				}
				#endif

				RayDataIndirect rayData = (RayDataIndirect)0;
				rayData.recursionDepth = 0;
				rayData.hitDist = -1; // miss
				rayData.luminance = 0.0;

				// ray trace
				uint instanceInclusionMask = 0xff;
				uint rayContributionToHitGroupIndex = ShaderGroupIdx_ClosestHitIndirect;
				uint multiplierForGeometryContributionToShaderIndex = 1;
				uint missShaderIndex = ShaderGroupIdx_MissIndirect;
				TraceRay(g_SceneStructure,
					RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
					instanceInclusionMask,
					rayContributionToHitGroupIndex,
					multiplierForGeometryContributionToShaderIndex,
					missShaderIndex,
					ray, rayData);

				float NdotL = saturate(dot(gbData.Normal, ray.Direction));
				indirectLight += rayData.luminance * NdotL;
			}
			indirectLight /= g_SceneCB.IndirectSamplesPerFrame;
		}
		else
		{
			// simplified ambient
			float3 skyDir = float3(gbData.Normal.x, max(gbData.Normal.y, 0.0), gbData.Normal.z);
			skyDir = normalize(skyDir * 0.5 + WorldUp);
			float3 skyLight = GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerBilinearWrap, worldPos, skyDir).xyz;
			indirectLight = skyLight * 0.5;// *0.05;
		}
	}

	// write result
	g_ShadowTarget[dispatchIdx.xy] = shadowPerLight;
	g_TracingInfoTarget[dispatchIdx.xy] = tracingInfo;
	g_IndirectLightTarget[dispatchIdx.xy] = float4(indirectLight.xyz, 0.0);
}

// miss

[shader("miss")]
void MissDirect(inout RayDataDirect rayData)
{
	rayData.occluded = false;
}

[shader("miss")]
void MissIndirect(inout RayDataIndirect rayData)
{
	rayData.hitDist = -1;
	rayData.luminance += GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerBilinearWrap, WorldRayHitPoint(), WorldRayDirection()).xyz;
}

// closest

[shader("closesthit")]
void ClosestHitDirect(inout RayDataDirect rayData, in BuiltInTriangleIntersectionAttributes attribs)
{
	// nothing to do, miss shader updates RayDataDirect

	#if (RT_ALPHATEST)
	// TODO: implement in any hit shader
	const float3 hitWorldPos = WorldRayHitPoint();
	MaterialInputs material = GetMeshMaterialAtRayHitPoint(attribs);
	rayData.occluded = (material.baseColor.w < 0.9 ? false : true);
	#endif
}

[shader("closesthit")]
void ClosestHitIndirect(inout RayDataIndirect rayData, in BuiltInTriangleIntersectionAttributes attribs)
{
	rayData.hitDist = RayTCurrent();
	rayData.recursionDepth += 1;

	// read hit surface material
	const float3 hitWorldPos = WorldRayHitPoint();
	MaterialInputs material = GetMeshMaterialAtRayHitPoint(attribs);

#if (RT_GI_TEST)

	// recursive bounces
	const uint IndirectLightBounces = min(8, (uint)g_SceneCB.DebugVec1[0]); //2;
	if (rayData.recursionDepth <= IndirectLightBounces)
	{
		const float3 hitWorldPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
		RayDesc ray;
		ray.Origin = hitWorldPos;
		ray.TMax = 100.0;
		ray.TMin = 1.0e-3;

		// sample direction
		uint3 dispatchIdx = DispatchRaysIndex();
		uint3 dispatchSize = DispatchRaysDimensions();
		uint dispatchConstHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x);
		uint dispatchCrntHash = HashUInt(dispatchIdx.x + dispatchIdx.y * dispatchSize.x * (1 + g_SceneCB.FrameNumber * g_SceneCB.PerFrameJitter));
		float2 dispathNoise2d = BlueNoiseDiskLUT64[dispatchCrntHash % 64];
		float3x3 dispatchSamplingFrame;
		dispatchSamplingFrame[2] = float3(0.0, 0.0, 1.0);
		dispatchSamplingFrame[0] = normalize(float3(dispathNoise2d.xy, 0.0));
		dispatchSamplingFrame[1] = cross(dispatchSamplingFrame[2], dispatchSamplingFrame[0]);
		uint sampleId = g_SceneCB.FrameNumber * g_SceneCB.PerFrameJitter + dispatchConstHash;
		uint sampleCount = g_SceneCB.IndirectAccumulationFrames + 1;
		float3x3 surfaceTBN = ComputeSamplingBasis(material.normal);
		//float3 sampleDir = GetHemisphereSampleDirection(sampleId, sampleCount);
		float2 p2d = Hammersley(sampleId % sampleCount, sampleCount);
		float3 sampleDir = HemisphereSampleCosine(p2d.x, p2d.y);
		ray.Direction = mul(mul(sampleDir, dispatchSamplingFrame), surfaceTBN);

		#if (RT_REFLECTION_TEST)
		// TEST: specular lobe importance sampling
		if (material.normal.y > 0.9)
		{
			float3 viewVec = normalize(WorldRayOrigin() - hitWorldPos);
			float2 xi = Hammersley(sampleId % sampleCount, sampleCount);
			xi = mul(float3(xi.xy, 1.0), dispatchSamplingFrame).xy;
			ray.Direction = ImportanceSampleGGX(xi, g_SceneCB.Material.Roughness, material.normal, viewVec);
		}
		#endif

		// ray trace
		uint instanceInclusionMask = 0xff;
		uint rayContributionToHitGroupIndex = ShaderGroupIdx_ClosestHitIndirect;
		uint multiplierForGeometryContributionToShaderIndex = 1;
		uint missShaderIndex = ShaderGroupIdx_MissIndirect;
		TraceRay(g_SceneStructure,
			RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
			instanceInclusionMask,
			rayContributionToHitGroupIndex,
			multiplierForGeometryContributionToShaderIndex,
			missShaderIndex,
			ray, rayData);

		float NoL = saturate(dot(material.normal, ray.Direction));
		rayData.luminance = rayData.luminance * material.baseColor.xyz * NoL * g_SceneCB.DebugVec2[3];
	}

	// calculate reflected radiance
	
	// direct
	LightingParams lightingParams = (LightingParams)0;
	getLightingParams(hitWorldPos, WorldRayOrigin(), material, lightingParams);
	float3 directLight = 0;
	uint lightCount = min(2, g_SceneCB.Lighting.LightSourceCount);
	for (uint ilight = 0; ilight < lightCount; ++ilight)
	{
		LightDesc light = g_SceneCB.Lighting.LightSources[ilight];
		float3 hitPosToLightVec = light.Position.xyz - hitWorldPos.xyz;
		float hitPosToLightDist = length(hitPosToLightVec);
		float3 lightDir = (LightType_Directional == light.Type ? -light.Direction : normalize(hitPosToLightVec));
		float shadowFactor = 1.0;

		// ray trace occlusion
		#if (1)
		RayDesc ray;
		ray.Origin = hitWorldPos;
		ray.TMax = (LightType_Directional == light.Type ? 1.0e+4 : max(0.0, hitPosToLightDist - light.Size));
		ray.TMin = 1.0e-3;
		ray.Direction = lightDir;

		RayDataDirect rayData = (RayDataDirect)0;
		rayData.occluded = true;

		uint instanceInclusionMask = 0xff;
		uint rayContributionToHitGroupIndex = ShaderGroupIdx_ClosestHitDirect;
		uint multiplierForGeometryContributionToShaderIndex = 1;
		uint missShaderIndex = ShaderGroupIdx_MissDirect;
		TraceRay(g_SceneStructure,
			RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
			instanceInclusionMask,
			rayContributionToHitGroupIndex,
			multiplierForGeometryContributionToShaderIndex,
			missShaderIndex,
			ray, rayData);

		shadowFactor = float(rayData.occluded ? 0.0 : 1.0);
		#else

		shadowFactor = 0.05;
		#endif

		// evaluate direct light

		float NoL = saturate(dot(lightDir, lightingParams.normal));
		#if (1)
		// TODO: check corectness of irradiance intensity
		directLight += GetLightIntensity(lightingParams, light) * shadowFactor * NoL * lightingParams.diffuseColor.xyz;
		#else
		shadowFactor *= saturate(NoL * 10.0); // approximate self shadowing at grazing angles
		float specularOcclusion = shadowFactor;
		directLight += EvaluateDirectLighting(lightingParams, light, shadowFactor, specularOcclusion).xyz;
		#endif
	}
	rayData.luminance += directLight;

	#if (RT_ALPHATEST)
	// TEMP: alpha tested windows approximation
	// TODO: implement in any hit shader
	if (material.baseColor.w < 0.9)
	{
		float3 skyLight = GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerBilinearWrap, hitWorldPos, WorldRayDirection()).xyz;
		rayData.luminance += skyLight * (1.0 - material.baseColor.w);
	}
	#endif

	// ambient approximation
	// TODO: consider as fallback when bounces limit reached
	#if (RT_REFLECTION_TEST) && 0
	float3 skyDir = float3(material.normal.x, max(material.normal.y, 0.0), material.normal.z);
	skyDir = normalize(skyDir * 0.5 + WorldUp);
	float3 skyLight = GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerBilinearWrap, hitWorldPos, skyDir).xyz;
	rayData.luminance = skyLight * material.baseColor.xyz * 0.5;
	#endif

#else

	// simplified indirect: sky light with ambient occlusion

	float hitWorldDist = length(hitWorldPos - g_SceneCB.CameraPos.xyz);
	float occlusionDist = hitWorldDist * g_SceneCB.DebugVec1[0];// 20.0;
	float sampleOcclusion = saturate(1.0 - rayData.hitDist / occlusionDist);
	float3 skyDir = WorldRayDirection();
	float3 absorption = 1.0;
	if (skyDir.z <= 0.0)
	{
		// bounced sky light approximation
		skyDir = reflect(skyDir, material.normal);
		absorption = material.baseColor.xyz;
	}
	rayData.luminance += GetSkyLight(g_SceneCB, g_PrecomputedSky, g_SamplerBilinearWrap, hitWorldDist, skyDir).xyz * absorption * (1.0 - sampleOcclusion);

#endif
}