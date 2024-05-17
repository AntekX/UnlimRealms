#ifndef PROCEDURAL_GENERATION_GRAPH_HLSLI
#define PROCEDURAL_GENERATION_GRAPH_HLSLI

#include "ShaderLib/CommonTypes.hlsli"

// global constants

static const CUINT(PartitionDepthMax) = 10;
static const CUINT(PartitionRootTetrahedraCount) = 6; // 6 tetrahedra forming root cube space
static const CUINT(PartitionTetrahedraCountMax) = PartitionRootTetrahedraCount * 1024; // 6 root tetrahedra * 2^PartitionDepthMax
static const CUINT(PartitionTetrahedraVertexSize) = 12; // 3 * sizeof(float)
static const CUINT(PartitionTetrahedraSize) = 4 * PartitionTetrahedraVertexSize; // 4 vertices x size
static const CUINT(PartitionNodeCountMax) = PartitionTetrahedraCountMax;
static const CUINT(PartitionNodeIdSize) = 4;
static const CUINT(InvalidIndex) = -1;

// 6 root tetrahedra vertices in [-1,1] space scaled by RootExtent
// vertices [0] and [1] designate an edge to be split
static const CFLOAT3(PartitionRootVertices)[8] = {
	{-1.0f,-1.0f,-1.0f }, {+1.0f,-1.0f,-1.0f }, {-1.0f,+1.0f,-1.0f }, {+1.0f,+1.0f,-1.0f },
	{-1.0f,-1.0f,+1.0f }, {+1.0f,-1.0f,+1.0f }, {-1.0f,+1.0f,+1.0f }, {+1.0f,+1.0f,+1.0f }
};
#define PRV PartitionRootVertices
static const CFLOAT3(PartitionRootTetrahedra)[PartitionRootTetrahedraCount][4] = {
	{ PRV[0], PRV[7], PRV[2], PRV[3] },
	{ PRV[0], PRV[7], PRV[3], PRV[1] },
	{ PRV[0], PRV[7], PRV[1], PRV[5] },
	{ PRV[0], PRV[7], PRV[5], PRV[4] },
	{ PRV[0], PRV[7], PRV[4], PRV[6] },
	{ PRV[0], PRV[7], PRV[6], PRV[2] }
};
#undef PRV

struct PartitionNodeData
{
	CFLOAT3(TetrahedraVertices)[4];
	CUINT(SubNodeIds)[2];
};
static const CUINT(InvalidSubNodeIds)[2] = { InvalidIndex, InvalidIndex };
static const CUINT(PartitionNodeDataSize) = PartitionTetrahedraSize + PartitionNodeIdSize * 2; // sizeof(PartitionNodeData)

// partition data buffer layout
// [nodesCounter] [freeNodeIdx: 0...PartitionNodeCountMax-1] [nodeData: 0...PartitionNodeCountMax-1]
static const CUINT(PartitionDataNodesCounterOfs) = 0;
static const CUINT(PartitionDataNodesCounterSize) = 4;
static const CUINT(PartitionDataFreeNodesOfs) = PartitionDataNodesCounterOfs + PartitionDataNodesCounterSize;
static const CUINT(PartitionDataFreeNodesStride) = 4;
static const CUINT(PartitionDataFreeNodesSize) = PartitionNodeCountMax * PartitionDataFreeNodesStride;
static const CUINT(PartitionDataNodesOfs) = PartitionDataFreeNodesOfs + PartitionDataFreeNodesSize;
static const CUINT(PartitionDataNodesStride) = PartitionNodeDataSize;
static const CUINT(PartitionDataNodesSize) = PartitionNodeCountMax * PartitionDataNodesStride;
static const CUINT(PartitionDataDebugOfs) = PartitionDataNodesOfs + PartitionDataNodesSize;
static const CUINT(PartitionDataDebugSize) = 1024;
static const CUINT(PartitionDataBufferSize) = PartitionDataDebugOfs + PartitionDataDebugSize;

// constant buffers

struct ProceduralConsts
{
	CFLOAT3(RootExtent);
	CFLOAT3(RootPosition);
	CFLOAT3(RefinementPoint);
	CFLOAT(RefinementDistanceFactor);
	CFLOAT2(__pad0);
};

// work graph records

struct PartitionMode
{
	static const CUINT(Merge) = 0;
	static const CUINT(Split) = 1;
};

struct PartitionUpdateRecord
{
	CUINT(Mode); // PartitionMode
	CUINT(ParentNodeId);
	CUINT(NodeId);
};

// descriptors

DESCRIPTOR_ConstantBuffer(ProceduralConsts, g_ProceduralConsts, 0);
DESCRIPTOR_RWByteAddressBuffer(g_PartitionData, 0);

#endif // PROCEDURAL_GENERATION_GRAPH_HLSLI