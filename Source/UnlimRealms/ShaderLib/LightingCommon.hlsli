///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef LIGHTING_COMMON_HLSLI
#define LIGHTING_COMMON_HLSLI

#include "CommonTypes.hlsli"

// useful lighting constants

static const CFLOAT(SolarIlluminanceNoon) = 123100.0; // lux
static const CFLOAT(SolarIlluminanceEvening) = 79000.0; // lux
static const CFLOAT(LuminousEfficacyMax) = 683.0; // Lm/W
static const CFLOAT(SolarIrradianceTopAtmosphere) = 1362.0; // W/m2
static const CFLOAT(SolarIlluminanceTopAtmosphere) = 930246.0; // lux, SolarIrradianceTopAtmosphere * LuminousEfficacyMax

// structures are explicitly aligned to be used in/as constant buffers

static const CUINT(LightType_Directional) = 0;
static const CUINT(LightType_Point) = 1;
static const CUINT(LightType_Spot) = 2;

struct LightDesc
{
	CFLOAT3	(Color);
	CFLOAT	(Intensity);
	CFLOAT3	(Direction);
	CUINT	(Type);
	CFLOAT3	(Position);
	CFLOAT	(IntensityTopAtmosphere);
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