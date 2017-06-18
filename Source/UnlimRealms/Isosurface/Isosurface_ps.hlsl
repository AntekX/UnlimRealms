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

static const float3x3 TBN_XP = { 
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 1.0, 0.0 },
	{ 1.0, 0.0, 0.0 }
};
static const float3x3 TBN_XN = {
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 1.0, 0.0 },
	{-1.0, 0.0, 0.0 }
};
static const float3x3 TBN_YP = {
	{ 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 1.0, 0.0 }
};
static const float3x3 TBN_YN = {
	{ 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0,-1.0, 0.0 }
};
static const float3x3 TBN_ZP = {
	{ 1.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 }
};
static const float3x3 TBN_ZN = {
	{ 1.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0,-1.0 }
};

int GetMaxValueIdx(float3 v)
{
	if (!(v.x < v.y || v.x < v.z)) return 0;
	if (!(v.y < v.z)) return 1;
	return 2;
}

int GetMaxValueIdx(float4 v)
{
	if (!(v.x < v.y || v.x < v.z || v.x < v.w)) return 0;
	if (!(v.y < v.z || v.y < v.w)) return 1;
	if (!(v.z < v.w)) return 2;
	return 3;
}

float3x3 GetDirectionalTBN(float3 n)
{
	int idx = GetMaxValueIdx(abs(n));
	if (0 == idx) return (n.x < 0 ? TBN_XN : TBN_XP);
	if (1 == idx) return (n.y < 0 ? TBN_YN : TBN_YP);
	return (n.z < 0 ? TBN_ZN : TBN_ZP);
}

float3x3 GetTBN(float3 n)
{
	float3x3 tbn = GetDirectionalTBN(n);
	tbn[2] = n;
	tbn[0] = cross(tbn[1], tbn[2]);
	tbn[1] = cross(tbn[2], tbn[0]);
	return tbn;
}

float3 UnpackNormal(float4 v)
{
	float3 n = float3(v.w, v.y, 0.0);
	n.z = sqrt(max(0.0, 1.0 - dot(v.xy, v.xy)));
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
	const float3 sphereN = normalize(input.wpos.xyz);
	const float slope = max(0.0, dot(n, sphereN));

#if TEXTURES_ENABLED
	float tileScale = 1.0 / 8.0;
	float2 uvXY = input.wpos.xy * tileScale;
	uvXY.y = 1.0 - frac(uvXY.y);
	float2 uvZY = input.wpos.zy * tileScale;
	uvZY.y = 1.0 - frac(uvZY.y);
	float2 uvXZ = input.wpos.xz * tileScale;
	uvXZ.y = 1.0 - frac(uvXZ.y);
	float4 tileColorXY = GammaToLinear(albedoMap0.Sample(sampler0, uvXY));
	float4 tileColorZY = GammaToLinear(albedoMap1.Sample(sampler0, uvZY));
	float4 tileColorXZ = GammaToLinear(albedoMap2.Sample(sampler0, uvXZ));
	float3 tileNormalXY = UnpackNormal(normalMap0.Sample(sampler0, uvXY));
	float3 tileNormalZY = UnpackNormal(normalMap1.Sample(sampler0, uvZY));
	float3 tileNormalXZ = UnpackNormal(normalMap2.Sample(sampler0, uvXZ));
	#if 0
	tileColorXY.xyz = float3(1, 0, 0);
	tileColorZY.xyz = float3(0, 1, 0);
	tileColorXZ.xyz = float3(0, 0, 1);
	#endif
	float wXY = (tileColorXY.w + 1.0) * abs(dot(n, float3(0, 0, 1)));
	float wZY = (tileColorZY.w + 1.0) * abs(dot(n, float3(1, 0, 0)));
	float wXZ = (tileColorXZ.w + 1.0) * abs(dot(n, float3(0, 1, 0)));
	float wS = wXY + wZY + wXZ;
	wXY /= wS;
	wZY /= wS;
	wXZ /= wS;
	float4 tileColor = tileColorXY * wXY + tileColorZY * wZY + tileColorXZ * wXZ;
	float3 tileNormal = tileNormalXY * wXY + tileNormalZY * wZY + tileNormalXZ * wXZ;
	const float3 surfAlbedo = tileColor.xyz;
	const float3 surfNormal = mul(tileNormal, GetTBN(n));
#else
	const float3 surfAlbedo = lerp(float3(0.7, 0.5, 0.5)*0.75, float3(1.0, 0.6, 0.2), slope);
	const float3 surfNormal = n;
#endif

	const float3 sunDir = float3(1, 0, 0);
	const float3 ambientLight = float3(0.5, 0.5, 1.0) * 1e-2;
	const float sunNdotL = max(0.0, dot(-LightDirection, surfNormal));
	const float globalSelfShadow = max(0.0, dot(+LightDirection, sphereN));
	
	float3 directLightIntensity = LightIntensity * 0.025;
	float3 surfDirect = surfAlbedo * max(0.0, sunNdotL - globalSelfShadow) * directLightIntensity;
	float3 surfAmbient = surfAlbedo * ambientLight;
	color.xyz = surfAmbient + AtmosphericScatteringSurface(Atmosphere, surfDirect, input.wpos.xyz, CameraPos.xyz).xyz;

	return color;
}