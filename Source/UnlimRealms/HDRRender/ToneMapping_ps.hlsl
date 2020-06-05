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

#if (1)
	// ACES filmic
	color = filmicACES(color) / filmicACES(LumWhite);
#elif (0)
	// Filmic Uncharted2
	color = filmicF(color) / filmicF(LumWhite);
#elif (0)
	// Simple
	color = color / (color + 1.0);
#endif

	color.xyz = LinearToSRGB(color.xyz);
	color.w = 1.0;

	return color;
}