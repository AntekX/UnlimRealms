///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Isosurface.hlsli"

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
	const float3 surfColor = lerp(float3(0.7, 0.5, 0.3)*0.75, float3(0.7, 0.8, 0.4), slope);
	const float3 ambientLight = float3(0.1, 0.1, 0.15);
	const float sunLightWrap = 0.5;
	const float sunNdotL = max(0.0, (dot(-sunDir, n) + sunLightWrap) / (1.0 + sunLightWrap));
	const float globalSelfShadow = max(0.0, (dot(+sunDir, sphereN) + sunLightWrap) / (1.0 + sunLightWrap));
	color.xyz = surfColor * (ambientLight + SunLight * max(0.0, sunNdotL - globalSelfShadow));

#if 0
	const float3 fogColorMax = float3(0.6, 0.8, 1.0);
	const float3 fogColorMin = float3(0.1, 0.1, 0.15);
	const float fogColorWrap = 0.5;
	const float fogNdotL = max(0.0, (dot(-sunDir, sphereN) + fogColorWrap) / (1.0 + fogColorWrap));
	const float3 fogColor = lerp(fogColorMin, fogColorMax, fogNdotL);
	const float fogBegin = 0.0f;
	const float fogRange = 5.0f;
	const float fogDensityMax = 0.75;
	float fogDensity = clamp((input.wpos.w - fogBegin) / fogRange, 0.0, fogDensityMax);
	color.xyz = lerp(color.xyz, fogColor, fogDensity);
#else
	color.xyz = atmosphericScatteringSurface(Atmosphere, color.xyz, input.wpos.xyz, CameraPos.xyz).xyz;
#endif

	return color;
}