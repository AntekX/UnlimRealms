///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
//  Based on "Adaptive Temporal Tone Mapping"
//	by ShaunDavidRamsey, J.ThomasJohnsonIII, CharlesHansen 
//  http://www.sci.utah.edu/~tjohnson/rp/attm.pdf
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};
sampler sampler0	: register(s0);
Texture2D hdrTexture	: register(t0);
Texture2D avgLumTexture	: register(t1);

cbuffer Constants : register(b1)
{
	float2 SrcTargetSize;
	float LumKey;
	float LumWhite;
};
static const float Gamma = 2.2;
static const float GammaRcp = 1.0 / Gamma;
static const float Eps = 1.0e-6;

float4 main(PS_INPUT input) : SV_Target
{
	float4 finalColor = 0.0;

	float4 hdrVal = hdrTexture.Sample(sampler0, input.uv);
	float4 lumData = avgLumTexture.Sample(sampler0, float2(0.0, 0.0));
	float Lf = exp(log(lumData.x + Eps));
#if 1
	// Reinhard
	float Lp = 0.27 * hdrVal.r + 0.67 * hdrVal.g + 0.06 * hdrVal.b;
	float L = LumKey / Lf * Lp ;
	float Lt = L * (1.0 + L / (LumWhite * LumWhite)) / (1.0 + L);
	finalColor = saturate(hdrVal * Lt);
#else
	// Exposure
	float T = pow(Lf, -1);
	finalColor = saturate(1.0 - exp(-T * hdrVal));
#endif

	// linear to gamma
	finalColor = pow(finalColor, GammaRcp);
	
	return finalColor;
}