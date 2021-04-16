#ifndef ATMOSPHERE_COMMON_HLSLI
#define ATMOSPHERE_COMMON_HLSLI

#include "../ShaderLib/CommonTypes.hlsli"

struct AtmosphereDesc
{
	CFLOAT	(InnerRadius);
	CFLOAT	(OuterRadius);
	CFLOAT	(ScaleDepth);
	CFLOAT	(G);
	CFLOAT	(Km);
	CFLOAT	(Kr);
	CFLOAT	(D);
	CFLOAT	(__padding);
};

#endif
