///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef LIGHTING_COMMON_HLSLI
#define LIGHTING_COMMON_HLSLI

#include "CommonTypes.hlsli"

// structures are explicitly aligned to be used in/as constant buffers

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
	CUINT	(LightSourceCount);
	CUINT3	(__pad0);
};

struct MeshMaterialDesc
{
	CFLOAT3	(BaseColor);
	CFLOAT	(Roughness);
	CFLOAT3	(EmissiveColor);
	CFLOAT	(Metallic);
	CFLOAT3	(SheenColor);
	CFLOAT	(Reflectance);
	CFLOAT3	(AnisotropyDirection);
	CFLOAT	(Anisotropy);
	CFLOAT	(ClearCoat);
	CFLOAT	(ClearCoatRoughness);
	CFLOAT2	(__pad0);
};

#endif