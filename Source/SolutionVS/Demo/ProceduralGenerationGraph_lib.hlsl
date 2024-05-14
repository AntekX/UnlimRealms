
#include "ProceduralGenerationGraph.hlsli"

PartitionNodeData InitialNodeData()
{
	PartitionNodeData nodeData = (PartitionNodeData)0;
	nodeData.SubNodeIds[0] = InvalidIndex;
	nodeData.SubNodeIds[1] = InvalidIndex;
	return nodeData;
}

bool AcquirePartitionDataNode(out uint nodeIdx = InvalidIndex)
{
	uint nodesCounter;
	g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, 1, nodesCounter);
	if (nodesCounter >= PartitionNodeCountMax)
	{
		g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, -1);
		nodeIdx = InvalidIndex;
		return false;
	}
	nodeIdx = g_PartitionData.Load(PartitionDataFreeNodesOfs + nodesCounter * PartitionDataFreeNodesStride);
	return true;
}

void ReleasePartitionDataNode(in uint nodeIdx)
{
	uint nodesCounter;
	g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, -1, nodesCounter);
	// assert(nodesCounter > 0);
	g_PartitionData.Store(PartitionDataFreeNodesOfs + nodesCounter * PartitionDataFreeNodesStride, nodeIdx);
}

uint LoadPartitionDataNodesCounter()
{
	return g_PartitionData.Load(PartitionDataNodesCounterOfs);
}

void LoadPartitionDataNode(in uint nodeIdx, out PartitionNodeData nodeData)
{
	uint nodeDataOfs = PartitionDataNodesOfs + nodeIdx * PartitionDataNodesStride;
	[unroll] for (uint iv = 0; iv < 4; ++iv)
	{
		nodeData.TetrahedraVertices[iv] = asfloat(g_PartitionData.Load(nodeDataOfs + iv * PartitionTetrahedraVertexSize));
	}
	nodeDataOfs += PartitionTetrahedraSize;
	[unroll] for (uint isub = 0; isub < 2; ++isub)
	{
		nodeData.SubNodeIds[isub] = g_PartitionData.Load(nodeDataOfs + isub * PartitionNodeIdSize);
	}
}

void StorePartitionDataNode(in uint nodeIdx, in PartitionNodeData nodeData)
{
	uint nodeDataOfs = PartitionDataNodesOfs + nodeIdx * PartitionDataNodesStride;
	[unroll] for (uint iv = 0; iv < 4; ++iv)
	{
		g_PartitionData.Store(nodeDataOfs + iv * PartitionTetrahedraVertexSize, asuint(nodeData.TetrahedraVertices[iv]));
	}
	nodeDataOfs += PartitionTetrahedraSize;
	[unroll] for (uint isub = 0; isub < 2; ++isub)
	{
		g_PartitionData.Store(nodeDataOfs + isub * PartitionNodeIdSize, nodeData.SubNodeIds[isub]);
	}
}

void UpdateNodePartition(NodeOutput<PartitionUpdateRecord> partitionNodeOutput, in uint nodeId, in PartitionNodeData nodeData)
{
	float3 edgeVec = nodeData.TetrahedraVertices[1] - nodeData.TetrahedraVertices[0];
	float edgeLenSq = dot(edgeVec.xyz, edgeVec.xyz);
	float3 edgeCenter = (nodeData.TetrahedraVertices[0] + nodeData.TetrahedraVertices[1]) * 0.5;
	float3 edgeToRefVec = g_ProceduralConsts.RefinementPoint.xyz - edgeCenter;
	float edgeToRefDistSq = dot(edgeToRefVec.xyz, edgeToRefVec.xyz);
	bool doSplit = (edgeToRefDistSq < edgeLenSq * g_ProceduralConsts.RefinementDistanceFactor);

	// TODO: check PartitionMode coming from input record (parent), skip split in Merge mode
	if (doSplit && nodeData.SubNodeIds[0] == InvalidIndex)
	{
		// split: create 2 sub nodes and proceed with parition update

		// allocate
		uint allocatedCount = 0;
		[unroll] for (uint isubNode = 0; isubNode < 2; ++isubNode)
		{
			allocatedCount += (uint)AcquirePartitionDataNode(nodeData.SubNodeIds[isubNode]);
		}
		if (allocatedCount < 2)
		{
			// assert(nodeData.SubNodeIds[0] == InvalidIndex); // reserved nodes count must be even
			return; // interrupt: out of memeory
		}

		// compute & store tetrahedra vertices
		PartitionNodeData subNodeData = InitialNodeData();

		subNodeData.TetrahedraVertices[0] = nodeData.TetrahedraVertices[0];
		subNodeData.TetrahedraVertices[1] = nodeData.TetrahedraVertices[3];
		subNodeData.TetrahedraVertices[2] = nodeData.TetrahedraVertices[2];
		subNodeData.TetrahedraVertices[3] = edgeCenter;
		StorePartitionDataNode(nodeData.SubNodeIds[0], subNodeData);

		subNodeData.TetrahedraVertices[0] = nodeData.TetrahedraVertices[3];
		subNodeData.TetrahedraVertices[1] = nodeData.TetrahedraVertices[1];
		subNodeData.TetrahedraVertices[2] = nodeData.TetrahedraVertices[2];
		subNodeData.TetrahedraVertices[3] = edgeCenter;
		StorePartitionDataNode(nodeData.SubNodeIds[1], subNodeData);
	}

	if (!doSplit && nodeData.SubNodeIds[0] != InvalidIndex)
	{
		// merge: produce output to release sub nodes
		// TODO
	}

	// output
	// TODO: move to separate function?
	if (nodeData.SubNodeIds[0] != InvalidIndex)
	{
		ThreadNodeOutputRecords<PartitionUpdateRecord> subNodeRecords = partitionNodeOutput.GetThreadNodeOutputRecords(2);

		PartitionUpdateRecord subNodeRecord0 = subNodeRecords.Get(0);
		subNodeRecord0.Mode = (doSplit ? PartitionMode::Split : PartitionMode::Merge);
		subNodeRecord0.ParentNodeId = nodeId;
		subNodeRecord0.NodeId = nodeData.SubNodeIds[0];

		PartitionUpdateRecord subNodeRecord1 = subNodeRecords.Get(1);
		subNodeRecord1.Mode = (doSplit ? PartitionMode::Split : PartitionMode::Merge);
		subNodeRecord1.ParentNodeId = nodeId;
		subNodeRecord0.NodeId = nodeData.SubNodeIds[1];

		subNodeRecords.OutputComplete();
	}
}

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NodeDispatchGrid(1, 1, 1)]
[NumThreads(PartitionRootTetrahedraCount, 1, 1)]
void PartitionUpdateRootNode(
	[MaxRecords(2)] NodeOutput<PartitionUpdateRecord> PartitionUpdateNode,
	uint3 dispatchID : SV_DispatchThreadID)
{	
	uint partitionTetrahedraCount = LoadPartitionDataNodesCounter();
	GroupMemoryBarrierWithGroupSync(); // all threads must read counter first

	// initialize root node data (load if available or compute inplace and store)

	const uint nodeIdx = dispatchID.x;
	PartitionNodeData nodeData = InitialNodeData();
	if (partitionTetrahedraCount < PartitionRootTetrahedraCount)
	{
		// compute vertices in world space
		[unroll] for (uint iv = 0; iv < 4; ++iv)
		{
			nodeData.TetrahedraVertices[iv] = PartitionRootTetrahedra[nodeIdx][iv] * g_ProceduralConsts.RootExtent.xyz + g_ProceduralConsts.RootPosition.xyz;
		}
		AcquirePartitionDataNode(); // returned idx does not matter, every root node writes at it's constant pos
		StorePartitionDataNode(nodeIdx, nodeData);
		GroupMemoryBarrierWithGroupSync(); // wait all root nodes to be stores
	}
	else
	{
		// read root node data
		LoadPartitionDataNode(nodeIdx, nodeData);
	}

	// update root node partition

	UpdateNodePartition(PartitionUpdateNode, nodeIdx, nodeData);
}

[Shader("node")]
[NodeLaunch("thread")]
void PartitionUpdateNode(
	ThreadNodeInputRecord<PartitionUpdateRecord> inputData,
	[MaxRecords(2)] NodeOutput<PartitionUpdateRecord> PartitionUpdateNode)
{
	//UpdateNodePartition(PartitionUpdateNode, tetrahedraVertices);
}