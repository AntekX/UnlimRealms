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

static const CFLOAT(SolarIlluminanceNoon) = 123100.0f; // lux
static const CFLOAT(SolarIlluminanceEvening) = 79000.0f; // lux
static const CFLOAT(LuminousEfficacyMax) = 683.0f; // Lm/W
static const CFLOAT(SolarIrradianceTopAtmosphere) = 1362.0f; // W/m2
static const CFLOAT(SolarIlluminanceTopAtmosphere) = 930246.0f; // lux, SolarIrradianceTopAtmosphere * LuminousEfficacyMax
static const CFLOAT(SolarDiskAngle) = 0.00925f; // 0.53 deg
static const CFLOAT(SolarDiskHalfAngle) = 0.004625f;
static const CFLOAT(SolarDiskAngleTangent) = 0.009251f; // tan(SolarDiskAngle)
static const CFLOAT(SolarDiskHalfAngleTangent) = 0.004625f; // tan(SolarDiskHalfAngle)

// structures are explicitly aligned to be used in/as constant buffers

static const CUINT(LightType_Directional) = 0;
static const CUINT(LightType_Spherical) = 1;
static const CUINT(LightType_Spot) = 2;

struct LightDesc
{
	CFLOAT3	(Color);
	CFLOAT	(Intensity);
	CFLOAT3	(Direction);
	CFLOAT	(Size);
	CFLOAT3	(Position);
	CFLOAT	(IntensityTopAtmosphere);
	CUINT	(Type);
	CFLOAT3	(__pad0);
};
static const CUINT(LightDescSize) = 64;

#define LIGHT_SOURCES_MAX 8
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
static const CUINT(MeshMaterialDescSize) = 80;

#endif