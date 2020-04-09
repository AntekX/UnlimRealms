
struct SceneConstants
{
	float4x4 viewProjInv;
	float4 cameraPos;
	float4 viewportSize; // w, h, 1/w, 1/h
	float4 clearColor;
	float4 lightDirection;
};

struct Vertex
{
	float3 pos;
	float3 norm;
};

static const uint RecursionDepthMax = 1;

ConstantBuffer<SceneConstants> g_SceneCB : register(b0);
RaytracingAccelerationStructure g_SceneStructure : register(t0);
//StructuredBuffer<Vertex> g_VertexBuffer: register(t1); // todo: load does not work as expected
ByteAddressBuffer g_VertexBuffer: register(t1);
ByteAddressBuffer g_IndexBuffer: register(t2);
RWTexture2D<float4> g_TargetTexture : register(u0);

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

float CalculateDiffuseCoefficient(in float3 worldPos, in float3 incidentLightRay, in float3 normal)
{
	float fNDotL = saturate(dot(-incidentLightRay, normal));
	return fNDotL;
}

float4 CalculateSpecularCoefficient(in float3 worldPos, in float3 incidentLightRay, in float3 normal, in float specularPower)
{
	float3 reflectedLightRay = normalize(reflect(incidentLightRay, normal));
	return pow(saturate(dot(reflectedLightRay, normalize(-WorldRayDirection()))), specularPower);
}

float3 FresnelReflectanceSchlick(in float3 I, in float3 N, in float3 f0)
{
	float cosi = saturate(dot(-I, N));
	return f0 + (1 - f0)*pow(1 - cosi, 5);
}

float4 CalculatePhongLighting(in float3 worldPos, in float3 normal, in float4 albedo, in float shadowFactor, in float diffuseCoef = 1.0, in float specularCoef = 1.0, in float specularPower = 50)
{
	float3 incidentLightRay = -g_SceneCB.lightDirection.xyz;

	// diffuse
	float4 lightDiffuseColor = float4(1.0, 1.0, 1.0, 1.0);
	float Kd = CalculateDiffuseCoefficient(worldPos, incidentLightRay, normal);
	float4 diffuseColor = diffuseCoef * Kd * lightDiffuseColor * albedo * shadowFactor;

	// specular
	float4 lightSpecularColor = float4(1, 1, 1, 1);
	float4 Ks = CalculateSpecularCoefficient(worldPos, incidentLightRay, normal, specularPower);
	float4 specularColor = specularCoef * Ks * lightSpecularColor * shadowFactor;

	// ambient
	float4 ambientColor = float4(0.4, 0.4, 0.4, 1.0);
	ambientColor = albedo * ambientColor;

	return ambientColor + diffuseColor + specularColor;
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

	float2 rayPixelPos = (float2)dispatchIdx.xy + 0.5;
	float2 rayUV = rayPixelPos / (float2)dispatchSize.xy;
	float2 rayClip = float2(rayUV.x, 1.0 - rayUV.y) * 2.0 - 1.0;
	#if (0)
	// test: simple orthographic projection
	ray.Origin = float3(rayClip, 0.0);
	ray.Direction = float3(0, 0, 1);
	#else
	// calculate camera ray
	float4 rayWorld = mul(g_SceneCB.viewProjInv, float4(rayClip.xy, 0.0, 1.0));
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
	float4 targetValue = 0.0;
	targetValue.xyz = lerp(targetValue.xyz, rayData.color.xyz, rayData.color.w);
	targetValue.w = saturate(targetValue.w + rayData.color.w);
	g_TargetTexture[dispatchIdx.xy] = targetValue;
}

[shader("miss")]
void SampleMiss(inout SampleRayData rayData)
{
	//rayData.color.xyz = float3(max(rayData.clipPos.xy, 0.0), 0.0);
	//rayData.color.w = saturate(max(rayData.color.x, rayData.color.y));
	
	// TODO: calculate atmosphere color here
	rayData.color.xyz = g_SceneCB.clearColor.xyz;
	rayData.color.w = 1.0;
}

[shader("miss")]
void SampleMissShadow(inout SampleShadowData shadowData)
{
	shadowData.occluded = false;
}

[shader("closesthit")]
void SampleClosestHit(inout SampleRayData rayData, in SampleHitAttributes attribs)
{
	float3 baryCoords = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float4 debugColor = float4(baryCoords.xyz, 1.0);
	float3 hitWorldPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

	// read vertex attributes

	uint indexOffset = PrimitiveIndex() * 3 * 4;
	uint3 indices = g_IndexBuffer.Load3(indexOffset);
	#if (0)
	// todo: fix structured buffer support
	Vertex vertices[3];
	vertices[0] = g_VertexBuffer.Load(indices[0]);
	vertices[1] = g_VertexBuffer.Load(indices[1]);
	vertices[2] = g_VertexBuffer.Load(indices[2]);
	float3 normal = vertices[0].norm;
	#else
	// read vertex data from from byte buffer
	uint vertexStride = 24;
	uint vertexNormOfs = 12;
	float3 normal;
	uint vertexOfs = indices[0] * vertexStride + vertexNormOfs;
	normal.x = asfloat(g_VertexBuffer.Load(vertexOfs + 0));
	normal.y = asfloat(g_VertexBuffer.Load(vertexOfs + 4));
	normal.z = asfloat(g_VertexBuffer.Load(vertexOfs + 8));
	#endif

	// trace shadow data

	float shadowFactor = 1.0;
	{
		SampleShadowData shadowData;
		shadowData.occluded = true;

		RayDesc ray;
		ray.TMin = 0.001;
		ray.TMax = 1.0e+4;
		ray.Origin = hitWorldPos;
		ray.Direction = g_SceneCB.lightDirection.xyz;

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

		shadowFactor = (shadowData.occluded ? 0.0 : 1.0);
	}

	// trace reflection data
	
	float4 reflectionColor = 0.0;
	if (rayData.recursionDepth < RecursionDepthMax)
	{
		SampleRayData reflectionData;
		reflectionData.color = 0;
		reflectionData.recursionDepth = rayData.recursionDepth + 1;

		RayDesc ray;
		ray.TMin = 0.001;
		ray.TMax = 1.0e+4;
		ray.Origin = hitWorldPos;
		ray.Direction = reflect(WorldRayDirection(), normal);

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
		
		reflectionColor = reflectionData.color;
	}

	// calculate lighting color

	float4 albedo = debugColor;
	float diffuseCoef = 0.9;
	float specularCoef = 0.5;
	float specularPower = 50.0;
	float reflectanceCoef = 0.25;
	float4 lightColor = CalculatePhongLighting(hitWorldPos, normal, albedo, shadowFactor, diffuseCoef, specularCoef, specularPower);
	float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), normal, albedo.xyz);
	lightColor += reflectanceCoef * float4(fresnelR, 1.0) * reflectionColor;

	rayData.color = lightColor;
}