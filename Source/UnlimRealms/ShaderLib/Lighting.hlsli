#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "LightingCommon.hlsli"
#include "Math.hlsli"

//------------------------------------------------------------------------------
// Lighting code adapted from Filament engine:
// https://github.com/google/filament/tree/master/shaders/src
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// BRDF configuration
//------------------------------------------------------------------------------

// Diffuse BRDFs
#define DIFFUSE_LAMBERT             0
#define DIFFUSE_BURLEY              1

// Specular BRDF
// Normal distribution functions
#define SPECULAR_D_GGX              0

// Anisotropic NDFs
#define SPECULAR_D_GGX_ANISOTROPIC  0

// Cloth NDFs
#define SPECULAR_D_CHARLIE          0

// Visibility functions
#define SPECULAR_V_SMITH_GGX        0
#define SPECULAR_V_SMITH_GGX_FAST   1
#define SPECULAR_V_GGX_ANISOTROPIC  2
#define SPECULAR_V_KELEMEN          3
#define SPECULAR_V_NEUBELT          4

// Fresnel functions
#define SPECULAR_F_SCHLICK          0

#define BRDF_DIFFUSE                DIFFUSE_LAMBERT

#define BRDF_SPECULAR_D             SPECULAR_D_GGX
#define BRDF_SPECULAR_V             SPECULAR_V_SMITH_GGX_FAST
#define BRDF_SPECULAR_F             SPECULAR_F_SCHLICK

#define BRDF_CLEAR_COAT_D           SPECULAR_D_GGX
#define BRDF_CLEAR_COAT_V           SPECULAR_V_KELEMEN

#define BRDF_ANISOTROPIC_D          SPECULAR_D_GGX_ANISOTROPIC
#define BRDF_ANISOTROPIC_V          SPECULAR_V_GGX_ANISOTROPIC

#define BRDF_CLOTH_D                SPECULAR_D_CHARLIE
#define BRDF_CLOTH_V                SPECULAR_V_NEUBELT

//------------------------------------------------------------------------------
// Specular BRDF implementations
//------------------------------------------------------------------------------

float D_GGX(float roughness, float NoH, const float3 h)
{
	// Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"

	// In mediump, there are two problems computing 1.0 - NoH^2
	// 1) 1.0 - NoH^2 suffers floating point cancellation when NoH^2 is close to 1 (highlights)
	// 2) NoH doesn't have enough precision around 1.0
	// Both problem can be fixed by computing 1-NoH^2 in highp and providing NoH in highp as well

	// However, we can do better using Lagrange's identity:
	//      ||a x b||^2 = ||a||^2 ||b||^2 - (a . b)^2
	// since N and H are unit vectors: ||N x H||^2 = 1.0 - NoH^2
	// This computes 1.0 - NoH^2 directly (which is close to zero in the highlights and has
	// enough precision).
	// Overall this yields better performance, keeping all computations in mediump
	#if (0)
	float3 NxH = cross(shading_normal, h);
	float oneMinusNoHSquared = dot(NxH, NxH);
	#else
	float oneMinusNoHSquared = 1.0 - NoH * NoH;
	#endif

	float a = NoH * roughness;
	float k = roughness / (oneMinusNoHSquared + a * a);
	float d = k * k * (1.0 / Pi);
	return d;
}

float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH)
{
	// Burley 2012, "Physically-Based Shading at Disney"

	// The values at and ab are perceptualRoughness^2, a2 is therefore perceptualRoughness^4
	// The dot product below computes perceptualRoughness^8. We cannot fit in fp16 without clamping
	// the roughness to too high values so we perform the dot product and the division in fp32
	float a2 = at * ab;
	float3 d = float3(ab * ToH, at * BoH, a2 * NoH);
	float d2 = dot(d, d);
	float b2 = a2 / d2;
	return a2 * b2 * b2 * (1.0 / Pi);
}

float D_Charlie(float roughness, float NoH)
{
	// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
	float invAlpha = 1.0 / roughness;
	float cos2h = NoH * NoH;
	float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
	return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * Pi);
}

float V_SmithGGXCorrelated(float roughness, float NoV, float NoL)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
	float a2 = roughness * roughness;
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
	float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
	float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
	float v = 0.5 / (lambdaV + lambdaL);
	// a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
	// a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
	return v;
}

float V_SmithGGXCorrelated_Fast(float roughness, float NoV, float NoL)
{
	// Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
	float v = 0.5 / lerp(2.0 * NoL * NoV, NoL + NoV, roughness);
	return v;
}

float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV,
	float ToL, float BoL, float NoV, float NoL)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
	float lambdaV = NoL * length(float3(at * ToV, ab * BoV, NoV));
	float lambdaL = NoV * length(float3(at * ToL, ab * BoL, NoL));
	float v = 0.5 / (lambdaV + lambdaL);
	return v;
}

float V_Kelemen(float LoH)
{
	// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
	return (0.25 / (LoH * LoH));
}

float V_Neubelt(float NoV, float NoL)
{
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return (1.0 / (4.0 * (NoL + NoV - NoL * NoV)));
}

float3 F_Schlick(const float3 f0, float f90, float VoH)
{
	// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float3 F_Schlick(const float3 f0, float VoH)
{
	float f = pow(1.0 - VoH, 5.0);
	return f + f0 * (1.0 - f);
}

float F_Schlick(float f0, float f90, float VoH)
{
	return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

//------------------------------------------------------------------------------
// Specular BRDF dispatch
//------------------------------------------------------------------------------

float distribution(float roughness, float NoH, const float3 h)
{
	#if (BRDF_SPECULAR_D == SPECULAR_D_GGX)
	return D_GGX(roughness, NoH, h);
	#endif
}

float visibility(float roughness, float NoV, float NoL)
{
	#if (BRDF_SPECULAR_V == SPECULAR_V_SMITH_GGX)
	return V_SmithGGXCorrelated(roughness, NoV, NoL);
	#elif (BRDF_SPECULAR_V == SPECULAR_V_SMITH_GGX_FAST)
	return V_SmithGGXCorrelated_Fast(roughness, NoV, NoL);
	#endif
}

float3 fresnel(const float3 f0, float LoH)
{
	#if (BRDF_SPECULAR_F == SPECULAR_F_SCHLICK)
	float f90 = saturate(dot(f0, (float3)(50.0 * 0.33))); // Frostbyte trick, values blow 2% include prebaked micro occlusion (therefore 50)
	return F_Schlick(f0, f90, LoH);
	#endif
}

float distributionAnisotropic(float at, float ab, float ToH, float BoH, float NoH)
{
	#if (BRDF_ANISOTROPIC_D == SPECULAR_D_GGX_ANISOTROPIC)
	return D_GGX_Anisotropic(at, ab, ToH, BoH, NoH);
	#endif
}

float visibilityAnisotropic(float roughness, float at, float ab,
	float ToV, float BoV, float ToL, float BoL, float NoV, float NoL)
{
	#if (BRDF_ANISOTROPIC_V == SPECULAR_V_SMITH_GGX)
	return V_SmithGGXCorrelated(roughness, NoV, NoL);
	#elif BRDF_ANISOTROPIC_V == SPECULAR_V_GGX_ANISOTROPIC
	return V_SmithGGXCorrelated_Anisotropic(at, ab, ToV, BoV, ToL, BoL, NoV, NoL);
	#endif
}

float distributionClearCoat(float roughness, float NoH, const float3 h)
{
	#if (BRDF_CLEAR_COAT_D == SPECULAR_D_GGX)
	return D_GGX(roughness, NoH, h);
	#endif
}

float visibilityClearCoat(float LoH)
{
	#if (BRDF_CLEAR_COAT_V == SPECULAR_V_KELEMEN)
	return V_Kelemen(LoH);
	#endif
}

float distributionCloth(float roughness, float NoH)
{
	#if (BRDF_CLOTH_D == SPECULAR_D_CHARLIE)
	return D_Charlie(roughness, NoH);
	#endif
}

float visibilityCloth(float NoV, float NoL)
{
	#if (BRDF_CLOTH_V == SPECULAR_V_NEUBELT)
	return V_Neubelt(NoV, NoL);
	#endif
}

//------------------------------------------------------------------------------
// Diffuse BRDF implementations
//------------------------------------------------------------------------------

float Fd_Lambert()
{
	return 1.0 / Pi;
}

float Fd_Burley(float roughness, float NoV, float NoL, float LoH)
{
	// Burley 2012, "Physically-Based Shading at Disney"
	float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
	float lightScatter = F_Schlick(1.0, f90, NoL);
	float viewScatter = F_Schlick(1.0, f90, NoV);
	return lightScatter * viewScatter * (1.0 / Pi);
}

// Energy conserving wrap diffuse term, does *not* include the divide by pi
float Fd_Wrap(float NoL, float w)
{
	return saturate((NoL + w) / pow2(1.0 + w));
}

//------------------------------------------------------------------------------
// Diffuse BRDF dispatch
//------------------------------------------------------------------------------

float diffuse(float roughness, float NoV, float NoL, float LoH)
{
	#if (BRDF_DIFFUSE == DIFFUSE_LAMBERT)
	return Fd_Lambert();
	#elif (BRDF_DIFFUSE == DIFFUSE_BURLEY)
	return Fd_Burley(roughness, NoV, NoL, LoH);
	#endif
}

//------------------------------------------------------------------------------
// Common materials
//------------------------------------------------------------------------------

#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define MIN_ROUGHNESS            0.002025
#define MIN_N_DOT_V 1e-4

float clampNoV(float NoV)
{
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return max(NoV, MIN_N_DOT_V);
}

float3 computeDiffuseColor(const float4 baseColor, float metallic)
{
	return baseColor.rgb * (1.0 - metallic);
}

float3 computeF0(const float4 baseColor, float metallic, float reflectance)
{
	return baseColor.rgb * metallic + (reflectance * (1.0 - metallic));
}

float computeDielectricF0(float reflectance)
{
	return 0.16 * reflectance * reflectance;
}

float computeMetallicFromSpecularColor(const float3 specularColor)
{
	return max3(specularColor);
}

float computeRoughnessFromGlossiness(float glossiness)
{
	return 1.0 - glossiness;
}

float perceptualRoughnessToRoughness(float perceptualRoughness)
{
	return perceptualRoughness * perceptualRoughness;
}

float roughnessToPerceptualRoughness(float roughness)
{
	return sqrt(roughness);
}

float iorToF0(float transmittedIor, float incidentIor)
{
	return pow2((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

float f0ToIor(float f0)
{
	float r = sqrt(f0);
	return (1.0 + r) / (1.0 - r);
}

float3 f0ClearCoatToSurface(const float3 f0)
{
	// Approximation of iorTof0(f0ToIor(f0), 1.5)
	// This assumes that the clear coat layer has an IOR of 1.5
	return saturate(f0 * (f0 * (0.941892 - 0.263008 * f0) + 0.346479) - 0.0285998);
}

//------------------------------------------------------------------------------
// Material
//------------------------------------------------------------------------------

struct MaterialInputs
{
	float4	baseColor;
	float4	emissive;
	float3	normal;
	float	ambientOcclusion;
	#if defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
	float3  specularColor;
	float	glossiness;
	#else
	float	roughness;
	float	metallic;
	float	reflectance;
	#endif
	
	// coating layer
	bool	hasClearCoat;
	float	clearCoat;
	float	clearCoatRoughness;
	float3	clearCoatNormal;

	// anisotropic specular
	bool	hasAnisotropy;
	float	anisotropy;
	float3	anisotropyDirection;

	// subsurface scattering
	bool	hasSubsurfaceScattering;
	float	subsurfacePower;
	float3  subsurfaceColor;
	float	thickness; // shared with refraction

	// cloth
	bool	isCloth;
	float3  sheenColor;
	
	// refraction
	bool	hasRefraction;
	float3	absorption;
	float	transmission;
	float	ior;
};

void initMaterial(inout MaterialInputs material)
{
	material.baseColor = float4(1.0, 1.0, 1.0, 1.0);
	material.emissive = float4(0.0, 0.0, 0.0, 1.0);
	material.normal = WorldUp;
	material.ambientOcclusion = 1.0;
	#if defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
	material.glossiness = 0.0;
	material.specularColor = float3(0.0, 0.0, 0.0);
	#else
	material.roughness = 1.0;
	material.metallic = 0.0;
	material.reflectance = (material.hasRefraction ? 0.0 : 0.5);
	#endif
	material.clearCoat = (material.hasClearCoat ? 1.0 : 0.0);
	material.clearCoatRoughness = 0.0;
	material.clearCoatNormal = WorldUp;
	material.anisotropy = (material.hasAnisotropy ? 1.0 : 0.0);
	material.anisotropyDirection = float3(1.0, 0.0, 0.0);
	material.thickness = (material.hasSubsurfaceScattering ? 0.5 : 0.0);
	material.subsurfacePower = (material.hasSubsurfaceScattering ? 12.234 : 0.0);
	material.subsurfaceColor = (material.hasSubsurfaceScattering ? float3(1.0, 1.0, 1.0) : 0.0);
	material.sheenColor = (material.isCloth ? sqrt(material.baseColor.rgb) : 0.0);
	material.absorption = (material.hasRefraction ? float3(0.0, 0.0, 0.0) : 0.0);
	material.transmission = (material.hasRefraction ? 1.0 : 0.0);
	material.ior = (material.hasRefraction ? 1.5 : 0.0);
}

//------------------------------------------------------------------------------
// Lighting evaluation
//------------------------------------------------------------------------------

struct LightingParams
{
	float3	worldPos;
	float3	normal;
	float3	viewPos;
	float3	viewDir;
	float3	viewDist;

	float3	diffuseColor;
	float3	f0;
	float	roughness;
	float	perceptualRoughness;
	float	perceptualRoughnessUnclamped;
	float	energyCompensation;

	// cloth
	bool	isCloth;
	
	// subsurface scattering
	bool	hasSubsurfaceScattering;
	float	subsurfacePower;
	float3	subsurfaceColor;
	float	thickness;

	// refraction 
	bool	hasRefraction;
	float3	absorption;
	float	transmission;
	float	etaIR; // air -> material
	float	etaRI; // material -> air

	// coating layer
	bool	hasClearCoat;
	float	clearCoat;
	float	clearCoatPerceptualRoughness;
	float	clearCoatRoughness;

	// anisotropic specular
	bool	hasAnisotropy;
	float	anisotropy;
	float3	anisotropicT;
	float3	anisotropicB;
};

float computeDiffuseAlpha(float a)
{
	#if defined(BLEND_MODE_TRANSPARENT) || defined(BLEND_MODE_FADE) || defined(BLEND_MODE_MASKED)
	return a;
	#else
	return 1.0;
	#endif
}

#if defined(BLEND_MODE_MASKED)
float computeMaskedAlpha(float a)
{
	return (a - getMaskThreshold()) + 0.5;
}
#endif

void applyAlphaMask(inout float4 baseColor)
{
	#if defined(BLEND_MODE_MASKED)
	baseColor.a = computeMaskedAlpha(baseColor.a);
	if (baseColor.a <= 0.0) {
		discard;
	}
	#endif
}

/**
 * Computes all the parameters required for shading.
 * These parameters are derived from the MaterialInputs structure computed
 * by the user's material code.
 *
 * This function is also responsible for discarding the fragment when alpha
 * testing fails.
 */
void getLightingParams(const float3 worldPos, const float3 viewPos, const MaterialInputs material, inout LightingParams lightingParams)
{
	float4 baseColor = material.baseColor;
	applyAlphaMask(baseColor);

	#if defined(BLEND_MODE_FADE) && !defined(SHADING_MODEL_UNLIT)
	// must be "unpremultiplied" if color is premultiplied by alpha
	//unpremultiply(baseColor);
	#endif

	lightingParams.worldPos = worldPos;
	lightingParams.normal = material.normal;
	lightingParams.viewPos = viewPos;
	lightingParams.viewDir = (lightingParams.viewPos - lightingParams.worldPos);
	lightingParams.viewDist = length(lightingParams.viewDir);
	lightingParams.viewDir /= (lightingParams.viewDist + 1.0e-5);

	lightingParams.isCloth = material.isCloth;
	if (lightingParams.isCloth)
	{
		lightingParams.diffuseColor = baseColor.rgb;
		lightingParams.f0 = material.sheenColor;
		lightingParams.subsurfaceColor = material.subsurfaceColor;
	}
	else
	{
		#if defined(SHADING_MODEL_SPECULAR_GLOSSINESS)

		// This is from KHR_materials_pbrSpecularGlossiness.
		float3 specularColor = material.specularColor;
		float metallic = computeMetallicFromSpecularColor(specularColor);
		lightingParams.diffuseColor = computeDiffuseColor(baseColor, metallic);
		lightingParams.f0 = specularColor;

		#else

		if (material.hasRefraction && !material.reflectance && material.ior)
		{
			lightingParams.diffuseColor = baseColor.rgb;
			// If refraction is enabled, and reflectance is not set in the material, but ior is,
			// then use it -- othterwise proceed as usual.
			lightingParams.f0 = (float3)iorToF0(material.ior, 1.0);
		}
		else
		{
			lightingParams.diffuseColor = computeDiffuseColor(baseColor, material.metallic);
			// Assumes an interface from air to an IOR of 1.5 for dielectrics
			float reflectance = computeDielectricF0(material.reflectance);
			lightingParams.f0 = computeF0(baseColor, material.metallic, reflectance);
		}

		#endif
	}

	lightingParams.hasRefraction = material.hasRefraction;
	if (lightingParams.hasRefraction)
	{
		// Air's Index of refraction is 1.000277 at STP but everybody uses 1.0
		const float airIor = 1.0;
		float materialor = 1.0;
		if (material.ior > 0)
		{
			// if ior is set in the material, use it (can lead to unrealistic materials)
			materialor = max(1.0, material.ior);
		}
		else
		{
			// [common case] ior is not set in the material, deduce it from F0
			materialor = f0ToIor(lightingParams.f0.g);
		}
		lightingParams.etaIR = airIor / materialor;  // air -> material
		lightingParams.etaRI = materialor / airIor;  // material -> air
		lightingParams.transmission = saturate(material.transmission);
		lightingParams.absorption = saturate(material.absorption);
		lightingParams.thickness = max(0.0, material.thickness);
	}

	lightingParams.hasClearCoat = material.hasClearCoat;
	if (lightingParams.hasClearCoat)
	{
		lightingParams.clearCoat = material.clearCoat;

		// Clamp the clear coat roughness to avoid divisions by 0
		lightingParams.clearCoatPerceptualRoughness = material.clearCoatRoughness;
		lightingParams.clearCoatPerceptualRoughness = clamp(lightingParams.clearCoatPerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
		lightingParams.clearCoatRoughness = perceptualRoughnessToRoughness(lightingParams.clearCoatPerceptualRoughness);

		#if defined(CLEAR_COAT_IOR_CHANGE)
		// The base layer's f0 is computed assuming an interface from air to an IOR
		// of 1.5, but the clear coat layer forms an interface from IOR 1.5 to IOR
		// 1.5. We recompute f0 by first computing its IOR, then reconverting to f0
		// by using the correct interface
		lightingParams.f0 = mix(lightingParams.f0, f0ClearCoatToSurface(lightingParams.f0), lightingParams.clearCoat);
		#endif
	}

	#if defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
	lightingParams.perceptualRoughness = computeRoughnessFromGlossiness(material.glossiness);
	#else
	lightingParams.perceptualRoughness = material.roughness;
	#endif
	if (material.hasClearCoat)
	{
		// This is a hack but it will do: the base layer must be at least as rough
		// as the clear coat layer to take into account possible diffusion by the
		// top layer
		float basePerceptualRoughness = max(lightingParams.perceptualRoughness, lightingParams.clearCoatPerceptualRoughness);
		lightingParams.perceptualRoughness = lerp(lightingParams.perceptualRoughness, basePerceptualRoughness, lightingParams.clearCoat);
	}
	lightingParams.perceptualRoughnessUnclamped = lightingParams.perceptualRoughness;
	// Clamp the roughness to a minimum value to avoid divisions by 0 during lighting
	lightingParams.perceptualRoughness = clamp(lightingParams.perceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
	// Remaps the roughness to a perceptually linear roughness (roughness^2)
	lightingParams.roughness = perceptualRoughnessToRoughness(lightingParams.perceptualRoughness);

	lightingParams.hasSubsurfaceScattering = material.hasSubsurfaceScattering;
	if (lightingParams.hasSubsurfaceScattering)
	{
		lightingParams.subsurfacePower = material.subsurfacePower;
		lightingParams.subsurfaceColor = material.subsurfaceColor;
		lightingParams.thickness = saturate(material.thickness);
	}

	lightingParams.hasAnisotropy = material.hasAnisotropy;
	if (lightingParams.hasAnisotropy)
	{
		float3 direction = material.anisotropyDirection;
		lightingParams.anisotropy = material.anisotropy;
		lightingParams.anisotropicT = normalize(cross(WorldUp, direction));
		lightingParams.anisotropicB = cross(direction, lightingParams.anisotropicT);
	}

	lightingParams.energyCompensation = 1.0;
	#if (0)
	// note: requires pregenerated DFG LUT
	lightingParams.dfg = prefilteredDFG(pixel.perceptualRoughness, lightingParams.NoV); // Pre-filtered DFG term used for image-based lighting
	if (!material.isCloth)
	{
		// Energy compensation for multiple scattering in a microfacet model
		// See "Multiple-Scattering Microfacet BSDFs with the Smith Model"
		lightingParams.energyCompensation = 1.0 + pixel.f0 * (1.0 / pixel.dfg.y - 1.0);
	}
	#endif
}

#if (0) // TODO
/**
 * This function evaluates all lights one by one:
 * - Image based lights (IBL)
 * - Directional lights
 * - Punctual lights
 *
 * Area lights are currently not supported.
 *
 * Returns a pre-exposed HDR RGBA color in linear space.
 */
float4 evaluateLights(const MaterialInputs material)
{
	PixelParams pixel;
	getPixelParams(material, pixel);

	// Ideally we would keep the diffuse and specular components separate
	// until the very end but it costs more ALUs on mobile. The gains are
	// currently not worth the extra operations
	float3 color = float3(0.0);

	// We always evaluate the IBL as not having one is going to be uncommon,
	// it also saves 1 shader variant
	evaluateIBL(material, pixel, color);

	#if defined(HAS_DIRECTIONAL_LIGHTING)
	evaluateDirectionalLight(material, pixel, color);
	#endif

	#if defined(HAS_DYNAMIC_LIGHTING)
	evaluatePunctualLights(pixel, color);
	#endif

	#if defined(BLEND_MODE_FADE) && !defined(SHADING_MODEL_UNLIT)
	// In fade mode we un-premultiply baseColor early on, so we need to
	// premultiply again at the end (affects diffuse and specular lighting)
	color *= material.baseColor.a;
	#endif

	return float4(color, computeDiffuseAlpha(material.baseColor.a));
}

void addEmissive(const MaterialInputs material, inout float4 color)
{
	#if defined(MATERIAL_HAS_EMISSIVE)
	highp float4 emissive = material.emissive;
	highp float attenuation = mix(1.0, frameUniforms.exposure, emissive.w);
	color.rgb += emissive.rgb * attenuation;
	#endif
}

/**
 * Evaluate lit materials. The actual shading model used to do so is defined
 * by the function surfaceShading() found in shading_model_*.fs.
 *
 * Returns a pre-exposed HDR RGBA color in linear space.
 */
float4 evaluateMaterial(const MaterialInputs material)
{
	float4 color = evaluateLights(material);
	addEmissive(material, color);
	return color;
}
#endif // TEMP: WIP section


void GetLightIntensityAndDir(LightingParams lightingParams, const LightDesc lightSource, out float3 lightColor, out float3 lightDir)
{
	lightDir = -lightSource.Direction; // dir towards light source
	float lightAttenuation = 1.0;
	[flatten] if (LightType_Spherical == lightSource.Type)
	{
		lightDir = lightSource.Position.xyz - lightingParams.worldPos.xyz;
		float dist = length(lightDir);
		lightAttenuation = 1.0 / max(dist * dist, max(lightSource.Size * lightSource.Size, 1.0e-4));
		lightDir /= dist;
	}
	lightColor = lightSource.Color * lightSource.Intensity * lightAttenuation;
}

float3 GetLightIntensity(LightingParams lightingParams, const LightDesc lightSource)
{
	float3 lightColor;
	float3 lightDir;
	GetLightIntensityAndDir(lightingParams, lightSource, lightColor, lightDir);
	return lightColor;
}

float4 EvaluateDirectLighting(LightingParams lightingParams, const LightDesc lightSource, const float directOcclusion, const float specularOcclusion)
{
	// common inputs

	float3 lightColor;
	float3 lightDir;
	GetLightIntensityAndDir(lightingParams, lightSource, lightColor, lightDir);

	float3 halfV = normalize(lightingParams.viewDir + lightDir);
	float NoV = abs(dot(lightingParams.normal, lightingParams.viewDir)) + 1.0e-5; // safe NoV, avoids artifacts in visibility function
	float NoL = saturate(dot(lightingParams.normal, lightDir));
	float NoH = saturate(dot(lightingParams.normal, halfV));
	float LoH = saturate(dot(lightDir, halfV));

	// diffuse term

	float3 Fd = lightingParams.diffuseColor * diffuse(lightingParams.roughness, NoV, NoL, LoH);
	Fd *= NoL * lightColor * directOcclusion;

	// specular term

	float3 Fr = 0.0;
	if (lightingParams.hasAnisotropy)
	{
		float3 l = lightDir;
		float3 t = lightingParams.anisotropicT;
		float3 b = lightingParams.anisotropicB;
		float3 v = lightingParams.viewDir;
		float3 h = halfV;

		float ToV = dot(t, v);
		float BoV = dot(b, v);
		float ToL = dot(t, l);
		float BoL = dot(b, l);
		float ToH = dot(t, h);
		float BoH = dot(b, h);

		// Anisotropic parameters: at and ab are the roughness along the tangent and bitangent
		// to simplify materials, we derive them from a single roughness parameter
		// Kulla 2017, "Revisiting Physically Based Shading at Imageworks"
		float at = max(lightingParams.roughness * (1.0 + lightingParams.anisotropy), MIN_ROUGHNESS);
		float ab = max(lightingParams.roughness * (1.0 - lightingParams.anisotropy), MIN_ROUGHNESS);

		// specular anisotropic BRDF
		float D = distributionAnisotropic(at, ab, ToH, BoH, NoH);
		float V = visibilityAnisotropic(lightingParams.roughness, at, ab, ToV, BoV, ToL, BoL, NoV, NoL);
		float3 F = fresnel(lightingParams.f0, LoH);

		Fr = max(0.0, (D * V) * F);
	}
	else
	{
		float D = distribution(lightingParams.roughness, NoH, halfV);
		float V = visibility(lightingParams.roughness, NoV, NoL);
		float3 F = fresnel(lightingParams.f0, LoH);

		Fr = (D * V) * F;
	}

	Fr *= lightingParams.energyCompensation * lightColor * specularOcclusion;

	// final

	float3 lightingResult = Fd + Fr;

	return float4(lightingResult, 1.0);
}

float3 FresnelReflectance(LightingParams lightingParams)
{
	float3 reflectedDir = reflect(-lightingParams.viewDir, lightingParams.normal);
	float3 halfV = normalize(lightingParams.viewDir + reflectedDir);
	float VoH = saturate(dot(lightingParams.viewDir, halfV));

	float3 F = fresnel(lightingParams.f0, VoH);

	return F;
}

#endif