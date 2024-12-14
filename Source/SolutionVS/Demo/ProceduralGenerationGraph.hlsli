#ifndef PROCEDURAL_GENERATION_GRAPH_HLSLI
#define PROCEDURAL_GENERATION_GRAPH_HLSLI

#include "ShaderLib/CommonTypes.hlsli"

#if !defined(CPP_CODE)
#pragma pack_matrix(row_major)
#endif

// global constants

static const CUINT(PartitionDepthMax) = 16;
static const CUINT(PartitionRootTetrahedraCount) = 6; // 6 tetrahedra forming root cube
static const CUINT(PartitionRootOutputRecordsMax) = PartitionRootTetrahedraCount * 2;
static const CUINT(PartitionTetrahedraCountMax) = PartitionRootTetrahedraCount * 65536; // 6 root tetrahedra * 2^PartitionDepthMax
static const CUINT(PartitionTetrahedraVertexSize) = 12; // 3 * sizeof(float)
static const CUINT(PartitionTetrahedraSize) = 4 * PartitionTetrahedraVertexSize; // 4 vertices x size
static const CUINT(PartitionNodeCountMax) = PartitionTetrahedraCountMax;
static const CUINT(PartitionNodeIdSize) = 4;
static const CUINT(InvalidIndex) = -1;

// root tetrahedra vertices in [-1,1]
// vertices [0] and [1] designate an edge to be split, while [2] and [3] are it's only non adjacent edge (opposite)
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

// tetrahedra sub divison indexation
// TODO: workaround, passing indexation consts via CB as fetching from PartitionTetrahedraSubDivisionIndices gives invalid results, seems to be a compiler issue
#define SUB_DIVISON_INDICES_WORKAROUND
static const CUINT(PartitionTetrahedraSubDivisionTypes) = 3; // number of unique sub divisions before arriving to scaled down version of source shape
static const CUINT3(PartitionTetrahedraSubDivisionIndices[PartitionTetrahedraSubDivisionTypes * 2]) = {
	{ 1, 2, 3 }, { 0, 3, 2 },
	{ 2, 1, 3 }, { 0, 2, 3 },
	{ 2, 1, 3 }, { 0, 2, 3 }
};

struct PartitionNodeData
{
	CFLOAT3(TetrahedraVertices)[4];
	CUINT(SubNodeIds)[2];
};
static const CUINT(InvalidSubNodeIds)[2] = { InvalidIndex, InvalidIndex };
static const CUINT(PartitionNodeDataSize) = PartitionTetrahedraSize + PartitionNodeIdSize * 2; // sizeof(PartitionNodeData)

// partition data buffer layout
// [nodesCounter] [freeNodeIdx: 0...PartitionNodeCountMax-1] [nodeData: 0...PartitionNodeCountMax-1]
static const CUINT(PartitionDataDebugOfs) = 0;
static const CUINT(PartitionDataDebugSize) = 1024;
static const CUINT(PartitionDataNodesCounterOfs) = PartitionDataDebugOfs + PartitionDataDebugSize;
static const CUINT(PartitionDataNodesCounterSize) = 4;
static const CUINT(PartitionDataFreeNodesOfs) = PartitionDataNodesCounterOfs + PartitionDataNodesCounterSize;
static const CUINT(PartitionDataFreeNodesStride) = 4;
static const CUINT(PartitionDataFreeNodesSize) = PartitionNodeCountMax * PartitionDataFreeNodesStride;
static const CUINT(PartitionDataNodesOfs) = PartitionDataFreeNodesOfs + PartitionDataFreeNodesSize;
static const CUINT(PartitionDataNodesStride) = PartitionNodeDataSize;
static const CUINT(PartitionDataNodesSize) = PartitionNodeCountMax * PartitionDataNodesStride;
static const CUINT(PartitionDataBufferSize) = PartitionDataNodesOfs + PartitionDataNodesSize;

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

// constant buffers

struct ProceduralConsts
{
	CFLOAT3(RootPosition);
	CFLOAT(RootExtent);
	CFLOAT3(RefinementPoint);
	CFLOAT(RefinementDistanceFactor);
	//CFLOAT4(__pad0);
	#ifdef SUB_DIVISON_INDICES_WORKAROUND
	CUINT4(SubDivisionIndices[PartitionTetrahedraSubDivisionTypes * 2]);
	#endif
};

struct SceneRenderConsts
{
	CFLOAT4x4(ViewProj);
};

// descriptors

DESCRIPTOR_ConstantBuffer(ProceduralConsts, g_ProceduralConsts, 0);
DESCRIPTOR_ConstantBuffer(SceneRenderConsts, g_SceneRenderConsts, 0);
DESCRIPTOR_RWByteAddressBuffer(g_PartitionData, 0);

#endif // PROCEDURAL_GENERATION_GRAPH_HLSLI