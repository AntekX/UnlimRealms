
#include "HybridRendering.hlsli"

Texture2D<float>	g_MeshTexture	: register(t0);

MeshPixelOutput main(MeshPixelInput input)
{
	MeshPixelOutput output;

	output.Target0 = float4(1, 0, 0, 0);
	output.Target1 = float4((input.Norm.xyz + 1.0) * 0.5, 0);
	output.Target2 = float4(0, 0, input.TexCoord.xy);

	return output;
}