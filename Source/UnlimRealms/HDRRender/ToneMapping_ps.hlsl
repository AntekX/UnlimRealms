///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"

sampler PointSampler	: register(s0);
sampler LinearSampler	: register(s1);
Texture2D HDRTexture	: register(t0);
Texture2D LumTexture	: register(t1);
Texture2D BloomTexture	: register(t2);

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 finalColor = 0.0;

	float4 hdrVal = HDRTexture.Sample(PointSampler, input.uv);
	float4 lumData = LumTexture.Sample(PointSampler, float2(0.0, 0.0));
	float4 bloom = BloomTexture.Sample(LinearSampler, input.uv);
	
	float Lf = (lumData.x + Eps);
	if (LogLuminance) Lf = exp(Lf);
	Lf = max(LumAdaptationMin, Lf);
#if 1
	// Reinhard
	float Lp = ComputeLuminance(hdrVal.rgb);
	float L = LumKey / Lf * Lp ;
	float Lt = L * (1.0 + L / (LumWhite * LumWhite)) / (1.0 + L);
	finalColor = saturate(hdrVal * Lt + bloom);
#else
	// Exposure
	float T = pow(Lf, -1);
	finalColor = saturate(1.0 - exp(-T * hdrVal));
#endif

	//finalColor = saturate(finalColor + bloom);
	finalColor = LinearToGamma(finalColor);
	
	return finalColor;
}