///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "../GenericRender/Generic.hlsli"
#include "../ShaderLib/Math.hlsli"
#include "HDRRender.hlsli"

sampler PointSampler	: register(s0);
sampler LinearSampler	: register(s1);
Texture2D HDRTexture	: register(t0);
Texture2D LumTexture	: register(t1);
Texture2D BloomTexture	: register(t2);

#define TONEMAP_ACES_FITTED 1
#define TONEMAP_ACES_FILMIC 2
#define TONEMAP_UNCHARTED 3
#define TONEMAP_REINHARD 4
#define TONEMAP_TYPE TONEMAP_ACES_FILMIC

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
	{0.59719, 0.35458, 0.04823},
	{0.07600, 0.90834, 0.01566},
	{0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
	{ 1.60475, -0.53108, -0.07367},
	{-0.10208,  1.10813, -0.00605},
	{-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

float3 ACESFitted(float3 color)
{
	color = mul(ACESInputMat, color);

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	color = mul(ACESOutputMat, color);

	// Clamp to [0, 1]
	color = saturate(color);

	return color;
}

float4 filmicF(float4 x)
{
	float a = 0.22;
	float b = 0.30;
	float c = 0.10;
	float d = 0.20;
	float e = 0.01;
	float f = 0.30;
	return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float4 filmicACES(float4 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return ((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 color = HDRTexture.Sample(PointSampler, input.uv);
	float4 lumData = LumTexture.Sample(PointSampler, input.uv);
	float4 bloom = pow(max(BloomTexture.Sample(LinearSampler, input.uv) * BloomIntensity, 0.0), 0.66);

#if (LIGHT_SHAFTS)
	// use max() fn to avoid overbloom (light shafts sample same bloom RT)
	color += max(bloom, LightShaftsScreenSpace(input.uv));
#else
	color += bloom;
#endif

	// calculate exposed color

	float Lf = max(lumData.x, Eps); // average luminance
	#if (LOG2_LUMINANCE_TEXTURE)
	Lf = pow(2.0, Lf);
	#endif
	Lf = clamp(Lf, LumAdaptationMin, LumAdaptationMax);
	#if (0)
	float linearExposure = (LumKey / Lf);
	float exposure = log2(max(linearExposure, Eps));
	#else
	#if (0)
	// manual mode
	float EV100 = ComputeEV100(Aperture, ShutterTime, ISO);
	#else
	// automatic from avg luminance
	float EV100 = ComputeEV100FromAvgLuminance(Lf);
	#endif
	float exposure = ConvertEV100ToExposure(EV100);
	#endif
	color *= exposure;

	// apply tonemapping

#if (TONEMAP_ACES_FITTED == TONEMAP_TYPE)
	color.xyz = ACESFitted(color.xyz) / ACESFitted(LumWhite);
#elif (TONEMAP_ACES_FILMIC == TONEMAP_TYPE)
	// ACES filmic
	color = filmicACES(color) / filmicACES(LumWhite);
#elif (TONEMAP_UNCHARTED == TONEMAP_TYPE)
	// Filmic Uncharted2
	color = filmicF(color) / filmicF(LumWhite);
#elif (TONEMAP_REINHARD == TONEMAP_TYPE)
	// Simple
	color = color / (color + 1.0);
#endif

	color.xyz = LinearToSRGB(color.xyz);
	color.w = 1.0;

	return color;
}