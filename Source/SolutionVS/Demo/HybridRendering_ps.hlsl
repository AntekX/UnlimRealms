
#include "HybridRendering.hlsli"

sampler				g_SamplerTrilinearWrap	: register(s0);
Texture2D<float4>	g_ColorTexture			: register(t1);
Texture2D<float4>	g_NormalTexture			: register(t2);
Texture2D<float4>	g_MaskTexture			: register(t3);

MeshPixelOutput main(MeshPixelInput input)
{
	MeshPixelOutput output;

	float3 baseColor = SRGBToLinear(g_ColorTexture.Sample(g_SamplerTrilinearWrap, input.TexCoord.xy).xyz);
	//float3 normal = g_ColorTexture.Sample(g_SamplerTrilinear, input.TexCoord.xy).xyz;
	//float mask = g_ColorTexture.Sample(g_SamplerTrilinear, input.TexCoord.xy).x;

	output.Target0 = float4(baseColor.xyz, 1);
	output.Target1 = float4(input.Norm.xyz, 0);
	output.Target2 = float4(input.TexCoord.xy, 0, 0);

	return output;
}