#ifndef GPU_WORK_GRAPHS_COMMON_HLSLI
#define GPU_WORK_GRAPHS_COMMON_HLSLI

#include "ShaderLib/CommonTypes.hlsli"

// constant buffers

struct WorkGraphGlobalConstants
{
};

// descriptors

DESCRIPTOR_RWByteAddressBuffer(g_UAV, 0);

#endif // GPU_WORK_GRAPHS_COMMON_HLSLI