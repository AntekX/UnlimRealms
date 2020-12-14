
#include "HybridRendering.hlsli"

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);

MeshPixelInput main(MeshVertexInput input)
{
	MeshPixelInput output;

	output.pos = mul(float4(input.pos.xyz, 1.0), g_SceneCB.viewProj);
	output.norm = input.norm;

	return output;
}
