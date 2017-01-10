#include "Isosurface.hlsli"

float4 main(PS_INPUT input) : SV_Target
{
#if 1
	float3 wpos_dx = ddx(input.wpos);
	float3 wpos_dy = ddy(input.wpos);
	float3 n = normalize(cross(wpos_dx, wpos_dy));
	n.xyz = n.xzy;
#else
	float3 n = input.norm;
#endif
	float4 color = float4((n + 1.0) * 0.5, 1.0f);
	const float3 sunDir = float3(1, 0, 0);
	color *= (dot(-sunDir, n) + 1.0) * 0.5;
	return color;
}