#ifndef LIGHTING_COMMON_HLSLI
#define LIGHTING_COMMON_HLSLI

#include "CommonTypes.hlsli"

struct LightDesc
{
	CFLOAT3	(Color);
	CFLOAT	(Intensity);
	CFLOAT3	(Direction);
	CFLOAT3	(Position);
	CFLOAT2	(__padding);
};

#define LIGHT_SOURCES_MAX 4
struct LightingDesc
{
	// TODO: array declared this way does not correspond to compiled shader buffer
	LightDesc LightSources[LIGHT_SOURCES_MAX];
	CUINT(LightSourceCount);
	CUINT3(__padding);
};

#endif