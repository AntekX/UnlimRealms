
#include "Lighting.hlsli"
#include "HDRRender/HDRRender.hlsli"
#include "Atmosphere/AtmosphericScattering.hlsli"

#pragma pack_matrix(row_major)

[shader("compute")]
[numthreads(8, 8, 1)]
void dummyShaderToMakeFXCHappy(const uint3 dispatchThreadId : SV_DispatchThreadID)
{

}