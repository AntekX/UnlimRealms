///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"

sampler CommonSampler	: register(s0);
Texture2D HDRTexture	: register(t0);
Texture2D LumTexture	: register(t1);
Texture2D BloomTexture	: register(t2);

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 finalColor = 0.0;

	float4 hdrVal = HDRTexture.Sample(CommonSampler, input.uv);
	float4 lumData = LumTexture.Sample(CommonSampler, float2(0.0, 0.0));
	float bloom = 0;// BloomTexture.Sample(CommonSampler, input.uv).x; // todo
	
	float Lf = lumData.x;
	if (LogLuminance) Lf = exp(Lf);
#if 1
	// Reinhard
	float Lp = ComputeLuminance(hdrVal.rgb) + bloom;
	float L = LumKey / Lf * Lp ;
	float Lt = L * (1.0 + L / (LumWhite * LumWhite)) / (1.0 + L);
	finalColor = saturate(hdrVal * Lt);
#else
	// Exposure
	float T = pow(Lf, -1);
	finalColor = saturate(1.0 - exp(-T * hdrVal));
#endif

	//finalColor = saturate(finalColor + bloom);
	finalColor = LinearToGamma(finalColor);
	
	return finalColor;
}