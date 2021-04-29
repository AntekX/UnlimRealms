
#include "HybridRendering.hlsli"

MeshPixelOutput main(MeshPixelInput input, in bool isFrontFace : SV_IsFrontFace)
{
	MeshPixelOutput output;

	float4 baseColor = g_ColorTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xyzw;
	float2 bumpPacked = g_NormalTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xy;
	float mask = g_MaskTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).w;
	float alpha = mask;
	clip(alpha - 0.5);

	const bool NormalMapInvertY = true;
	float3 bumpNormal;
	bumpNormal.xy = float2(bumpPacked.x, (NormalMapInvertY ? 1.0 - bumpPacked.y : bumpPacked.y)) * 2.0 - 1.0;
	bumpNormal.z = sqrt(saturate(1.0 - dot(bumpNormal.xy, bumpNormal.xy)));

	float3 normal = input.Normal.xyz;
	#if (1)
	float3 bitangent = float3(0.0, 0.0, 1.0);
	if (normal.z + Eps >= 1.0) bitangent = float3(0.0, -1.0, 0.0);
	if (normal.z - Eps <= -1.0) bitangent = float3(0.0, 1.0, 0.0);
	float3 tangent = normalize(cross(bitangent, normal));
	bitangent = cross(normal, tangent);
	#else
	float3 tangent = input.Tangent.xyz;
	float3 bitangent = normalize(cross(normal, tangent));
	#endif
	float3x3 surfaceTBN = {
		tangent,
		bitangent,
		normal
	};
	surfaceTBN *= (isFrontFace ? 1.0 : -1.0);
	normal = mul(bumpNormal, surfaceTBN);

	output.Target0 = float4(baseColor.xyz * input.Color.xyz, 1);
	output.Target1 = float4(normal.xyz, 0);
	output.Target2 = float4(tangent.xyz, 0);

	return output;
}