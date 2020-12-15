
#include "HybridRendering.hlsli"

MeshPixelOutput main(MeshPixelInput input)
{
	MeshPixelOutput output;

	output.target0 = float4(1, 0, 0, 0);
	output.target1 = float4((input.norm.xyz + 1.0) * 0.5, 0);
	output.target2 = float4(0, 0, 0, 0);

	return output;
}