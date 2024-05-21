
#include "ProceduralGenerationGraph.hlsli"

// TODO: move to common
uint LoadPartitionDataNodesCounter()
{
	return g_PartitionData.Load(PartitionDataNodesCounterOfs);
}

// TODO: move to common
void LoadPartitionDataNode(in uint nodeIdx, out PartitionNodeData nodeData)
{
	uint nodeDataOfs = PartitionDataNodesOfs + nodeIdx * PartitionDataNodesStride;
	[unroll] for (uint iv = 0; iv < 4; ++iv)
	{
		nodeData.TetrahedraVertices[iv] = asfloat(g_PartitionData.Load3(nodeDataOfs + iv * PartitionTetrahedraVertexSize));
	}
	nodeDataOfs += PartitionTetrahedraSize;
	[unroll] for (uint isub = 0; isub < 2; ++isub)
	{
		nodeData.SubNodeIds[isub] = g_PartitionData.Load(nodeDataOfs + isub * PartitionNodeIdSize);
	}
}

struct StructureVSOutput
{
	float4 Pos : SV_Position0;
	float4 Color : Color0;
};

struct StructurePSOutput
{
	float4 Color : SV_Target0;
};

static const uint VertexIdToTetrahedra[12] = { 0,1, 0,2, 0,3, 1,2, 2,3, 3,1 };

[Shader("vertex")]
StructureVSOutput PartitionStructureVS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	StructureVSOutput output = (StructureVSOutput)0;

	uint nodesCount = LoadPartitionDataNodesCounter();
	if (instanceID >= nodesCount)
		return output; // discard

	PartitionNodeData nodeData;
	LoadPartitionDataNode(instanceID, nodeData);
	if (nodeData.SubNodeIds[0] != InvalidIndex)
		return output; // discard: not a leaf node

	float3 worldPos = nodeData.TetrahedraVertices[VertexIdToTetrahedra[vertexID % 12]].xyz;
	
	output.Pos = mul(float4(worldPos.xyz, 1.0), g_SceneRenderConsts.ViewProj);
	output.Color = (vertexID < 2 ? float4(1.0, 0.0, 0.0, 1.0) : float4(1.0, 1.0, 0.0, 1.0));

	return output;
}

[Shader("pixel")]
StructurePSOutput PartitionStructurePS(StructureVSOutput input)
{
	StructurePSOutput output = (StructurePSOutput)0;

	output.Color = input.Color;

	return output;
}