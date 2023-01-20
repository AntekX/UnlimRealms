
#include "HybridRendering.hlsli"

MeshPixelInput main(MeshVertexInput input, uint instanceId : SV_InstanceID)
{
	MeshPixelInput output;

	// load instance data
	uint instanceOfs = (g_SubMeshCB.InstanceOfs + instanceId) * InstanceSize;
	float3x4 instanceTransform;
	instanceTransform[0] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 0));
	instanceTransform[1] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 16));
	instanceTransform[2] = asfloat(g_InstanceBuffer.Load4(instanceOfs + 32));
	
	// load mesh material desc
	const uint subMeshIdx = (g_InstanceBuffer.Load(instanceOfs + 48) & 0x00ffffff);
	const uint subMeshBufferOfs = subMeshIdx * SubMeshDescSize;
	const uint materialBufferIndex = g_MeshDescBuffer.Load(subMeshBufferOfs + 24);
	const uint materialBufferOfs = materialBufferIndex * MeshMaterialDescSize;
	float3 baseColor = asfloat(g_MaterialDescBuffer.Load3(materialBufferOfs + 0));

	// transform
	float3 worldPos = mul(instanceTransform, float4(input.Pos.xyz, 1.0)); // order changed because input matrix is column major
	float3 worldNorm = normalize(mul(instanceTransform, float4(input.Normal, 0.0)));
	float3 worldTangent = normalize(mul(instanceTransform, float4(input.Tangent, 0.0)));

	// output
	output.Pos = mul(float4(worldPos, 1.0), g_SceneCB.ViewProj);
	output.Normal = worldNorm;
	output.Tangent = worldTangent;
	output.TexCoord = float2(input.TexCoord.x, 1.0 - input.TexCoord.y);
	output.Color = baseColor;

	return output;
}
