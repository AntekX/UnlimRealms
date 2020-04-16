#ifndef LIGHTING_COMMON_HLSLI
#define LIGHTING_COMMON_HLSLI

#include "CommonTypes.hlsli"

struct LightDesc
{
	CFLOAT3	(Color);
	CFLOAT	(Intensity);
	CFLOAT3	(Direction);
	CFLOAT	(__pad0);
	CFLOAT3	(Position);
	CFLOAT	(__pad1);
};

#define LIGHT_SOURCES_MAX 4
struct LightingDesc
{
	LightDesc LightSources[LIGHT_SOURCES_MAX];
	CUINT(LightSourceCount);
	CUINT3(__pad0);
};

#endif