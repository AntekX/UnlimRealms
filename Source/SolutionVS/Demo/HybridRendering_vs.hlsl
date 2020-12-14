
#include "HybridRendering.hlsli"

MeshPixelInput main(MeshVertexInput input)
{
	MeshPixelInput output;

	output.pos = LogDepthPos(mul(float4(input.pos.xyz, 1.0), g_SceneCB.viewProj));
	output.norm = input.norm;

	return output;
}
