
#include "HybridRendering.hlsli"

MeshPixelOutput main(MeshPixelInput input)
{
	MeshPixelOutput output;

	float3 baseColor = g_ColorTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xyz;
	float2 bumpPacked = g_NormalTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xy;
	float mask = g_MaskTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).x;
	
	// dyanmic indexing test
	//baseColor = g_Texture2DArray[0].Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xyz;

	//if (!any(baseColor)) baseColor.xyz = float3(0.5, 0.5, 0.5);
	//if (!any(bumpPacked)) bumpPacked.xy = float2(0.5, 0.5);
	//if (!any(mask)) mask = 1.0;
	//clip(mask - 0.5);

	float3 bumpNormal;
	bumpNormal.xy = float2(bumpPacked.x, bumpPacked.y) * 2.0 - 1.0;
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
	normal = mul(bumpNormal, surfaceTBN);

	output.Target0 = float4(baseColor.xyz, 1);
	output.Target1 = float4(normal.xyz, 0);
	output.Target2 = float4(tangent.xyz, 0);

	return output;
}