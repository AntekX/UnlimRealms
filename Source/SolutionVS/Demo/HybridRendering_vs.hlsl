
#include "HybridRendering.hlsli"

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);

MeshPixelInput main(MeshVertexInput input)
{
	MeshPixelInput output;

	output.Pos = mul(float4(input.Pos.xyz, 1.0), g_SceneCB.ViewProj);
	output.Norm = input.Norm;
	output.TexCoord = input.TexCoord;

	return output;
}
