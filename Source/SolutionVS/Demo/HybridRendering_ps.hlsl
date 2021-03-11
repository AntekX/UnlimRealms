
#include "HybridRendering.hlsli"

//sampler				g_SamplerTrilinear		: register(s0);
//Texture2D<float4>	g_ColorTexture			: register(t1);
//Texture2D<float4>	g_NormalTexture			: register(t2);
//Texture2D<float4>	g_MaskTexture			: register(t3);

MeshPixelOutput main(MeshPixelInput input)
{
	MeshPixelOutput output;

	//float3 baseColor = g_ColorTexture.Sample(g_SamplerTrilinear, input.TexCoord.xy).xyz;
	//float3 normal = g_ColorTexture.Sample(g_SamplerTrilinear, input.TexCoord.xy).xyz;
	//float mask = g_ColorTexture.Sample(g_SamplerTrilinear, input.TexCoord.xy).x;

	output.Target0 = float4(1, 0, 0, 0);
	output.Target1 = float4(input.Norm.xyz, 0);
	output.Target2 = float4(0, 0, input.TexCoord.xy);

	return output;
}