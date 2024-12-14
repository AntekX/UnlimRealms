
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
	g_PartitionData.InterlockedExchange(PartitionDataFreeNodesOfs + nodesCounter * PartitionDataFreeNodesStride, InvalidIndex, nodeIdx);
	return true;
}

bool AcquirePartitionDataNodes(out uint nodeIds[2])
{
	uint nodesCounter;
	g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, 2, nodesCounter);
	if (nodesCounter >= PartitionNodeCountMax)
	{
		g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, -2);
		nodeIds[0] = InvalidIndex;
		nodeIds[1] = InvalidIndex;
		return false;
	}
	g_PartitionData.InterlockedExchange(PartitionDataFreeNodesOfs + (nodesCounter + 0) * PartitionDataFreeNodesStride, InvalidIndex, nodeIds[0]);
	g_PartitionData.InterlockedExchange(PartitionDataFreeNodesOfs + (nodesCounter + 1) * PartitionDataFreeNodesStride, InvalidIndex, nodeIds[1]);
	return true;
}

void ReleasePartitionDataNode(in uint nodeIdx)
{
	uint nodesCounter;
	g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, -1, nodesCounter);
	// assert(nodesCounter > 0);
	g_PartitionData.Store(PartitionDataFreeNodesOfs + (nodesCounter - 1) * PartitionDataFreeNodesStride, nodeIdx);
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
		nodeData.TetrahedraVertices[iv] = asfloat(g_PartitionData.Load3(nodeDataOfs + iv * PartitionTetrahedraVertexSize));
	}
	nodeDataOfs += PartitionTetrahedraSize;
	[unroll] for (uint isub = 0; isub < 2; ++isub)
	{
		nodeData.SubNodeIds[isub] = g_PartitionData.Load(nodeDataOfs + isub * PartitionNodeIdSize);
	}
}

void StorePartitionDataSubNodeIds(in uint nodeIdx, in uint subNodeIds[2])
{
	uint nodeDataOfs = PartitionDataNodesOfs + nodeIdx * PartitionDataNodesStride + PartitionTetrahedraSize;
	[unroll] for (uint isub = 0; isub < 2; ++isub)
	{
		g_PartitionData.Store(nodeDataOfs + isub * PartitionNodeIdSize, subNodeIds[isub]);
	}
}

void StorePartitionDataNode(in uint nodeIdx, in PartitionNodeData nodeData)
{
	uint nodeDataOfs = PartitionDataNodesOfs + nodeIdx * PartitionDataNodesStride;
	[unroll] for (uint iv = 0; iv < 4; ++iv)
	{
		g_PartitionData.Store3(nodeDataOfs + iv * PartitionTetrahedraVertexSize, asuint(nodeData.TetrahedraVertices[iv]));
	}
	StorePartitionDataSubNodeIds(nodeIdx, nodeData.SubNodeIds);
}

void ResetDebugOutput()
{
	uint outputIdx;
	g_PartitionData.InterlockedExchange(PartitionDataDebugOfs, 0, outputIdx);
}

void AddToDebugOutput(in uint value)
{
	uint outputIdx;
	g_PartitionData.InterlockedAdd(PartitionDataDebugOfs, 1, outputIdx);
	uint writePos = outputIdx * 4;
	if (writePos + 8 < PartitionDataDebugSize)
		g_PartitionData.Store(PartitionDataDebugOfs + 4 + writePos, value);
}

void AddToDebugOutput(in uint4 value)
{
	uint outputIdx;
	g_PartitionData.InterlockedAdd(PartitionDataDebugOfs, 4, outputIdx);
	uint writePos = outputIdx * 4;
	if (writePos + 20 < PartitionDataDebugSize)
		g_PartitionData.Store4(PartitionDataDebugOfs + 4 + writePos, value);
}

uint UpdateNodePartition(in uint partitionMode, in uint partitionDepth, in uint nodeId, inout PartitionNodeData nodeData)
{
	AddToDebugOutput(partitionDepth);
	bool doSplit = (PartitionMode::Split == partitionMode); // skip split test if parent node is merged
	if (doSplit)
	{
		float3 edgeVec = nodeData.TetrahedraVertices[1] - nodeData.TetrahedraVertices[0];
		float edgeLenSq = dot(edgeVec.xyz, edgeVec.xyz);
		float3 edgeCenter = (nodeData.TetrahedraVertices[0] + nodeData.TetrahedraVertices[1]) * 0.5;
		float3 edgeToRefVec = g_ProceduralConsts.RefinementPoint.xyz - edgeCenter;
		float edgeToRefDistSq = dot(edgeToRefVec.xyz, edgeToRefVec.xyz);
		doSplit = (edgeToRefDistSq < edgeLenSq * g_ProceduralConsts.RefinementDistanceFactor);

		if (doSplit && nodeData.SubNodeIds[0] == InvalidIndex)
		{
			// split: create 2 sub nodes and proceed with parition update

			// allocate
			bool nodesAcquired = AcquirePartitionDataNodes(nodeData.SubNodeIds);
			if (!nodesAcquired)
			{
				// assert(nodeData.SubNodeIds[0] == InvalidIndex); // reserved nodes count must be even
				return PartitionMode::Merge; // interrupt: out of memeory
			}

			// store refernce ids to new sub nodes
			StorePartitionDataSubNodeIds(nodeId, nodeData.SubNodeIds);

			// compute & store tetrahedra vertices
			PartitionNodeData subNodeData = InitialNodeData();

			const uint subdivTypeId = (partitionDepth % PartitionTetrahedraSubDivisionTypes);
			#ifdef SUB_DIVISON_INDICES_WORKAROUND
			const uint3 subdivIds0 = g_ProceduralConsts.SubDivisionIndices[subdivTypeId * 2 + 0].xyz;
			const uint3 subdivIds1 = g_ProceduralConsts.SubDivisionIndices[subdivTypeId * 2 + 1].xyz;
			#else
			const uint3 subdivIds0 = PartitionTetrahedraSubDivisionIndices[subdivTypeId * 2 + 0];
			const uint3 subdivIds1 = PartitionTetrahedraSubDivisionIndices[subdivTypeId * 2 + 1];
			#endif

			subNodeData.TetrahedraVertices[0] = nodeData.TetrahedraVertices[subdivIds0[0]];
			subNodeData.TetrahedraVertices[1] = nodeData.TetrahedraVertices[subdivIds0[1]];
			subNodeData.TetrahedraVertices[2] = nodeData.TetrahedraVertices[subdivIds0[2]];
			subNodeData.TetrahedraVertices[3] = edgeCenter;
			StorePartitionDataNode(nodeData.SubNodeIds[0], subNodeData);

			subNodeData.TetrahedraVertices[0] = nodeData.TetrahedraVertices[subdivIds1[0]];
			subNodeData.TetrahedraVertices[1] = nodeData.TetrahedraVertices[subdivIds1[1]];
			subNodeData.TetrahedraVertices[2] = nodeData.TetrahedraVertices[subdivIds1[2]];
			subNodeData.TetrahedraVertices[3] = edgeCenter;
			StorePartitionDataNode(nodeData.SubNodeIds[1], subNodeData);
		}
	}

	if (!doSplit && nodeData.SubNodeIds[0] != InvalidIndex)
	{
		// merge: invalidate references to sub nodes
		// sub nodes data will be released further down the graph

		StorePartitionDataSubNodeIds(nodeId, InvalidSubNodeIds);
	}

	return (doSplit ? PartitionMode::Split : PartitionMode::Merge);
}

void OutputNodePartition(NodeOutput<PartitionUpdateRecord> partitionNodeOutput, in uint partitionMode, in uint nodeId, in PartitionNodeData nodeData)
{
	if (InvalidIndex == nodeData.SubNodeIds[0])
		return; // leaf node reached, interrupt traversal

	ThreadNodeOutputRecords<PartitionUpdateRecord> subNodeRecords = partitionNodeOutput.GetThreadNodeOutputRecords(2);

	subNodeRecords.Get(0).Mode = partitionMode;
	subNodeRecords.Get(0).ParentNodeId = nodeId;
	subNodeRecords.Get(0).NodeId = nodeData.SubNodeIds[0];
	subNodeRecords.Get(1).Mode = partitionMode;
	subNodeRecords.Get(1).ParentNodeId = nodeId;
	subNodeRecords.Get(1).NodeId = nodeData.SubNodeIds[1];

	subNodeRecords.OutputComplete();
}

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NodeDispatchGrid(1, 1, 1)]
[NumThreads(PartitionRootTetrahedraCount, 1, 1)]
void PartitionUpdateRootNode(
	[MaxRecords(PartitionRootOutputRecordsMax)] NodeOutput<PartitionUpdateRecord> PartitionUpdateNode,
	uint3 dispatchID : SV_DispatchThreadID)
{
	if (WaveIsFirstLane())
	{
		ResetDebugOutput();
	}
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
			nodeData.TetrahedraVertices[iv] = PartitionRootTetrahedra[nodeIdx][iv] * g_ProceduralConsts.RootExtent + g_ProceduralConsts.RootPosition.xyz;
		}
		AcquirePartitionDataNode(); // returned idx does not matter, every root node writes at it's constant pos
		StorePartitionDataNode(nodeIdx, nodeData);
		GroupMemoryBarrierWithGroupSync(); // wait all root nodes to be stored
	}
	else
	{
		// read root node data
		LoadPartitionDataNode(nodeIdx, nodeData);
	}

	// update root node partition

	uint partitionMode = PartitionMode::Split; // always try to split root node
	uint partitionDepth = 0;
	partitionMode = UpdateNodePartition(partitionMode, partitionDepth, nodeIdx, nodeData);

	// produce output

	OutputNodePartition(PartitionUpdateNode, partitionMode, nodeIdx, nodeData);
}

[Shader("node")]
[NodeLaunch("thread")]
[NodeMaxRecursionDepth(PartitionDepthMax)]
void PartitionUpdateNode(
	ThreadNodeInputRecord<PartitionUpdateRecord> inputData,
	[MaxRecords(2)] NodeOutput<PartitionUpdateRecord> PartitionUpdateNode)
{
	PartitionUpdateRecord partitionInput = inputData.Get();

	// load node data

	PartitionNodeData nodeData = InitialNodeData();
	LoadPartitionDataNode(partitionInput.NodeId, nodeData);

	if (PartitionMode::Merge == partitionInput.Mode)
	{
		// if parent is merged - release this node data after loading to properly free it's sub nodes
		ReleasePartitionDataNode(partitionInput.NodeId);
	}

	if (GetRemainingRecursionLevels() > 0)
	{
		// update partition

		uint partitionDepth = PartitionDepthMax - GetRemainingRecursionLevels() + 1;
		uint partitionMode = UpdateNodePartition(partitionInput.Mode, partitionDepth, partitionInput.NodeId, nodeData);

		// produce output

		OutputNodePartition(PartitionUpdateNode, partitionMode, partitionInput.NodeId, nodeData);
	}
}