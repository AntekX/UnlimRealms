#ifndef PROCEDURAL_GENERATION_GRAPH_HLSLI
#define PROCEDURAL_GENERATION_GRAPH_HLSLI

#include "ShaderLib/CommonTypes.hlsli"

// global constants

static const CUINT(PartitionDepthMax) = 10;
static const CUINT(PartitionTetrahedraMax) = 4 * 1024; // 4 root tetrahedra * 2^PartitionDepthMax
static const CUINT(PartitionTetrahedraVertexSize) = 12; // 3 * sizeof(float)
static const CUINT(PartitionTetrahedraSize) = PartitionTetrahedraVertexSize * 4; // 4 vertices x sizeof(Vertex)
static const CUINT(PartitionDataSizeMax) = PartitionTetrahedraMax * PartitionTetrahedraSize;

// 4 root tetrahedra vertices in [-1,1] space scaled by RootExtent
// TODO
static const CFLOAT3(PartitionRootTetrahedra)[][4] = {
	{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
	{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
	{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
	{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }
};

// constant buffers

struct ProceduralGenConsts
{
	CFLOAT3(RootExtent);
	CFLOAT3(RootPosition);
	CFLOAT3(RefinementPoint);
	CFLOAT3(__pad0);
};

// work graph entry record

struct ProceduralGenInputRecord
{
	CUINT(GridSize) GPU_ONLY(: SV_DispatchGrid);
};

// descriptors

DESCRIPTOR_ConstantBuffer(ProceduralGenConsts, g_ProceduralConsts, 0);
DESCRIPTOR_RWByteAddressBuffer(g_PartitionData, 0);

#endif // PROCEDURAL_GENERATION_GRAPH_HLSLI