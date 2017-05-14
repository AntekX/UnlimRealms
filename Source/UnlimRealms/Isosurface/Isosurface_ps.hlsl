///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Isosurface.hlsli"
#include "../HDRRender/HDRRender.hlsli"

#define TEXTURES_ENABLED 0

#if TEXTURES_ENABLED
sampler sampler0		: register(s0);
Texture2D albedoMap0	: register(t0);
Texture2D albedoMap1	: register(t1);
Texture2D albedoMap2	: register(t2);
Texture2D normalMap0	: register(t3);
Texture2D normalMap1	: register(t4);
Texture2D normalMap2	: register(t5);
#endif

float3 unpackNormal(float4 v)
{
	float3 n = float3(v.w, v.y, 0.0);
	n.z = sqrt(saturate(1.0 - dot(v.xy, v.xy)));
	return n;
}

float4 main(PS_INPUT input) : SV_Target
{
	float4 color = float4(0, 0, 0, 1);

#if 1
	float3 wpos_dx = ddx(input.wpos.xyz);
	float3 wpos_dy = ddy(input.wpos.xyz);
	float3 n = normalize(cross(wpos_dx, wpos_dy));
#else
	float3 n = input.norm;
#endif
	
	const float3 sunDir = float3(1, 0, 0);
	const float3 sphereN = normalize(input.wpos.xyz);
	const float slope = max(0.0, dot(n, sphereN));
	const float3 ambientLight = float3(0.1, 0.1, 0.2);// *0.5;
	const float sunLightWrap = 0.0;
	const float sunNdotL = max(0.0, (dot(-sunDir, n) + sunLightWrap) / (1.0 + sunLightWrap));
	const float globalSelfShadow = max(0.0, (dot(+sunDir, sphereN) + sunLightWrap) / (1.0 + sunLightWrap));

#if TEXTURES_ENABLED
	float tileScale = 1.0 / 8.0;
	float2 uvXY = input.wpos.xy * tileScale;
	float2 uvZY = input.wpos.zy * tileScale;
	float2 uvXZ = input.wpos.xz * tileScale;
	float4 tileColorXY = GammaToLinear(albedoMap0.Sample(sampler0, uvXY));
	float4 tileColorZY = GammaToLinear(albedoMap1.Sample(sampler0, uvZY));
	float4 tileColorXZ = GammaToLinear(albedoMap2.Sample(sampler0, uvXZ));
	float3 tileNormalXY = unpackNormal(normalMap0.Sample(sampler0, uvXY));
	float3 tileNormalZY = unpackNormal(normalMap1.Sample(sampler0, uvZY));
	float3 tileNormalXZ = unpackNormal(normalMap2.Sample(sampler0, uvXZ));
	#if 0
	tileColorXY.xyz = float3(1, 0, 0);
	tileColorZY.xyz = float3(0, 1, 0);
	tileColorXZ.xyz = float3(0, 0, 1);
	#endif
	float wXY = (tileColorXY.w + 0.1) * abs(dot(n, float3(0, 0, 1)));
	float wZY = (tileColorZY.w + 0.1) * abs(dot(n, float3(1, 0, 0)));
	float wXZ = (tileColorXZ.w + 0.1) * abs(dot(n, float3(0, 1, 0)));
	float wS = wXY + wZY + wXZ;
	wXY /= wS;
	wZY /= wS;
	wXZ /= wS;
	float4 tileColor = tileColorXY * wXY + tileColorZY * wZY + tileColorXZ * wXZ;
	float3 tileNormal = tileNormalXY * wXY + tileNormalZY * wZY + tileNormalXZ * wXZ;
	const float3 surfColor = tileColor.xyz;
#else
	const float3 surfColor = lerp(float3(0.7, 0.5, 0.5)*0.75, float3(1.0, 0.6, 0.2), slope);
#endif
	
	color.xyz = surfColor * (ambientLight + SunLight * max(0.0, sunNdotL - globalSelfShadow));

	color.xyz = atmosphericScatteringSurface(Atmosphere, color.xyz, input.wpos.xyz, CameraPos.xyz).xyz;

	return color;
}