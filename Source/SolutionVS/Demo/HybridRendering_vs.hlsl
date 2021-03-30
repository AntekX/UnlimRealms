
#include "HybridRendering.hlsli"

ConstantBuffer<SceneConstants> g_SceneCB	: register(b0);
ByteAddressBuffer	g_InstanceBuffer		: register(t0);

MeshPixelInput main(MeshVertexInput input, uint instanceId : SV_InstanceID)
{
	MeshPixelInput output;

	// load instance data
	uint instanceOfs = instanceId * InstanceStride;
	float3x4 instanceTransform;
	instanceTransform[0] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 0));
	instanceTransform[1] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 16));
	instanceTransform[2] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 32));

	// transform
	float3 worldPos = mul(instanceTransform, float4(input.Pos.xyz, 1.0)); // order changed because input matrix is column major
	float3 worldNorm = normalize(mul(instanceTransform, float4(input.Normal, 0.0)));
	float3 worldTangent = normalize(mul(instanceTransform, float4(input.Tangent, 0.0)));

	// output
	output.Pos = mul(float4(worldPos, 1.0), g_SceneCB.ViewProj);
	output.Normal = worldNorm;
	output.Tangent = worldTangent;
	output.TexCoord = float2(input.TexCoord.x, 1.0 - input.TexCoord.y);

	return output;
}
