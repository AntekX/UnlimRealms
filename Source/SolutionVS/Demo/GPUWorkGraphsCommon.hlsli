#ifndef GPU_WORK_GRAPHS_COMMON_HLSLI
#define GPU_WORK_GRAPHS_COMMON_HLSLI

#include "ShaderLib/CommonTypes.hlsli"

// constant buffers

struct WorkGraphGlobalConstants
{
	CUINT4(pad);
};

// work graph entry record

struct entryRecord
{
    CUINT(gridSize) GPU_ONLY(: SV_DispatchGrid);
    CUINT(recordIndex);
};

// descriptors

DESCRIPTOR_ConstantBuffer(WorkGraphGlobalConstants, g_CB, 0);
DESCRIPTOR_RWByteAddressBuffer(g_UAV, 0);

#endif // GPU_WORK_GRAPHS_COMMON_HLSLI