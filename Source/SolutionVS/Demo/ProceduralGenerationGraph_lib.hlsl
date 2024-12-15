
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

bool AcquirePartitionDataNodes(out uint nodeIds[PartitionTetrahedraSubNodeCount])
{
	uint nodesCounter;
	g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, PartitionTetrahedraSubNodeCount, nodesCounter);
	if (nodesCounter >= PartitionNodeCountMax)
	{
		g_PartitionData.InterlockedAdd(PartitionDataNodesCounterOfs, -PartitionTetrahedraSubNodeCount);
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
	[unroll] for (uint isub = 0; isub < PartitionTetrahedraSubNodeCount; ++isub)
	{
		nodeData.SubNodeIds[isub] = g_PartitionData.Load(nodeDataOfs + isub * PartitionNodeIdSize);
	}
}

void StorePartitionDataSubNodeIds(in uint nodeIdx, in uint subNodeIds[PartitionTetrahedraSubNodeCount])
{
	uint nodeDataOfs = PartitionDataNodesOfs + nodeIdx * PartitionDataNodesStride + PartitionTetrahedraSize;
	[unroll] for (uint isub = 0; isub < PartitionTetrahedraSubNodeCount; ++isub)
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
	bool doSplit = (PartitionMode::Split == partitionMode); // skip split test if parent node is merged
	if (doSplit)
	{
		#if (0)

		float3 edgeVec = nodeData.TetrahedraVertices[1] - nodeData.TetrahedraVertices[0];
		float edgeLenSq = dot(edgeVec.xyz, edgeVec.xyz);
		float3 edgeCenter = (nodeData.TetrahedraVertices[0] + nodeData.TetrahedraVertices[1]) * 0.5;
		#if (1)
		float3 vecToRefPoint = g_ProceduralConsts.RefinementPoint.xyz - edgeCenter;
		#else
		float3 tetrahedraCenter = 0;
		[unroll] for (uint i = 0; i < 4; ++i)
		{
			tetrahedraCenter += nodeData.TetrahedraVertices[i].xyz;
		}
		float3 vecToRefPoint = g_ProceduralConsts.RefinementPoint.xyz - tetrahedraCenter / 4;
		#endif
		float distToRefPointSq = dot(vecToRefPoint.xyz, vecToRefPoint.xyz);
		doSplit = (distToRefPointSq < edgeLenSq * g_ProceduralConsts.RefinementDistanceFactor);

		if (doSplit && nodeData.SubNodeIds[0] == InvalidIndex)
		{
			// split: create sub nodes and proceed with parition update

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
			#ifdef STATIC_CONSTS_WORKAROUND
			const uint3 subdivIds0 = g_ProceduralConsts.SubDivisionIndices[subdivTypeId * PartitionTetrahedraSubNodeCount + 0].xyz;
			const uint3 subdivIds1 = g_ProceduralConsts.SubDivisionIndices[subdivTypeId * PartitionTetrahedraSubNodeCount + 1].xyz;
			#else
			const uint3 subdivIds0 = PartitionTetrahedraSubDivisionIndices[subdivTypeId * PartitionTetrahedraSubNodeCount + 0];
			const uint3 subdivIds1 = PartitionTetrahedraSubDivisionIndices[subdivTypeId * PartitionTetrahedraSubNodeCount + 1];
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

		#else

		// TODO: LEB based tetrahedra sub division
		// find longest edge in tetrahedra

		uint longestEdgeId = 0;
		float longestEdgeLenSq = 0;
		[unroll] for (uint edgeId = 0; edgeId < PartitionTetrahedraEdgeCount; ++edgeId)
		{
			#ifdef STATIC_CONSTS_WORKAROUND
			const uint2 edgeVertIds = g_ProceduralConsts.TetrahedraEdges[edgeId].xy;
			#else
			const uint2 edgeVertIds = PartitionTetrahedraEdges[edgeId];
			#endif
			const float3 edgeVec = nodeData.TetrahedraVertices[edgeVertIds[1]] - nodeData.TetrahedraVertices[edgeVertIds[0]];
			const float edgeLenSq = dot(edgeVec, edgeVec);
			if (edgeLenSq > longestEdgeLenSq)
			{
				longestEdgeId = edgeId;
				longestEdgeLenSq = edgeLenSq;
			}
		}

		#ifdef STATIC_CONSTS_WORKAROUND
		const uint2 splitEdgeIds = g_ProceduralConsts.TetrahedraEdges[longestEdgeId].xy;
		#else
		const uint2 splitEdgeIds = PartitionTetrahedraEdges[longestEdgeId];
		#endif
		float3 splitEdgeVertices[2] = {
			nodeData.TetrahedraVertices[splitEdgeIds[0]],
			nodeData.TetrahedraVertices[splitEdgeIds[1]]
		};
		float3 splitPoint = splitEdgeVertices[0] * 0.5 + splitEdgeVertices[1] * 0.5;

		// refinement distance check

		#if (0)
		float3 vecToRefPoint = g_ProceduralConsts.RefinementPoint.xyz - splitPoint;
		#else
		float3 tetrahedraCenter = 0;
		[unroll] for (uint i = 0; i < 4; ++i)
		{
			tetrahedraCenter += nodeData.TetrahedraVertices[i].xyz;
		}
		float3 vecToRefPoint = g_ProceduralConsts.RefinementPoint.xyz - tetrahedraCenter / 4;
		#endif
		float distToRefPointSq = dot(vecToRefPoint.xyz, vecToRefPoint.xyz);
		doSplit = (distToRefPointSq < longestEdgeLenSq * g_ProceduralConsts.RefinementDistanceFactor);

		if (doSplit && nodeData.SubNodeIds[0] == InvalidIndex)
		{
			// split: create sub nodes and proceed with parition update

			// allocate
			bool nodesAcquired = AcquirePartitionDataNodes(nodeData.SubNodeIds);
			if (!nodesAcquired)
			{
				// assert(nodeData.SubNodeIds[0] == InvalidIndex); // reserved nodes count must be even
				return PartitionMode::Merge; // interrupt: out of memeory
			}

			// store refernce ids to new sub nodes
			StorePartitionDataSubNodeIds(nodeId, nodeData.SubNodeIds);

			/*AddToDebugOutput(888);
			for (uint i = 0; i < PartitionTetrahedraEdgeCount * PartitionTetrahedraSubNodeCount; ++i)
			{
				AddToDebugOutput(g_ProceduralConsts.TetrahedraEdgeSplitInfo[i]);
			}*/
			#if (1)
			AddToDebugOutput(uint4(777, longestEdgeId, splitEdgeIds[0], splitEdgeIds[1]));
			[unroll] for (uint subId = 0; subId < PartitionTetrahedraSubNodeCount; ++subId)
			{
				#ifdef STATIC_CONSTS_WORKAROUND
				uint4 subTetrahedraIds = g_ProceduralConsts.TetrahedraEdgeSplitInfo[longestEdgeId * PartitionTetrahedraSubNodeCount + subId];
				#else
				uint4 subTetrahedraIds = PartitionTetrahedraEdgeSplitInfo[longestEdgeId * PartitionTetrahedraSubNodeCount + subId];
				#endif
				PartitionNodeData subNodeData = InitialNodeData();
				[unroll] for (uint vid = 0; vid < 4; ++vid)
				{
					subNodeData.TetrahedraVertices[vid] = (subTetrahedraIds[vid] != 0xff ? nodeData.TetrahedraVertices[subTetrahedraIds[vid]] : splitPoint);
				}
				StorePartitionDataNode(nodeData.SubNodeIds[subId], subNodeData);
				AddToDebugOutput(600 + subId);
				AddToDebugOutput(subTetrahedraIds);
			}
			#endif
		}

		#endif
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

	ThreadNodeOutputRecords<PartitionUpdateRecord> subNodeRecords = partitionNodeOutput.GetThreadNodeOutputRecords(PartitionTetrahedraSubNodeCount);

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
	[MaxRecords(PartitionTetrahedraSubNodeCount)] NodeOutput<PartitionUpdateRecord> PartitionUpdateNode)
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