
#include "HybridRendering.hlsli"

sampler				g_SamplerTrilinearWrap	: register(s0);
Texture2D<float4>	g_ColorTexture			: register(t1);
Texture2D<float4>	g_NormalTexture			: register(t2);
Texture2D<float4>	g_MaskTexture			: register(t3);

MeshPixelOutput main(MeshPixelInput input)
{
	MeshPixelOutput output;

	float3 baseColor = g_ColorTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xyz;
	float2 bumpPacked = g_NormalTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xy;
	float mask = g_MaskTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).x;

	//if (!any(baseColor)) baseColor.xyz = float3(0.5, 0.5, 0.5);
	//if (!any(bumpPacked)) bumpPacked.xy = float2(0.5, 0.5);
	//if (!any(mask)) mask = 1.0;
	//clip(mask - 0.5);

	float3 bumpNormal;
	bumpNormal.xy = float2(bumpPacked.x, bumpPacked.y) * 2.0 - 1.0;
	bumpNormal.z = sqrt(saturate(1.0 - dot(bumpNormal.xy, bumpNormal.xy)));

	float3 normal = input.Norm.xyz;
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
	normal = mul(bumpNormal, surfaceTBN);

	output.Target0 = float4(baseColor.xyz, 1);
	output.Target1 = float4(normal.xyz, 0);
	output.Target2 = float4(input.TexCoord.xy, 0, 0);

	return output;
}