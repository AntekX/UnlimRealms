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

struct MaterialDesc
{
	CFLOAT3	(BaseColor); // diffuse or specular color depending on the metallic
	CFLOAT	(Roughness);
	CFLOAT3	(Normal);
	CFLOAT	(Metallic);
	CFLOAT3	(Emissive);
	CFLOAT	(Reflectance);
	CFLOAT	(AmbientOcclusion);
	CFLOAT3	(__pad0);

	// second layer
	CFLOAT3	(ClearCoatNormal);
	CFLOAT	(ClearCoat);
	CFLOAT	(ClearCoatRoughness);
	CFLOAT3	(__pad1);

	// cloth
	CFLOAT3	(SheenColor);
	CFLOAT	(__pad2);

	#if (0)
	// anisotropic specular
	CFLOAT3	(AnisotropyDirection);
	CFLOAT	(Anisotropy);

	// subsurface scattering
	CFLOAT3	(SubsurfaceColor);
	CFLOAT	(SubsurfacePower);
	CFLOAT	(Thickness);
	CFLOAT3	(__pad2);

	// refraction
	CFLOAT3 (Absorption); // use either Absorption or Transmission
	CFLOAT	(Transmission);
	CFLOAT	(IndexOfRefracttion); // usually used as constant = 1.5
	#endif
};

#endif